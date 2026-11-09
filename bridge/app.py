import json
import os
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.error import URLError
from urllib.parse import parse_qs, urlencode, urlparse
from urllib.request import urlopen

import spotify
from tune_page import PAGE

PORT = int(os.environ.get("PORT", "8765"))
CACHE_SECONDS = int(os.environ.get("CACHE_MINUTES", "15")) * 60
UPSTREAM = "https://api.open-meteo.com/v1/forecast"

CLEAR, MOSTLY_CLEAR, CLOUDY, RAIN, FOG, SHOWERS, STORM = 0, 1, 2, 3, 4, 5, 6

CODES = {
    0: (CLEAR, "Clear"),
    1: (MOSTLY_CLEAR, "Mostly clear"),
    2: (MOSTLY_CLEAR, "Partly cloudy"),
    3: (CLOUDY, "Cloudy"),
    45: (FOG, "Fog"),
    48: (FOG, "Freezing fog"),
    51: (RAIN, "Light drizzle"),
    53: (RAIN, "Drizzle"),
    55: (RAIN, "Heavy drizzle"),
    56: (RAIN, "Freezing drizzle"),
    57: (RAIN, "Freezing drizzle"),
    61: (RAIN, "Light rain"),
    63: (RAIN, "Rain"),
    65: (RAIN, "Heavy rain"),
    66: (RAIN, "Freezing rain"),
    67: (RAIN, "Freezing rain"),
    71: (CLOUDY, "Light snow"),
    73: (CLOUDY, "Snow"),
    75: (CLOUDY, "Heavy snow"),
    77: (CLOUDY, "Snow grains"),
    80: (SHOWERS, "Light showers"),
    81: (SHOWERS, "Showers"),
    82: (SHOWERS, "Heavy showers"),
    85: (CLOUDY, "Snow showers"),
    86: (CLOUDY, "Snow showers"),
    95: (STORM, "Thunderstorm"),
    96: (STORM, "Thunderstorm"),
    99: (STORM, "Thunderstorm"),
}

cache = {}
lock = threading.Lock()


def condition(code):
    return CODES.get(code, (CLOUDY, "Unknown"))


def whole(value, default=0):
    return default if value is None else int(round(value))


def reshape(raw):
    current = raw["current"]
    daily = raw["daily"]
    cond, summary = condition(current["weather_code"])
    forecast = []
    for day in (1, 2, 3):
        forecast.append(
            {
                "c": condition(daily["weather_code"][day])[0],
                "hi": whole(daily["temperature_2m_max"][day]),
                "lo": whole(daily["temperature_2m_min"][day]),
            }
        )
    return {
        "cond": cond,
        "sum": summary,
        "temp": whole(current["temperature_2m"]),
        "hum": whole(current["relative_humidity_2m"]),
        "hi": whole(daily["temperature_2m_max"][0]),
        "lo": whole(daily["temperature_2m_min"][0]),
        "uv": whole(daily["uv_index_max"][0]),
        "rain": whole(daily["precipitation_probability_max"][0]),
        "fc": forecast,
    }


def fetch(lat, lon):
    query = urlencode(
        {
            "latitude": lat,
            "longitude": lon,
            "timezone": "auto",
            "forecast_days": 4,
            "current": "temperature_2m,relative_humidity_2m,weather_code,is_day",
            "daily": "weather_code,temperature_2m_max,temperature_2m_min,uv_index_max,precipitation_probability_max",
        }
    )
    with urlopen(f"{UPSTREAM}?{query}", timeout=10) as response:
        return reshape(json.load(response))


def weather(lat, lon):
    key = (round(lat, 2), round(lon, 2))
    with lock:
        entry = cache.get(key)
        if entry and time.time() - entry["at"] < CACHE_SECONDS:
            return {**entry["data"], "age": int(time.time() - entry["at"])}
        try:
            data = fetch(*key)
        except (URLError, OSError, KeyError, IndexError, ValueError) as error:
            print(f"upstream failed: {error}", flush=True)
            if entry:
                return {**entry["data"], "age": int(time.time() - entry["at"])}
            raise
        cache[key] = {"data": data, "at": time.time()}
        return {**data, "age": 0}


spotify_client = spotify.Spotify()
MAX_BODY_BYTES = 1024


