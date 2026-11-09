import json
import math
import os
import re
import threading
import time
import unicodedata
from collections import OrderedDict
from io import BytesIO
from urllib.error import HTTPError, URLError
from urllib.parse import urlencode, urlparse
from urllib.request import Request, urlopen

API = "https://api.spotify.com/v1"
ACCOUNTS = "https://accounts.spotify.com"
LRCLIB = "https://lrclib.net"
USER_AGENT = "DeskAssistantBridge/1.0 (personal desk display)"
DEFAULT_DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "spotify.json")

NOW_CACHE_SECONDS = 2.0
ALBUM_CACHE_SECONDS = 24 * 3600
MAX_TRACKS = 24
MAX_BLOCKS = 120
LINE_CHARS = 100
ROW_CHARS = 23
MAX_ROWS_PER_LINE = 4
SECONDS_PER_CHAR = 0.1
LAST_LINE_SECONDS = 5.0
TEXT_CHARS = 39
MAX_RETRY_AFTER = 120
COVER_SIZE = 88
COVER_BYTES = COVER_SIZE * COVER_SIZE * 2
COVER_CACHE = 8
COVER_SOURCE_WIDTH = 300
COVER_TUNING = (
    ("COVER_SATURATION", 1.2, 0.0, 3.0),
    ("COVER_CONTRAST", 1.225, 0.0, 3.0),
    ("COVER_GAMMA", 1.85, 0.3, 3.0),
    ("COVER_GAMMA_G", 1.3, 0.3, 3.0),
    ("COVER_GAIN_R", 0.8, 0.0, 2.0),
    ("COVER_GAIN_G", 1.1, 0.0, 2.0),
    ("COVER_GAIN_B", 1.0, 0.0, 2.0),
)
TUNING_FILE = "cover_tuning.json"
MAX_PREVIEW_SCALE = 8
MAX_IMAGE_BYTES = 2_000_000
ID_PATTERN = re.compile(r"^[A-Za-z0-9]{10,32}$")
LRC_STAMP = re.compile(r"\[(\d{1,3}):(\d{2})(?:[.:](\d{1,3}))?\]")

PLAYER_COMMANDS = {
    "play": ("PUT", "/me/player/play"),
    "pause": ("PUT", "/me/player/pause"),
    "next": ("POST", "/me/player/next"),
    "prev": ("POST", "/me/player/previous"),
}

SYMBOLS = {"‘": "'", "’": "'", "“": '"', "”": '"', "–": "-", "—": "-", "…": "..."}


class SpotifyError(Exception):
    def __init__(self, code):
        super().__init__(code)
        self.code = code


def ascii_text(text, limit):
    for symbol, plain in SYMBOLS.items():
        text = text.replace(symbol, plain)
    text = unicodedata.normalize("NFKD", text).encode("ascii", "ignore").decode()
    text = " ".join(text.split())
    return text[:limit].rstrip()


def parse_lrc(text):
    lines = []
    for raw in text.splitlines():
        stamps = list(LRC_STAMP.finditer(raw))
        if not stamps:
            continue
        body = ascii_text(raw[stamps[-1].end():], LINE_CHARS)
        if not body:
            continue
        for stamp in stamps:
            fraction = float("0." + stamp.group(3)) if stamp.group(3) else 0.0
            lines.append((int(stamp.group(1)) * 60 + int(stamp.group(2)) + fraction, body))
    lines.sort(key=lambda line: line[0])
    return lines


def pick_image(images):
    sized = [image for image in images or [] if image.get("url") and image.get("width")]
    if sized:
        return min(sized, key=lambda image: abs(image["width"] - COVER_SOURCE_WIDTH))["url"]
    return next((image["url"] for image in images or [] if image.get("url")), None)


def cover_style(env=None):
    env = os.environ if env is None else env
    style = {}
    for name, default, low, high in COVER_TUNING:
        try:
            value = float(env.get(name) or default)
        except (TypeError, ValueError):
            value = default
        style[name] = min(high, max(low, value))
    return style


def merge_style(base, values):
    style = dict(base)
    if not isinstance(values, dict):
        return style
    for name, default, low, high in COVER_TUNING:
        value = values.get(name)
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
            continue
        style[name] = round(min(high, max(low, float(value))), 3)
    return style


