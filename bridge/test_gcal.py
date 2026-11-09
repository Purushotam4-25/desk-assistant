import json
import os
import sys
import tempfile
import threading
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse
from urllib.request import Request, urlopen

import app
import gcal

failures = []
state = {}
calls = []

NOW = datetime(2026, 11, 9, 12, 0, tzinfo=timezone.utc).timestamp()
OFFSET = 330


def item(summary, start, end, **extra):
    return {"summary": summary, "start": {"dateTime": start}, "end": {"dateTime": end}, **extra}


def reset():
    calls.clear()
    state.clear()
    state.update(
        {
            "token_status": 200,
            "events_status": 200,
            "fail_first_events": False,
            "items": [
                item("Design review", "2026-11-09T10:00:00+05:30", "2026-11-09T11:00:00+05:30"),
                item("Standup", "2026-11-09T08:30:00+05:30", "2026-11-09T09:00:00+05:30"),
                item("Café ’Sync’ हिन्दी", "2026-11-09T05:00:00Z", "2026-11-09T06:00:00Z"),
                item("", "2026-11-09T12:00:00+05:30", "2026-11-09T12:30:00+05:30"),
                {"summary": "Holiday", "start": {"date": "2026-11-09"}, "end": {"date": "2026-11-10"}},
                item("Cancelled", "2026-11-09T14:00:00+05:30", "2026-11-09T15:00:00+05:30", status="cancelled"),
                item(
                    "Declined",
                    "2026-11-09T15:00:00+05:30",
                    "2026-11-09T16:00:00+05:30",
                    attendees=[{"self": True, "responseStatus": "declined"}],
                ),
                item(
                    "Accepted",
                    "2026-11-09T16:00:00+05:30",
                    "2026-11-09T17:00:00+05:30",
                    attendees=[{"self": True, "responseStatus": "accepted"}, {"responseStatus": "declined"}],
                ),
                item("Overnight in", "2026-11-08T22:00:00+05:30", "2026-11-09T01:00:00+05:30"),
                item("Overnight out", "2026-11-09T23:00:00+05:30", "2026-11-10T02:00:00+05:30"),
            ],
        }
    )


class Fake(BaseHTTPRequestHandler):
    def send(self, status, body):
        payload = json.dumps(body).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0"))
        form = parse_qs(self.rfile.read(length).decode())
        calls.append(("POST", self.path, {k: v[0] for k, v in form.items()}))
        if state["token_status"] != 200:
            self.send(state["token_status"], {"error": "invalid_grant"})
        else:
            self.send(200, {"access_token": "access-1", "expires_in": 3600})

    def do_GET(self):
        url = urlparse(self.path)
        calls.append(("GET", url.path, {k: v[0] for k, v in parse_qs(url.query).items()}, self.headers.get("Authorization")))
        if state["fail_first_events"]:
            state["fail_first_events"] = False
            self.send(401, {"error": "expired"})
        elif state["events_status"] != 200:
            self.send(state["events_status"], {"error": "boom"})
        else:
            self.send(200, {"items": state["items"]})

    def log_message(self, format, *args):
        pass


def check(condition, label):
    print(("ok   " if condition else "FAIL ") + label)
    if not condition:
        failures.append(label)


def make_client(linked=True, clock=lambda: NOW):
    path = os.path.join(tempfile.mkdtemp(), "google.json")
    if linked:
        with open(path, "w") as handle:
            json.dump({"client_id": "cid", "client_secret": "csecret", "refresh_token": "rtoken"}, handle)
    return gcal.GoogleCalendar(data_file=path, api=BASE + "/calendar/v3", token_url=BASE + "/token", clock=clock)


def error_of(function, *args):
    try:
        function(*args)
    except gcal.CalendarError as error:
        return error.code
    return None


def event_calls():
    return [call for call in calls if call[0] == "GET"]