class Handler(BaseHTTPRequestHandler):
    def reply(self, status, body):
        payload = json.dumps(body, separators=(",", ":")).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def spotify_reply(self, action):
        try:
            self.reply(200, action())
        except ValueError:
            self.reply(400, {"error": "bad request"})
        except spotify.SpotifyError as error:
            self.reply(200, {"ok": False, "err": error.code})
        except Exception as error:
            print(f"spotify failed: {error}", flush=True)
            self.reply(200, {"ok": False, "err": "upstream"})

    def cover_reply(self, args):
        preview = args.get("fmt", [""])[0]
        try:
            if preview in ("png", "orig"):
                scale = max(1, min(spotify.MAX_PREVIEW_SCALE, int(args.get("scale", ["1"])[0])))
                pixels = spotify_client.cover_png(args.get("id", [""])[0], preview == "png", scale)
            else:
                pixels = spotify_client.cover(args.get("id", [""])[0])
        except ValueError:
            self.reply(400, {"error": "bad request"})
            return
        except spotify.SpotifyError as error:
            self.reply(200, {"ok": False, "err": error.code})
            return
        except Exception as error:
            print(f"spotify cover failed: {error}", flush=True)
            self.reply(200, {"ok": False, "err": "upstream"})
            return
        self.send_response(200)
        self.send_header("Content-Type", "image/png" if preview in ("png", "orig") else "application/octet-stream")
        self.send_header("Content-Length", str(len(pixels)))
        self.end_headers()
        self.wfile.write(pixels)

    def page_reply(self, html):
        payload = html.encode()
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(payload)

    def tune(self, body):
        if body.get("reset") is True:
            return spotify_client.reset_tuning()
        return spotify_client.tune(body)

    def read_json(self):
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if not 0 < length <= MAX_BODY_BYTES:
                raise ValueError
            body = json.loads(self.rfile.read(length))
            if not isinstance(body, dict):
                raise ValueError
            return body
        except ValueError:
            return None

    def do_GET(self):
        url = urlparse(self.path)
        args = parse_qs(url.query)
        if url.path == "/health":
            self.reply(200, {"ok": True})
        elif url.path == "/weather":
            self.weather_reply(args)
        elif url.path == "/spotify/now":
            self.spotify_reply(spotify_client.now)
        elif url.path == "/spotify/album":
            self.spotify_reply(lambda: spotify_client.album(args.get("id", [""])[0]))
        elif url.path == "/spotify/lyrics":
            self.spotify_reply(lambda: self.lyrics(args))
        elif url.path == "/spotify/cover":
            self.cover_reply(args)
        elif url.path == "/spotify/tune":
            self.page_reply(PAGE)
        elif url.path == "/spotify/tune/state":
            self.spotify_reply(lambda: spotify_client.tuning_state())
        else:
            self.reply(404, {"error": "not found"})

    def do_POST(self):
        url = urlparse(self.path)
        body = self.read_json()
        if url.path not in ("/spotify/control", "/spotify/play", "/spotify/tune"):
            self.reply(404, {"error": "not found"})
        elif body is None:
            self.reply(400, {"error": "bad request"})
        elif url.path == "/spotify/tune":
            self.spotify_reply(lambda: self.tune(body))
        elif url.path == "/spotify/control":
            self.spotify_reply(lambda: spotify_client.control(str(body.get("action", ""))))
        else:
            self.spotify_reply(lambda: spotify_client.play(str(body.get("album", "")), body.get("index")))

    def lyrics(self, args):
        first = lambda name: args.get(name, [None])[0]
        milliseconds = first("ms")
        return spotify_client.lyrics(
            first("id") or "",
            first("title"),
            first("artist"),
            first("album"),
            int(milliseconds) / 1000 if milliseconds else None,
        )

    def weather_reply(self, args):
        try:
            lat = float(args["lat"][0])
            lon = float(args["lon"][0])
            if not (-90 <= lat <= 90 and -180 <= lon <= 180):
                raise ValueError
        except (KeyError, ValueError):
            self.reply(400, {"error": "lat and lon required"})
            return
        try:
            self.reply(200, weather(lat, lon))
        except Exception:
            self.reply(502, {"error": "upstream failed"})

    def log_message(self, format, *args):
        print(f"{self.address_string()} {format % args}", flush=True)


if __name__ == "__main__":
    print(f"bridge listening on {PORT}", flush=True)
    print("cover tuning " + " ".join(f"{name}={value}" for name, value in spotify_client.style.items()), flush=True)
    ThreadingHTTPServer(("0.0.0.0", PORT), Handler).serve_forever()