def cover_table(style):
    table = []
    for gain, extra in ((style["COVER_GAIN_R"], 1.0), (style["COVER_GAIN_G"], style["COVER_GAMMA_G"]), (style["COVER_GAIN_B"], 1.0)):
        exponent = style["COVER_GAMMA"] * extra
        table += [min(255, max(0, int(round(255 * (i / 255) ** exponent * gain)))) for i in range(256)]
    return table


def correct_cover(image, style):
    from PIL import ImageEnhance

    image = ImageEnhance.Color(image).enhance(style["COVER_SATURATION"])
    image = ImageEnhance.Contrast(image).enhance(style["COVER_CONTRAST"])
    return image.point(cover_table(style))


def pack_rgb565(image):
    raw = image.tobytes()
    out = bytearray(COVER_BYTES)
    for pixel in range(COVER_SIZE * COVER_SIZE):
        r, g, b = raw[pixel * 3], raw[pixel * 3 + 1], raw[pixel * 3 + 2]
        value = ((r * 31 + 127) // 255) << 11 | ((g * 63 + 127) // 255) << 5 | (b * 31 + 127) // 255
        out[pixel * 2] = value >> 8
        out[pixel * 2 + 1] = value & 255
    return bytes(out)


def png_bytes(image):
    buffer = BytesIO()
    image.save(buffer, "PNG")
    return buffer.getvalue()


def render_cover(data, style):
    try:
        from PIL import Image, ImageOps

        original = ImageOps.fit(Image.open(BytesIO(data)).convert("RGB"), (COVER_SIZE, COVER_SIZE), Image.Resampling.LANCZOS)
        corrected = correct_cover(original, style)
        return {"raw": pack_rgb565(corrected), "png": png_bytes(corrected), "orig": png_bytes(original)}
    except Exception:
        raise SpotifyError("no_cover")


def wrap_text(text, width=ROW_CHARS):
    rows, current = [], ""
    for word in text.split():
        while len(word) > width:
            if current:
                rows.append(current)
                current = ""
            rows.append(word[:width])
            word = word[width:]
        if not current:
            current = word
        elif len(current) + 1 + len(word) <= width:
            current += " " + word
        else:
            rows.append(current)
            current = word
    if current:
        rows.append(current)
    if len(rows) > MAX_ROWS_PER_LINE:
        rows = rows[:MAX_ROWS_PER_LINE]
        rows[-1] = rows[-1][: ROW_CHARS - 2].rstrip() + ".."
    return rows


def lyric_rows(lines, duration=None):
    return [row for group in lyric_groups(lines, duration) for row in group]


def lyric_groups(lines, duration=None):
    groups = []
    for index, (start, text) in enumerate(lines):
        rows = []
        pieces = wrap_text(text)
        if index + 1 < len(lines):
            gap = lines[index + 1][0] - start
        elif duration and duration > start:
            gap = min(LAST_LINE_SECONDS, duration - start)
        else:
            gap = LAST_LINE_SECONDS
        total = sum(len(piece) for piece in pieces)
        span = min(gap, total * SECONDS_PER_CHAR)
        before = 0
        for piece in pieces:
            rows.append((start + span * before / total, piece))
            before += len(piece)
        groups.append(rows)
    return groups


def pair_blocks(lines, duration=None):
    blocks = []
    for rows in lyric_groups(lines, duration):
        for index in range(0, len(rows), 2):
            second = rows[index + 1][1] if index + 1 < len(rows) else ""
            blocks.append({"ms": int(round(rows[index][0] * 1000)), "a": rows[index][1], "b": second})
            if len(blocks) == MAX_BLOCKS:
                return blocks
    return blocks


class Spotify:
    def __init__(self, client_id=None, data_file=None, api=None, accounts=None, lrclib=None, style=None):
        self.client_id = client_id if client_id is not None else os.environ.get("SPOTIFY_CLIENT_ID", "")
        self.data_file = data_file or os.environ.get("SPOTIFY_DATA") or DEFAULT_DATA
        self.api = api or os.environ.get("SPOTIFY_API_BASE") or API
        self.accounts = accounts or os.environ.get("SPOTIFY_ACCOUNTS_BASE") or ACCOUNTS
        self.lrclib = lrclib or os.environ.get("LRCLIB_BASE") or LRCLIB
        self._tuning_file = os.path.join(os.path.dirname(self.data_file), TUNING_FILE)
        self.tune_rev = 0
        self.tune_saved = True
        if style:
            self.style = style
        else:
            self.style = self._saved_style(cover_style())

        self._token_lock = threading.RLock()
        self._now_lock = threading.Lock()
        self._refresh = None
        self._access = None
        self._expires = 0.0
        self._blocked_until = 0.0
        self._now = None
        self._now_at = 0.0
        self._meta = {}
        self._albums = {}
        self._lyrics = {}
        self._cover_urls = {}
        self._covers = OrderedDict()
        self._raw_images = OrderedDict()
        self._cover_lock = threading.Lock()
        self._load()

    def _load(self):
        try:
            with open(self.data_file) as handle:
                stored = json.load(handle)
        except (OSError, ValueError):
            return
        self._refresh = stored.get("refresh_token") or None
        if stored.get("client_id") and not self.client_id:
            self.client_id = stored["client_id"]

    def _save(self):
        os.makedirs(os.path.dirname(self.data_file), exist_ok=True)
        temp = self.data_file + ".tmp"
        with open(temp, "w") as handle:
            json.dump({"client_id": self.client_id, "refresh_token": self._refresh}, handle)
        os.replace(temp, self.data_file)

    def _token(self):
        with self._token_lock:
            if not self._refresh or not self.client_id:
                raise SpotifyError("not_linked")
            if self._access and time.time() < self._expires - 30:
                return self._access
            body = urlencode(
                {"grant_type": "refresh_token", "refresh_token": self._refresh, "client_id": self.client_id}
            ).encode()
            request = Request(
                self.accounts + "/api/token",
                data=body,
                headers={"Content-Type": "application/x-www-form-urlencoded", "User-Agent": USER_AGENT},
            )
            try:
                with urlopen(request, timeout=10) as response:
                    granted = json.load(response)
            except HTTPError as error:
                if error.code in (400, 401):
                    self._access = None
                    raise SpotifyError("not_linked")
                raise SpotifyError("upstream")
            except (URLError, OSError, ValueError):
                raise SpotifyError("upstream")
            self._access = granted["access_token"]
            self._expires = time.time() + granted.get("expires_in", 3600)
            rotated = granted.get("refresh_token")
            if rotated and rotated != self._refresh:
                self._refresh = rotated
                self._save()
            return self._access

    def _call(self, method, path, body=None, retried=False):
        if time.time() < self._blocked_until:
            raise SpotifyError("rate_limited")
        headers = {"Authorization": "Bearer " + self._token(), "User-Agent": USER_AGENT}
        data = None
        if body is not None:
            data = json.dumps(body).encode()
            headers["Content-Type"] = "application/json"
        elif method in ("PUT", "POST"):
            data = b""
        request = Request(self.api + path, data=data, headers=headers, method=method)
        retry_after = None
        try:
            with urlopen(request, timeout=10) as response:
                status, raw = response.status, response.read()
        except HTTPError as error:
            status, raw, retry_after = error.code, error.read(), error.headers.get("Retry-After")
        except (URLError, OSError):
            raise SpotifyError("upstream")

        if status == 401 and not retried:
            with self._token_lock:
                self._access = None
            return self._call(method, path, body, True)
        if status == 429:
            try:
                wait = min(int(retry_after), MAX_RETRY_AFTER)
            except (TypeError, ValueError):
                wait = 5
            self._blocked_until = time.time() + wait
            raise SpotifyError("rate_limited")
        try:
            payload = json.loads(raw) if raw else None
        except ValueError:
            payload = None
        return status, payload

    @staticmethod
    def _failure(status, payload):
        error = payload.get("error") if isinstance(payload, dict) else None
        reason = error.get("reason", "") if isinstance(error, dict) else ""
        if status == 401:
            return SpotifyError("not_linked")
        if reason == "NO_ACTIVE_DEVICE":
            return SpotifyError("no_device")
        if reason == "PREMIUM_REQUIRED":
            return SpotifyError("premium_required")
        return SpotifyError("upstream")

    def _snapshot(self, status, payload):
        if status == 204 or payload is None:
            return {"err": "no_device"}
        if status != 200:
            raise self._failure(status, payload)
        item = payload.get("item")
        if not item or item.get("type", "track") != "track":
            return {"idle": True}
        artists = [artist.get("name", "") for artist in item.get("artists", [])]
        album = item.get("album", {})
        self._meta[item["id"]] = {
            "title": item.get("name", ""),
            "artist": artists[0] if artists else "",
            "album": album.get("name", ""),
            "duration": item.get("duration_ms", 0) / 1000,
        }
        if len(self._meta) > 50:
            self._meta.pop(next(iter(self._meta)))
        self._note_cover(album.get("id"), album.get("images"))
        return {
            "playing": bool(payload.get("is_playing")),
            "id": item["id"],
            "title": ascii_text(item.get("name", ""), TEXT_CHARS) or "?",
            "artist": ascii_text(", ".join(artists), TEXT_CHARS) or "?",
            "albumId": album.get("id", ""),
            "album": ascii_text(album.get("name", ""), TEXT_CHARS) or "?",
            "track": item.get("track_number", 0),
            "pos": payload.get("progress_ms") or 0,
            "dur": item.get("duration_ms", 0),
        }

    def _render_now(self):
        snap = self._now
        if "err" in snap:
            return {"ok": False, "err": snap["err"]}
        if "idle" in snap:
            return {"ok": True, "idle": True}
        out = dict(snap, ok=True, cv=self.tune_rev)
        if snap["playing"]:
            out["pos"] = min(snap["dur"], snap["pos"] + int((time.time() - self._now_at) * 1000))
        return out

    def now(self):
        with self._now_lock:
            if self._now is None or time.time() - self._now_at >= NOW_CACHE_SECONDS:
                try:
                    status, payload = self._call("GET", "/me/player")
                    snap = self._snapshot(status, payload)
                except SpotifyError as error:
                    if error.code == "rate_limited" and self._now is not None:
                        return self._render_now()
                    raise
                self._now, self._now_at = snap, time.time()
            return self._render_now()

    def _forget_now(self):
        with self._now_lock:
            self._now = None

    def album(self, album_id):
        if not ID_PATTERN.match(album_id):
            raise ValueError("bad id")
        cached = self._albums.get(album_id)
        if cached and time.time() - cached[0] < ALBUM_CACHE_SECONDS:
            return cached[1]
        status, payload = self._call("GET", "/albums/" + album_id)
        if status != 200 or not isinstance(payload, dict):
            raise self._failure(status, payload)
        self._note_cover(album_id, payload.get("images"))
        items = payload.get("tracks", {}).get("items", [])
        artists = [artist.get("name", "") for artist in payload.get("artists", [])]
        result = {
            "ok": True,
            "id": album_id,
            "title": ascii_text(payload.get("name", ""), TEXT_CHARS) or "?",
            "artist": ascii_text(", ".join(artists), TEXT_CHARS) or "?",
            "year": (payload.get("release_date") or "")[:4],
            "total": payload.get("total_tracks", len(items)),
            "tracks": [
                {"id": track["id"], "t": ascii_text(track.get("name", ""), 27) or "?", "ms": track.get("duration_ms", 0)}
                for track in items[:MAX_TRACKS]
            ],
        }
        self._albums[album_id] = (time.time(), result)
        return result

    def _note_cover(self, album_id, images):
        url = pick_image(images)
        if album_id and url:
            self._cover_urls[album_id] = url
            if len(self._cover_urls) > 50:
                self._cover_urls.pop(next(iter(self._cover_urls)))

    def _download(self, url):
        if urlparse(url).scheme not in ("http", "https"):
            raise SpotifyError("no_cover")
        try:
            with urlopen(Request(url, headers={"User-Agent": USER_AGENT}), timeout=10) as response:
                data = response.read(MAX_IMAGE_BYTES + 1)
        except (URLError, OSError):
            raise SpotifyError("upstream")
        if len(data) > MAX_IMAGE_BYTES:
            raise SpotifyError("no_cover")
        return data

    def _cover_entry(self, album_id):
        if not ID_PATTERN.match(album_id):
            raise ValueError("bad id")
        with self._cover_lock:
            cached = self._covers.get(album_id)
            if cached is not None:
                self._covers.move_to_end(album_id)
                return cached
            if album_id not in self._cover_urls:
                self.album(album_id)
            if album_id not in self._cover_urls:
                raise SpotifyError("no_cover")
            image = self._raw_images.get(album_id)
            if image is None:
                image = self._download(self._cover_urls[album_id])
            entry = render_cover(image, self.style)
            self._raw_images[album_id] = image
            self._covers[album_id] = entry
            for cache in (self._raw_images, self._covers):
                while len(cache) > COVER_CACHE:
                    cache.popitem(last=False)
            return entry

    def _saved_style(self, base):
        try:
            with open(self._tuning_file) as handle:
                return merge_style(base, json.load(handle))
        except (OSError, ValueError):
            return base

    def _store_style(self):
        try:
            os.makedirs(os.path.dirname(self._tuning_file), exist_ok=True)
            temp = self._tuning_file + ".tmp"
            with open(temp, "w") as handle:
                json.dump(self.style, handle)
            os.replace(temp, self._tuning_file)
            self.tune_saved = True
        except OSError:
            self.tune_saved = False

    def _restyled(self, style):
        self.style = style
        self._covers.clear()
        self.tune_rev += 1

    def tune(self, values):
        with self._cover_lock:
            self._restyled(merge_style(self.style, values))
            self._store_style()
        return self.tuning_state()

    def reset_tuning(self):
        with self._cover_lock:
            self._restyled(cover_style())
            try:
                os.remove(self._tuning_file)
                self.tune_saved = True
            except FileNotFoundError:
                self.tune_saved = True
            except OSError:
                self.tune_saved = False
        return self.tuning_state()

    def tuning_state(self):
        snap = self._now or {}
        album = snap.get("albumId") or next(reversed(self._cover_urls), "")
        return {
            "ok": True,
            "rev": self.tune_rev,
            "saved": self.tune_saved,
            "album": album,
            "params": [
                {"name": name, "value": self.style[name], "default": default, "low": low, "high": high}
                for name, default, low, high in COVER_TUNING
            ],
        }

    def cover(self, album_id):
        return self._cover_entry(album_id)["raw"]

    def cover_png(self, album_id, corrected=True, scale=1):
        data = self._cover_entry(album_id)["png" if corrected else "orig"]
        if scale <= 1:
            return data
        try:
            from PIL import Image

            image = Image.open(BytesIO(data))
            out = BytesIO()
            image.resize((COVER_SIZE * scale, COVER_SIZE * scale), Image.Resampling.NEAREST).save(out, "PNG")
            return out.getvalue()
        except Exception:
            raise SpotifyError("no_cover")

    def control(self, action):
        if action not in PLAYER_COMMANDS:
            raise ValueError("bad action")
        method, path = PLAYER_COMMANDS[action]
        return self._command(method, path, None)

    def play(self, album_id, index):
        if not ID_PATTERN.match(album_id) or not isinstance(index, int) or not 0 <= index < 200:
            raise ValueError("bad play request")
        body = {"context_uri": "spotify:album:" + album_id, "offset": {"position": index}}
        return self._command("PUT", "/me/player/play", body)

    def _command(self, method, path, body):
        status, payload = self._call(method, path, body)
        self._forget_now()
        if status not in (200, 202, 204):
            raise self._failure(status, payload)
        return {"ok": True}

    def lyrics(self, track_id, title=None, artist=None, album=None, seconds=None):
        if not ID_PATTERN.match(track_id):
            raise ValueError("bad id")
        if track_id in self._lyrics:
            return self._lyrics[track_id]
        if title and artist:
            meta = {"title": title, "artist": artist, "album": album or "", "duration": seconds}
        else:
            meta = self._meta.get(track_id)
            if meta is None:
                self.now()
                meta = self._meta.get(track_id)
            if meta is None:
                raise SpotifyError("upstream")
        result = {"ok": True, "id": track_id, "blocks": pair_blocks(self._lookup_lyrics(meta), meta.get("duration"))}
        self._lyrics[track_id] = result
        if len(self._lyrics) > 50:
            self._lyrics.pop(next(iter(self._lyrics)))
        return result

    def _lrclib(self, path, query):
        request = Request(self.lrclib + path + "?" + urlencode(query), headers={"User-Agent": USER_AGENT})
        try:
            with urlopen(request, timeout=8) as response:
                return json.load(response)
        except HTTPError as error:
            if error.code == 404:
                return None
            raise SpotifyError("upstream")
        except (URLError, OSError, ValueError):
            raise SpotifyError("upstream")

    def _lookup_lyrics(self, meta):
        query = {"track_name": meta["title"], "artist_name": meta["artist"]}
        if meta.get("album"):
            query["album_name"] = meta["album"]
        if meta.get("duration"):
            query["duration"] = int(round(meta["duration"]))
        found = self._lrclib("/api/get", query)
        if not found:
            candidates = self._lrclib("/api/search", {"track_name": meta["title"], "artist_name": meta["artist"]}) or []
            wanted = meta.get("duration")
            found = next(
                (
                    entry
                    for entry in candidates
                    if entry.get("syncedLyrics") and (not wanted or abs(entry.get("duration", 0) - wanted) <= 5)
                ),
                None,
            )
        if not found or found.get("instrumental") or not found.get("syncedLyrics"):
            return []
        return parse_lrc(found["syncedLyrics"])
