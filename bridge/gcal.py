import json
import os
import threading
import time
from datetime import datetime, timedelta, timezone
from urllib.error import HTTPError, URLError
from urllib.parse import urlencode
from urllib.request import Request, urlopen

from spotify import USER_AGENT, ascii_text

TOKEN_URL = "https://oauth2.googleapis.com/token"
API = "https://www.googleapis.com/calendar/v3"
DEFAULT_DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "google.json")

CACHE_SECONDS = 300
MAX_EVENTS = 16
MAX_FETCH = 50
TITLE_CHARS = 27
MAX_OFFSET_MINUTES = 14 * 60
UNTITLED = "(no title)"


class CalendarError(Exception):
    def __init__(self, code):
        super().__init__(code)
        self.code = code


class GoogleCalendar:
    def __init__(self, data_file=None, api=None, token_url=None, clock=time.time):
        self.data_file = data_file or os.environ.get("GOOGLE_DATA") or DEFAULT_DATA
        self.api = api or os.environ.get("GOOGLE_API_BASE") or API
        self.token_url = token_url or os.environ.get("GOOGLE_TOKEN_URL") or TOKEN_URL
        self._clock = clock
        self._token_lock = threading.RLock()
        self._fetch_lock = threading.Lock()
        self._access = None
        self._expires = 0.0
        self._cache = {}
        self.client_id = self.client_secret = self.refresh_token = None
        try:
            with open(self.data_file) as handle:
                stored = json.load(handle)
            self.client_id = stored.get("client_id") or None
            self.client_secret = stored.get("client_secret") or None
            self.refresh_token = stored.get("refresh_token") or None
        except (OSError, ValueError, AttributeError):
            pass

    def _token(self):
        with self._token_lock:
            if not (self.client_id and self.client_secret and self.refresh_token):
                raise CalendarError("not_linked")
            if self._access and self._clock() < self._expires - 30:
                return self._access
            body = urlencode(
                {
                    "client_id": self.client_id,
                    "client_secret": self.client_secret,
                    "refresh_token": self.refresh_token,
                    "grant_type": "refresh_token",
                }
            ).encode()
            request = Request(
                self.token_url,
                data=body,
                headers={"Content-Type": "application/x-www-form-urlencoded", "User-Agent": USER_AGENT},
            )
            try:
                with urlopen(request, timeout=10) as response:
                    granted = json.load(response)
                self._access = granted["access_token"]
                self._expires = self._clock() + granted.get("expires_in", 3600)
            except HTTPError as error:
                self._access = None
                raise CalendarError("not_linked" if error.code in (400, 401) else "upstream")
            except (URLError, OSError, ValueError, KeyError):
                raise CalendarError("upstream")
            return self._access

    def _get(self, path, params, retried=False):
        request = Request(
            f"{self.api}{path}?{urlencode(params)}",
            headers={"Authorization": "Bearer " + self._token(), "User-Agent": USER_AGENT},
        )
        try:
            with urlopen(request, timeout=10) as response:
                return json.load(response)
        except HTTPError as error:
            if error.code == 401 and not retried:
                with self._token_lock:
                    self._access = None
                return self._get(path, params, True)
            raise CalendarError("rate_limited" if error.code == 429 else "upstream")
        except (URLError, OSError, ValueError):
            raise CalendarError("upstream")

    @staticmethod
    def _declined(item):
        return any(a.get("self") and a.get("responseStatus") == "declined" for a in item.get("attendees") or [])

    def _events_for(self, day_start, tz, offset):
        day_end = day_start + timedelta(days=1)
        utc = lambda moment: moment.astimezone(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        payload = self._get(
            "/calendars/primary/events",
            {
                "timeMin": utc(day_start),
                "timeMax": utc(day_end),
                "singleEvents": "true",
                "orderBy": "startTime",
                "maxResults": MAX_FETCH,
                "fields": "items(summary,status,start,end,attendees(self,responseStatus))",
            },
        )
        events = []
        for item in payload.get("items", []) if isinstance(payload, dict) else []:
            start, end = item.get("start") or {}, item.get("end") or {}
            if item.get("status") == "cancelled" or "dateTime" not in start or "dateTime" not in end:
                continue
            if self._declined(item):
                continue
            try:
                first = datetime.fromisoformat(start["dateTime"]).astimezone(tz)
                last = datetime.fromisoformat(end["dateTime"]).astimezone(tz)
            except ValueError:
                continue
            begin = max(0, int((first - day_start).total_seconds() // 60))
            finish = min(1440, int((last - day_start).total_seconds() // 60))
            if finish <= begin:
                continue
            title = ascii_text(item.get("summary") or "", TITLE_CHARS) or UNTITLED
            events.append({"s": begin, "e": finish, "t": title})
        events.sort(key=lambda event: (event["s"], event["e"]))
        return events

    def today(self, offset_minutes):
        if not isinstance(offset_minutes, int) or abs(offset_minutes) > MAX_OFFSET_MINUTES:
            raise ValueError("bad offset")
        tz = timezone(timedelta(minutes=offset_minutes))
        now = self._clock()
        day_start = datetime.fromtimestamp(now, tz).replace(hour=0, minute=0, second=0, microsecond=0)
        date = day_start.strftime("%Y-%m-%d")

        with self._fetch_lock:
            cached = self._cache.get(offset_minutes)
            if cached and cached["date"] == date and now - cached["at"] < CACHE_SECONDS:
                return dict(cached["body"], age=int(now - cached["at"]))
            try:
                events = self._events_for(day_start, tz, offset_minutes)
            except CalendarError as error:
                if error.code != "not_linked" and cached and cached["date"] == date:
                    return dict(cached["body"], age=int(now - cached["at"]))
                raise
            body = {"ok": True, "date": date, "n": len(events), "events": events[:MAX_EVENTS]}
            self._cache[offset_minutes] = {"at": now, "date": date, "body": body}
            return dict(body, age=0)
