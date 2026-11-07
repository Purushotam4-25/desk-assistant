import json
import os
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.error import URLError
from urllib.parse import parse_qs, urlencode, urlparse
from urllib.request import urlopen

PORT = int(os.environ.get("PORT", "8765"))
CACHE_SECONDS = int(os.environ.get("CACHE_MINUTES", "15")) * 60
UPSTREAM = "https://api.open-meteo.com/v1/forecast"

CLEAR, MOSTLY_CLEAR, CLOUDY, RAIN = 0, 1, 2, 3

CODES = {
    0: (CLEAR, "Clear"),
    1: (MOSTLY_CLEAR, "Mostly clear"),
    2: (MOSTLY_CLEAR, "Partly cloudy"),
    3: (CLOUDY, "Cloudy"),
    45: (CLOUDY, "Fog"),
    48: (CLOUDY, "Freezing fog"),
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
    80: (RAIN, "Light showers"),
    81: (RAIN, "Showers"),
    82: (RAIN, "Heavy showers"),
    85: (CLOUDY, "Snow showers"),
    86: (CLOUDY, "Snow showers"),
    95: (RAIN, "Thunderstorm"),
    96: (RAIN, "Thunderstorm"),
    99: (RAIN, "Thunderstorm"),
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


class Handler(BaseHTTPRequestHandler):
    def reply(self, status, body):
        payload = json.dumps(body, separators=(",", ":")).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self):
        url = urlparse(self.path)
        if url.path == "/health":
            self.reply(200, {"ok": True})
            return
        if url.path != "/weather":
            self.reply(404, {"error": "not found"})
            return
        args = parse_qs(url.query)
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
    ThreadingHTTPServer(("0.0.0.0", PORT), Handler).serve_forever()