def run_tests():
    reset()
    client = make_client()
    body = client.today(OFFSET)
    events = body["events"]
    check(body["ok"] and body["date"] == "2026-11-09" and body["age"] == 0, "today: ok, date in the device's offset, fresh")

    query = event_calls()[0][2]
    check(query["timeMin"] == "2026-11-08T18:30:00Z" and query["timeMax"] == "2026-11-09T18:30:00Z", "today: window is the device's local day in UTC")
    check(query["singleEvents"] == "true" and query["orderBy"] == "startTime", "today: recurring events are expanded and ordered")
    check(event_calls()[0][3] == "Bearer access-1", "today: bearer token sent")
    token_call = next(call for call in calls if call[0] == "POST")
    check(token_call[2]["grant_type"] == "refresh_token" and token_call[2]["refresh_token"] == "rtoken", "token: refresh grant with the stored token")

    titles = [e["t"] for e in events]
    check("Holiday" not in titles, "all-day events are skipped")
    check("Cancelled" not in titles, "cancelled events are skipped")
    check("Declined" not in titles and "Accepted" in titles, "events you declined are skipped, others kept")
    check(
        titles == ["Overnight in", "Standup", "Design review", "Cafe 'Sync'", "(no title)", "Accepted", "Overnight out"],
        "events are sorted by start, titles reduced to ASCII",
    )
    by_title = {e["t"]: e for e in events}
    check(by_title["Standup"] == {"s": 510, "e": 540, "t": "Standup"}, "minutes of day in the device's offset (08:30-09:00 = 510-540)")
    check(by_title["Overnight in"]["s"] == 0 and by_title["Overnight in"]["e"] == 60, "event from yesterday is clipped to midnight")
    check(by_title["Overnight out"]["s"] == 1380 and by_title["Overnight out"]["e"] == 1440, "event past midnight is clipped to 24:00")
    utc_event = next(e for e in events if e["s"] == 630)
    check(utc_event["e"] == 690 and utc_event["t"] == "Cafe 'Sync'", "UTC (Z) times convert to local minutes (05:00Z = 10:30)")
    check(by_title["(no title)"]["s"] == 720, "empty title -> (no title)")
    check(body["n"] == len(events) == 7, "n counts the events shown")

    before = len(event_calls())
    again = client.today(OFFSET)
    check(len(event_calls()) == before and again["events"] == events, "cached: no second request inside 5 minutes")

    later = [NOW]
    client2 = make_client(clock=lambda: later[0])
    client2.today(OFFSET)
    first_count = len(event_calls())
    later[0] = NOW + 100
    client2.today(OFFSET)
    check(len(event_calls()) == first_count, "cache holds before the interval")
    later[0] = NOW + 301
    client2.today(OFFSET)
    check(len(event_calls()) == first_count + 1, "cache expires after 5 minutes")
    state["events_status"] = 500
    later[0] = NOW + 700
    stale = client2.today(OFFSET)
    check(stale["ok"] and stale["age"] == 399 and stale["events"], "upstream failure serves the stale list with its age")
    later[0] = NOW + 86400
    check(error_of(client2.today, OFFSET) == "upstream", "stale events from a previous day are never served")

    reset()
    many = [item(f"E{i}", f"2026-11-09T{8 + i // 4:02d}:{(i % 4) * 15:02d}:00+05:30", f"2026-11-09T{8 + i // 4:02d}:{(i % 4) * 15 + 10:02d}:00+05:30") for i in range(20)]
    state["items"] = many
    body = make_client().today(OFFSET)
    check(len(body["events"]) == gcal.MAX_EVENTS == 16 and body["n"] == 20, "capped at 16 events, n reports the real count")
    check(all(len(e["t"]) <= gcal.TITLE_CHARS for e in body["events"]), "titles fit 27 characters")
    state["items"] = [item("x" * 80, "2026-11-09T10:00:00+05:30", "2026-11-09T11:00:00+05:30")]
    check(len(make_client().today(OFFSET)["events"][0]["t"]) == 27, "long title is cut to 27 characters")

    reset()
    check(error_of(make_client(linked=False).today, OFFSET) == "not_linked", "no google.json -> not_linked")
    state["token_status"] = 400
    check(error_of(make_client().today, OFFSET) == "not_linked", "revoked token (invalid_grant) -> not_linked")
    reset()
    state["token_status"] = 500
    check(error_of(make_client().today, OFFSET) == "upstream", "token endpoint failure -> upstream")
    reset()
    state["events_status"] = 429
    check(error_of(make_client().today, OFFSET) == "rate_limited", "429 -> rate_limited")
    reset()
    state["events_status"] = 500
    check(error_of(make_client().today, OFFSET) == "upstream", "events failure with nothing cached -> upstream")
    reset()
    state["fail_first_events"] = True
    client = make_client()
    body = client.today(OFFSET)
    check(body["ok"] and len([c for c in calls if c[0] == "POST"]) == 2, "401 from the API triggers one token refresh and a retry")

    for bad in (5000, -5000, "330", None):
        try:
            make_client().today(bad)
            check(False, f"bad offset {bad!r} rejected")
        except ValueError:
            check(True, f"bad offset {bad!r} rejected")

    reset()
    other = make_client().today(-480)
    check(other["date"] == "2026-11-09" and other["events"] != [], "a different offset gives its own local day")
    near_midnight = make_client(clock=lambda: datetime(2026, 11, 9, 19, 0, tzinfo=timezone.utc).timestamp()).today(OFFSET)
    check(near_midnight["date"] == "2026-11-10", "19:00 UTC is already tomorrow at +05:30")


def run_http_tests():
    reset()
    app.gcal_client = make_client()
    request = lambda path: _request(path)
    status, body = request("/calendar/today?off=330")
    check(status == 200 and body["ok"] and body["date"] == "2026-11-09" and len(body["events"]) == 7, "HTTP GET /calendar/today")
    check(request("/calendar/today")[0] == 400, "HTTP missing off -> 400")
    check(request("/calendar/today?off=abc")[0] == 400, "HTTP non-numeric off -> 400")
    check(request("/calendar/today?off=99999")[0] == 400, "HTTP out of range off -> 400")
    app.gcal_client = make_client(linked=False)
    status, body = request("/calendar/today?off=330")
    check(status == 200 and body == {"ok": False, "err": "not_linked"}, "HTTP not linked -> err code with 200")


def _request(path):
    try:
        with urlopen(Request(SERVER + path), timeout=10) as response:
            return response.status, json.load(response)
    except Exception as error:
        return getattr(error, "code", 0), None


if __name__ == "__main__":
    fake = ThreadingHTTPServer(("127.0.0.1", 0), Fake)
    BASE = f"http://127.0.0.1:{fake.server_address[1]}"
    threading.Thread(target=fake.serve_forever, daemon=True).start()
    bridge = ThreadingHTTPServer(("127.0.0.1", 0), app.Handler)
    SERVER = f"http://127.0.0.1:{bridge.server_address[1]}"
    threading.Thread(target=bridge.serve_forever, daemon=True).start()

    run_tests()
    run_http_tests()

    print("\nFAILED: " + ", ".join(failures) if failures else "\nall passed")
    sys.exit(1 if failures else 0)
