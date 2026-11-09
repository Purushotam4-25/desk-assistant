import io
import json
import os
import sys
import tempfile
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.request import Request, urlopen

import app
import spotify

state = {}
calls = []


def reset():
    calls.clear()
    state.clear()
    state.update(
        {
            "player": (200, {"is_playing": True, "progress_ms": 10000, "item": TRACK}),
            "command": (204, None),
            "rotate": False,
            "reject_first_api": False,
            "token_status": 200,
            "get": (200, {"syncedLyrics": SYNCED, "instrumental": False, "duration": 200}),
            "search": (200, []),
            "images": True,
            "image_status": 200,
            "image_bytes": None,
        }
    )


TRACK = {
    "id": "TRACKID12345",
    "type": "track",
    "name": "Café ’Test’ हिन्दी",
    "duration_ms": 200000,
    "track_number": 3,
    "artists": [{"name": "Artist One"}, {"name": "Artist Two"}],
    "album": {"id": "ALBUMID12345", "name": "The Album"},
}

ORANGE = (255, 128, 0)
BLUE = (0, 0, 255)


def test_image(fmt="PNG"):
    from PIL import Image

    image = Image.new("RGB", (300, 200), ORANGE)
    image.paste(BLUE, (150, 0, 300, 200))
    out = io.BytesIO()
    image.save(out, fmt)
    return out.getvalue()

SYNCED = "[ar: Someone]\n[00:05.00] line one\n[00:10.50] line two\n[00:15.00] \n[00:20.00][00:40.00] line three\n[00:30.25] हिन्दी\n[00:35.00] line four"


class Fake(BaseHTTPRequestHandler):
    def send(self, status, body=None):
        raw = json.dumps(body).encode() if body is not None else b""
        self.send_response(status)
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)

    def record(self):
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length) if length else b""
        calls.append((self.command, self.path, self.headers.get("Authorization"), body))
        return body

    def do_POST(self):
        body = self.record()
        if self.path == "/api/token":
            if state["token_status"] != 200:
                return self.send(state["token_status"], {"error": "invalid_grant"})
            count = sum(1 for call in calls if call[1] == "/api/token")
            reply = {"access_token": f"access{count}", "expires_in": 3600}
            if state["rotate"]:
                reply["refresh_token"] = "rotated-refresh"
            return self.send(200, reply)
        self.send(*state["command"])

    do_PUT = do_POST

    def do_GET(self):
        self.record()
        if self.path.startswith("/img/"):
            if state["image_status"] != 200:
                return self.send(state["image_status"], None)
            raw = state["image_bytes"] or test_image()
            self.send_response(200)
            self.send_header("Content-Type", "image/png")
            self.send_header("Content-Length", str(len(raw)))
            self.end_headers()
            return self.wfile.write(raw)
        if self.path.startswith("/v1/me/player"):
            if state["reject_first_api"]:
                state["reject_first_api"] = False
                return self.send(401, {"error": {"status": 401}})
            return self.send(*state["player"])
        if self.path.startswith("/v1/albums/"):
            items = [{"id": f"TRK{i:09d}", "name": f"Song {i}", "duration_ms": 180000 + i} for i in range(30)]
            images = (
                [
                    {"url": BASE + "/img/big.png", "width": 640, "height": 640},
                    {"url": BASE + "/img/mid.png", "width": 300, "height": 300},
                    {"url": BASE + "/img/small.png", "width": 64, "height": 64},
                ]
                if state["images"]
                else []
            )
            return self.send(
                200,
                {
                    "name": "The Album",
                    "artists": [{"name": "Artist One"}],
                    "release_date": "2021-05-07",
                    "total_tracks": 30,
                    "tracks": {"items": items},
                    "images": images,
                },
            )
        if self.path.startswith("/api/get"):
            return self.send(*state["get"])
        if self.path.startswith("/api/search"):
            return self.send(*state["search"])
        self.send(404, None)

    def log_message(self, *args):
        pass


IDENTITY = spotify.cover_style({name: "1" for name, *_ in spotify.COVER_TUNING})


def make_client(refresh="refresh0", data_file=None, style=IDENTITY):
    path = data_file or os.path.join(tempfile.mkdtemp(), "spotify.json")
    if refresh:
        with open(path, "w") as handle:
            json.dump({"client_id": "cid", "refresh_token": refresh}, handle)
    return spotify.Spotify(
        client_id="cid", data_file=path, api=BASE + "/v1", accounts=BASE, lrclib=BASE, style=style
    ), path


def error_of(function, *args):
    try:
        function(*args)
    except spotify.SpotifyError as error:
        return error.code
    return None


def api_calls():
    return [call for call in calls if call[1].startswith("/v1/")]


def check(condition, label):
    print(("ok   " if condition else "FAIL ") + label)
    if not condition:
        failures.append(label)


def run_fake_tests():
    reset()
    client, _ = make_client(refresh=None)
    check(error_of(client.now) == "not_linked", "no token file -> not_linked")

    reset()
    client, path = make_client()
    now = client.now()
    check(now["ok"] and now["playing"] and now["id"] == "TRACKID12345", "now maps the playing track")
    check(now["title"] == "Cafe 'Test'" and now["artist"] == "Artist One, Artist Two", "text reduced to ASCII")
    check(now["albumId"] == "ALBUMID12345" and now["track"] == 3 and now["dur"] == 200000, "album id, track number, duration")
    check(api_calls()[0][2] == "Bearer access1", "bearer token from refresh")
    client.now()
    check(len(api_calls()) == 1, "second now within 2 s served from cache")
    time.sleep(0.5)
    advanced = client.now()["pos"]
    check(advanced >= 10400, "position advances with cache age")

    reset()
    client, path = make_client()
    state["rotate"] = True
    client.now()
    stored = json.load(open(path))
    check(stored["refresh_token"] == "rotated-refresh", "rotated refresh token saved")

    reset()
    client, _ = make_client()
    state["reject_first_api"] = True
    check(client.now()["ok"], "401 triggers a token refresh and retry")
    check(sum(1 for call in calls if call[1] == "/api/token") == 2, "two token requests after the 401")

    reset()
    client, _ = make_client()
    state["token_status"] = 400
    check(error_of(client.now) == "not_linked", "revoked refresh token -> not_linked")

    reset()
    client, _ = make_client()
    state["player"] = (204, None)
    check(client.now() == {"ok": False, "err": "no_device"}, "204 -> no_device")

    reset()
    client, _ = make_client()
    state["player"] = (200, {"is_playing": False, "item": None})
    check(client.now() == {"ok": True, "idle": True}, "no item -> idle")

    reset()
    client, _ = make_client()
    state["player"] = (200, {"is_playing": True, "item": dict(TRACK, type="episode")})
    check(client.now() == {"ok": True, "idle": True}, "episode -> idle")

    reset()
    client, _ = make_client()
    state["player"] = (429, None)
    check(error_of(client.now) == "rate_limited", "429 -> rate_limited")
    n = len(api_calls())
    check(error_of(client.now) == "rate_limited" and len(api_calls()) == n, "blocked while rate limited, no new call")

    reset()
    client, _ = make_client()
    for action, method, path in [("next", "POST", "/v1/me/player/next"), ("prev", "POST", "/v1/me/player/previous"),
                                 ("pause", "PUT", "/v1/me/player/pause"), ("play", "PUT", "/v1/me/player/play")]:
        calls.clear()
        result = client.control(action)
        sent = api_calls()[0]
        check(result == {"ok": True} and sent[0] == method and sent[1] == path, f"control {action} -> {method} {path}")
    state["command"] = (404, {"error": {"status": 404, "reason": "NO_ACTIVE_DEVICE"}})
    check(error_of(client.control, "next") == "no_device", "NO_ACTIVE_DEVICE -> no_device")
    state["command"] = (403, {"error": {"status": 403, "reason": "PREMIUM_REQUIRED"}})
    check(error_of(client.control, "next") == "premium_required", "PREMIUM_REQUIRED -> premium_required")
    state["command"] = (500, None)
    check(error_of(client.control, "next") == "upstream", "other failure -> upstream")
    try:
        client.control("explode")
        check(False, "unknown action rejected")
    except ValueError:
        check(True, "unknown action rejected")

    reset()
    client, _ = make_client()
    calls.clear()
    client.play("ALBUMID12345", 4)
    sent = api_calls()[0]
    body = json.loads(sent[3])
    check(sent[0] == "PUT" and sent[1] == "/v1/me/player/play", "play uses PUT /me/player/play")
    check(body == {"context_uri": "spotify:album:ALBUMID12345", "offset": {"position": 4}}, "play body has album and position")
    try:
        client.play("../etc", 1)
        check(False, "bad album id rejected")
    except ValueError:
        check(True, "bad album id rejected")

    reset()
    client, _ = make_client()
    album = client.album("ALBUMID12345")
    check(album["title"] == "The Album" and album["year"] == "2021" and album["total"] == 30, "album details")
    check(len(album["tracks"]) == spotify.MAX_TRACKS and album["tracks"][0]["t"] == "Song 0", "album tracks capped at 24")
    before = len(api_calls())
    client.album("ALBUMID12345")
    check(len(api_calls()) == before, "album cached")

    reset()
    client, _ = make_client()
    client.now()
    lyrics = client.lyrics("TRACKID12345")
    blocks = lyrics["blocks"]
    check(len(blocks) == 5, "lyrics: 5 usable lines -> one block per line")
    capped = spotify.parse_lrc("[00:01.00] " + "word " * 40)
    check(len(capped[0][1]) <= spotify.LINE_CHARS == 100, "lyric lines keep up to 100 characters before wrapping")
    check(blocks[0] == {"ms": 5000, "a": "line one", "b": ""}, "lyrics: first block is its own line, time in ms")
    check(blocks[1] == {"ms": 10500, "a": "line two", "b": ""}, "lyrics: block starts at the line's own stamp")
    check([b["ms"] for b in blocks] == [5000, 10500, 20000, 35000, 40000], "lyrics: empty and non-ASCII lines dropped")
    check(blocks[4] == {"ms": 40000, "a": "line three", "b": ""}, "lyrics: repeated stamps expand")
    lrclib_query = [c[1] for c in calls if c[1].startswith("/api/get")][0]
    check("duration=200" in lrclib_query and "track_name=" in lrclib_query, "lyrics lookup sends duration and names")
    before = len(calls)
    client.lyrics("TRACKID12345")
    check(len(calls) == before, "lyrics cached by track id")

    reset()
    client, _ = make_client()
    state["get"] = (404, None)
    state["search"] = (200, [{"syncedLyrics": SYNCED, "duration": 400}, {"syncedLyrics": SYNCED, "duration": 201}])
    result = client.lyrics("TRACKID12345", "T", "A", "B", 200)
    check(len(result["blocks"]) == 5, "lyrics search fallback picks the entry within 5 s")

    reset()
    client, _ = make_client()
    state["get"] = (404, None)
    check(client.lyrics("TRACKID12345", "T", "A", "B", 200)["blocks"] == [], "no lyrics -> empty list")

    reset()
    client, _ = make_client()
    state["get"] = (200, {"instrumental": True, "syncedLyrics": None})
    check(client.lyrics("TRACKID12345", "T", "A", "B", 200)["blocks"] == [], "instrumental -> empty list")

    reset()
    client, _ = make_client()
    state["get"] = (500, None)
    check(error_of(client.lyrics, "TRACKID12345", "T", "A", "B", 200) == "upstream", "LRCLIB failure -> upstream, not cached")


def font_row_limit():
    import re

    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "include", "fonts", "VHS_Gothic_16.h")
    if not os.path.exists(path):
        return None
    text = open(path).read()
    bitmaps = re.search(r"Bitmaps\s*\[\]\s*(?:PROGMEM)?\s*=\s*\{(.*?)\};", text, re.S).group(1)
    bits = bytes(int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", bitmaps))
    glyphs = re.search(r"GFXglyph\s+\w+\[\]\s*(?:PROGMEM)?\s*=\s*\{(.*?)\};", text, re.S).group(1)
    left, right = 99, 0
    for index, found in enumerate(re.findall(r"\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(-?\d+),\s*(-?\d+)\s*\}", glyphs)):
        offset, width, height, advance, x_offset, _ = (int(v) for v in found)
        if index == 0:
            continue
        columns = [
            x
            for x in range(width)
            if any(bits[offset + ((y * width + x) >> 3)] & (0x80 >> ((y * width + x) & 7)) for y in range(height))
        ]
        if columns:
            left = min(left, x_offset + columns[0])
            right = max(right, x_offset + columns[-1] + 1)
    return [n for n in range(1, 60) if 12 * (n - 1) + right - left <= 276][-1]


def run_lyric_chunk_tests():
    limit = font_row_limit()
    check(limit is None or limit == spotify.ROW_CHARS == 23, f"row width 23 matches the font data (limit {limit})")
    check(spotify.wrap_text("short") == ["short"], "wrap: short line stays one row")
    rows = spotify.wrap_text("one two three four five six seven eight nine")
    check(rows == ["one two three four five", "six seven eight nine"], "wrap: greedy at spaces, 23 characters")
    rows = spotify.wrap_text("a" + "b" * 50)
    check([len(r) for r in rows] == [23, 23, 5] and "".join(rows) == "a" + "b" * 50, "wrap: long word is hard-broken")
    rows = spotify.wrap_text("hi " + "x" * 30)
    check(rows == ["hi", "x" * 23, "x" * 7], "wrap: hard break flushes the pending words first")
    rows = spotify.wrap_text(" ".join(["word"] * 60))
    check(
        len(rows) == spotify.MAX_ROWS_PER_LINE and rows[-1].endswith("..") and all(len(r) <= 23 for r in rows),
        "wrap: at most 4 rows, cut marked with ..",
    )

    two_rows = "a" * 23 + " " + "b" * 23
    rows = spotify.lyric_rows([(10.0, two_rows), (20.0, "next")])
    check([round(t, 3) for t, _ in rows] == [10.0, 12.3, 20.0], "timing: rows spread over 0.1 s per character")
    rows = spotify.lyric_rows([(10.0, two_rows), (11.0, "next")])
    check(round(rows[1][0], 3) == 10.5, "timing: spread is capped by the gap to the next line")
    rows = spotify.lyric_rows([(10.0, two_rows)])
    check(round(rows[1][0], 3) == 12.3, "timing: last line uses a default length")
    rows = spotify.lyric_rows([(10.0, two_rows)], duration=11.0)
    check(round(rows[1][0], 3) == 10.5, "timing: last line is bounded by the track length")
    check(all(a[0] <= b[0] for a, b in zip(rows, rows[1:])), "timing: rows never go backwards")

    blocks = spotify.pair_blocks([(1.0, "a"), (2.0, "b"), (3.0, "c")])
    check(
        blocks == [{"ms": 1000, "a": "a", "b": ""}, {"ms": 2000, "a": "b", "b": ""}, {"ms": 3000, "a": "c", "b": ""}],
        "pairing: one block per line, ms timestamps",
    )
    blocks = spotify.pair_blocks([(1.0, "x"), (5.0, two_rows), (9.0, "y")])
    check(
        blocks
        == [
            {"ms": 1000, "a": "x", "b": ""},
            {"ms": 5000, "a": "a" * 23, "b": "b" * 23},
            {"ms": 9000, "a": "y", "b": ""},
        ],
        "pairing: a two-row line never shares a block with its neighbours",
    )
    three_rows = " ".join(["a" * 23, "b" * 23, "c" * 23])
    blocks = spotify.pair_blocks([(10.0, three_rows), (30.0, "next")])
    check(
        [b["ms"] for b in blocks] == [10000, 14600, 30000] and blocks[1] == {"ms": 14600, "a": "c" * 23, "b": ""},
        "pairing: only the third row of a long line gets an estimated time",
    )
    check(all(blocks[i]["ms"] <= blocks[i + 1]["ms"] for i in range(len(blocks) - 1)), "pairing: block times never go backwards")
    blocks = spotify.pair_blocks([(1.0, two_rows), (9.0, "c")])
    check(
        blocks == [{"ms": 1000, "a": "a" * 23, "b": "b" * 23}, {"ms": 9000, "a": "c", "b": ""}],
        "pairing: a two-row line fills one block",
    )
    blocks = spotify.pair_blocks([(float(i), "x") for i in range(400)])
    check(len(blocks) == spotify.MAX_BLOCKS == 120, "pairing: capped at 120 blocks")


def image_calls():
    return [call[1] for call in calls if call[1].startswith("/img/")]


def run_cover_tests():
    reset()
    client, _ = make_client()
    pixels = client.cover("ALBUMID12345")
    size = spotify.COVER_SIZE
    check(len(pixels) == 15488 == size * size * 2, "cover is 88x88 RGB565 (15488 bytes)")
    check(image_calls() == ["/img/mid.png"], "cover picks the 300 px image")
    orange = ((31 << 11) | (32 << 5)).to_bytes(2, "big")
    blue = (31).to_bytes(2, "big")
    row = 40 * size * 2
    check(pixels[row : row + 2] == orange == b"\xfc\x00", "left pixel is orange, high byte first (0xFC00)")
    check(pixels[row + (size - 1) * 2 : row + size * 2] == blue == b"\x00\x1f", "right pixel is blue, high byte first (0x001F)")
    check(pixels[0:2] == orange and pixels[(size - 1) * 2 : size * 2] == blue, "cover-crop keeps the centre of a wide image")
    split = next(x for x in range(size) if pixels[row + x * 2 : row + x * 2 + 2] == blue)
    check(abs(split - size // 2) <= 1, f"crop is centred (colour change at column {split})")

    before = len(calls)
    again = client.cover("ALBUMID12345")
    check(again == pixels and len(calls) == before, "cover cached per album id, no new request")

    check(spotify.cover_table(IDENTITY) == list(range(256)) * 3, "cover tuning at 1.0 is the identity")
    check(spotify.cover_style({"COVER_GAMMA": "abc", "COVER_GAIN_G": ""})["COVER_GAMMA"] == 1.85, "cover tuning: bad value -> default")
    check(spotify.cover_style({"COVER_SATURATION": "99"})["COVER_SATURATION"] == 3.0, "cover tuning: clamped to a sane range")
    check(spotify.cover_style({"COVER_GAIN_B": "0"})["COVER_GAIN_B"] == 0.0, "cover tuning: zero is a real value")

    reset()
    tuned, _ = make_client(style=spotify.cover_style({}))
    shaded = tuned.cover("ALBUMID12345")
    check(len(shaded) == 15488, "tuned cover keeps the 15488 byte size")
    green = lambda data: (int.from_bytes(data[row : row + 2], "big") >> 5) & 63
    check(green(shaded) < green(pixels), f"default tuning lowers green on orange ({green(pixels)} -> {green(shaded)})")
    check(shaded[row + (size - 1) * 2 : row + size * 2] != b"\x00\x00", "tuned blue stays blue")

    from PIL import Image

    png = client.cover_png("ALBUMID12345")
    check(png[:8] == b"\x89PNG\r\n\x1a\n" and Image.open(io.BytesIO(png)).size == (88, 88), "preview: 88x88 PNG")
    check(Image.open(io.BytesIO(client.cover_png("ALBUMID12345", scale=4))).size == (352, 352), "preview: scale 4 -> 352x352")
    original = Image.open(io.BytesIO(tuned.cover_png("ALBUMID12345", corrected=False))).convert("RGB")
    corrected = Image.open(io.BytesIO(tuned.cover_png("ALBUMID12345"))).convert("RGB")
    check(original.getpixel((0, 40)) == ORANGE and corrected.getpixel((0, 40)) != ORANGE, "preview: original untouched, corrected differs")

    for index in range(spotify.COVER_CACHE + 2):
        client._covers[f"X{index:011d}"] = b"x"
        while len(client._covers) > spotify.COVER_CACHE:
            client._covers.popitem(last=False)
    check(len(client._covers) <= spotify.COVER_CACHE, "cover cache is bounded")

    reset()
    client, _ = make_client()
    with_images = dict(TRACK, album=dict(TRACK["album"], images=[{"url": BASE + "/img/now.png", "width": 300}]))
    state["player"] = (200, {"is_playing": True, "progress_ms": 1, "item": with_images})
    client.now()
    calls.clear()
    client.cover("ALBUMID12345")
    check(image_calls() == ["/img/now.png"] and not any(c[1].startswith("/v1/albums") for c in calls), "url from now-playing saves the album call")

    reset()
    client, _ = make_client()
    state["images"] = False
    check(error_of(client.cover, "ALBUMID12345") == "no_cover", "album without images -> no_cover")

    reset()
    client, _ = make_client()
    state["image_status"] = 404
    check(error_of(client.cover, "ALBUMID12345") == "upstream", "image download failure -> upstream")

    reset()
    client, _ = make_client()
    state["image_bytes"] = b"this is not an image"
    check(error_of(client.cover, "ALBUMID12345") == "no_cover", "undecodable image -> no_cover")
    check(len(client._covers) == 0, "failed covers are not cached")

    reset()
    client, _ = make_client()
    state["image_bytes"] = test_image("JPEG")
    jpeg = client.cover("ALBUMID12345")
    check(len(jpeg) == 15488 and jpeg[row] >> 3 >= 29, "JPEG source decodes to a full-size cover")

    try:
        client.cover("../x")
        check(False, "bad album id rejected for cover")
    except ValueError:
        check(True, "bad album id rejected for cover")


def raw_request(path):
    try:
        with urlopen(Request(SERVER + path), timeout=10) as response:
            return response.status, response.headers.get("Content-Type"), response.headers.get("Content-Length"), response.read()
    except Exception as error:
        return getattr(error, "code", 0), None, None, b""


def run_cover_http_tests():
    reset()
    app.spotify_client, _ = make_client()
    status, kind, length, body = raw_request("/spotify/cover?id=ALBUMID12345")
    check(status == 200 and kind == "application/octet-stream" and length == "15488" and len(body) == 15488, "HTTP cover: octet-stream, 15488 bytes")
    check(body[40 * 176 : 40 * 176 + 2] == b"\xfc\x00", "HTTP cover: pixel bytes intact")
    status, kind, length, body = raw_request("/spotify/cover?id=ALBUMID12345&fmt=png")
    check(status == 200 and kind == "image/png" and body[:4] == b"\x89PNG", "HTTP cover fmt=png: PNG preview")
    status, kind, length, body = raw_request("/spotify/cover?id=ALBUMID12345&fmt=orig&scale=2")
    from PIL import Image

    check(status == 200 and kind == "image/png" and Image.open(io.BytesIO(body)).size == (176, 176), "HTTP cover fmt=orig&scale=2")
    check(raw_request("/spotify/cover?id=ALBUMID12345&fmt=png&scale=x")[0] == 400, "HTTP cover bad scale -> 400")
    status, kind, length, body = raw_request("/spotify/cover?id=bad")
    check(status == 400, "HTTP cover bad id -> 400")
    reset()
    app.spotify_client, _ = make_client()
    state["images"] = False
    status, kind, length, body = raw_request("/spotify/cover?id=ALBUMID12345")
    check(status == 200 and kind == "application/json" and json.loads(body) == {"ok": False, "err": "no_cover"}, "HTTP cover failure: JSON error, not an image")
    app.spotify_client, _ = make_client(refresh=None)
    status, kind, length, body = raw_request("/spotify/cover?id=ALBUMID12345")
    check(kind == "application/json" and json.loads(body)["err"] == "not_linked", "HTTP cover not linked -> JSON error")


def run_tuning_tests():
    reset()
    size = spotify.COVER_SIZE
    right_blue = lambda pixels: int.from_bytes(pixels[40 * size * 2 + (size - 1) * 2 :][:2], "big") & 31

    merged = spotify.merge_style(IDENTITY, {"COVER_GAIN_B": 0.5, "COVER_GAMMA": 99, "COVER_GAIN_R": float("nan"), "COVER_GAIN_G": "x", "COVER_CONTRAST": True, "NOPE": 1})
    check(merged["COVER_GAIN_B"] == 0.5, "tune: valid value applied")
    check(merged["COVER_GAMMA"] == 3.0, "tune: out of range is clamped")
    check(merged["COVER_GAIN_R"] == 1.0 and merged["COVER_GAIN_G"] == 1.0 and merged["COVER_CONTRAST"] == 1.0, "tune: NaN, text and bool are ignored")
    check("NOPE" not in merged and spotify.merge_style(IDENTITY, [1, 2]) == IDENTITY, "tune: unknown names and non-objects ignored")
    check(spotify.merge_style(IDENTITY, {"COVER_GAIN_B": 0})["COVER_GAIN_B"] == 0.0, "tune: zero is a real value")

    client, path = make_client()
    before = client.cover("ALBUMID12345")
    downloads = len(image_calls())
    check(client.now()["cv"] == 0 and client.tune_rev == 0, "tuning version starts at 0 and rides on /now")
    state = client.tune({"COVER_GAIN_B": 0.5})
    after = client.cover("ALBUMID12345")
    check(state["rev"] == 1 and client.now()["cv"] == 1, "tune bumps the version seen by the device")
    check(after != before and right_blue(after) < right_blue(before), "tune re-renders the cover with the new value")
    check(len(image_calls()) == downloads, "tune reuses the downloaded image, no new download")
    check(state["album"] == "ALBUMID12345", "tuning state names the playing album")
    check([p["name"] for p in state["params"]] == [name for name, *_ in spotify.COVER_TUNING], "tuning state lists every parameter in order")
    tuning_file = os.path.join(os.path.dirname(path), spotify.TUNING_FILE)
    check(os.path.exists(tuning_file) and state["saved"], "tune saves cover_tuning.json next to the data file")

    reloaded, _ = make_client(data_file=path, style=None)
    check(reloaded.style["COVER_GAIN_B"] == 0.5, "saved tuning is loaded at start-up")
    explicit, _ = make_client(data_file=path, style=IDENTITY)
    check(explicit.style["COVER_GAIN_B"] == 1.0, "an explicit style ignores the saved file")

    again = client.reset_tuning()
    check(again["rev"] == 2 and not os.path.exists(tuning_file), "reset removes the saved file and bumps the version")
    check(client.style == spotify.cover_style(), "reset returns to env / default values")
    check(client.reset_tuning()["ok"], "reset with no saved file is fine")

    with open(tuning_file, "w") as handle:
        handle.write("{not json")
    broken, _ = make_client(data_file=path, style=None)
    check(broken.style == spotify.cover_style(), "corrupt tuning file -> defaults")

    reset()
    app.spotify_client, _ = make_client()
    status, kind, length, body = raw_request("/spotify/tune")
    check(status == 200 and kind.startswith("text/html") and b"Cover tuning" in body, "HTTP GET /spotify/tune serves the page")
    status, body = request("GET", "/spotify/tune/state")
    check(status == 200 and body["ok"] and len(body["params"]) == len(spotify.COVER_TUNING), "HTTP GET /spotify/tune/state")
    status, body = request("POST", "/spotify/tune", {"COVER_SATURATION": 2.0, "COVER_GAIN_R": float("nan")})
    saturation = next(p for p in body["params"] if p["name"] == "COVER_SATURATION")["value"]
    check(status == 200 and body["rev"] == 1 and saturation == 2.0, "HTTP POST /spotify/tune applies values")
    status, body = request("POST", "/spotify/tune", {"reset": True})
    check(status == 200 and body["rev"] == 2, "HTTP POST /spotify/tune reset")
    check(request("POST", "/spotify/tune", [1])[0] == 400, "HTTP POST /spotify/tune with a non-object -> 400")
    status, body = request("GET", "/spotify/now")
    check(status == 200 and body["cv"] == 2, "HTTP /spotify/now carries the tuning version")


def request(method, path, body=None):
    data = json.dumps(body).encode() if body is not None else None
    http = Request(SERVER + path, data=data, method=method, headers={"Content-Type": "application/json"})
    try:
        with urlopen(http, timeout=10) as response:
            return response.status, json.load(response)
    except Exception as error:
        return getattr(error, "code", 0), None


def run_http_tests():
    reset()
    app.spotify_client, _ = make_client()
    status, body = request("GET", "/spotify/now")
    check(status == 200 and body["ok"] and body["title"] == "Cafe 'Test'", "HTTP GET /spotify/now")
    status, body = request("GET", "/spotify/album?id=ALBUMID12345")
    check(status == 200 and len(body["tracks"]) == 24, "HTTP GET /spotify/album")
    status, body = request("GET", "/spotify/album?id=bad")
    check(status == 400, "HTTP bad album id -> 400")
    status, body = request("GET", "/spotify/lyrics?id=TRACKID12345")
    check(status == 200 and len(body["blocks"]) == 5, "HTTP GET /spotify/lyrics")
    status, body = request("POST", "/spotify/control", {"action": "next"})
    check(status == 200 and body == {"ok": True}, "HTTP POST /spotify/control")
    status, body = request("POST", "/spotify/play", {"album": "ALBUMID12345", "index": 2})
    check(status == 200 and body == {"ok": True}, "HTTP POST /spotify/play")
    status, body = request("POST", "/spotify/control", {"action": "boom"})
    check(status == 400, "HTTP bad action -> 400")
    state["command"] = (404, {"error": {"reason": "NO_ACTIVE_DEVICE"}})
    status, body = request("POST", "/spotify/control", {"action": "next"})
    check(status == 200 and body == {"ok": False, "err": "no_device"}, "HTTP no device -> err code with 200")
    app.spotify_client, _ = make_client(refresh=None)
    status, body = request("GET", "/spotify/now")
    check(status == 200 and body == {"ok": False, "err": "not_linked"}, "HTTP not linked -> err code with 200")
    check(request("GET", "/nope")[0] == 404, "HTTP unknown path -> 404")


def run_live_lrclib():
    client = spotify.Spotify(client_id="x", data_file=os.path.join(tempfile.mkdtemp(), "none.json"))
    meta = {"title": "Yesterday", "artist": "The Beatles", "album": "Help!", "duration": 125}
    lines = client._lookup_lyrics(meta)
    blocks = spotify.pair_blocks(lines)
    ascii_only = all(all(ord(c) < 128 for c in block["a"] + block["b"]) for block in blocks)
    check(len(lines) > 10 and len(blocks) > 5, f"live LRCLIB: {len(lines)} synced lines -> {len(blocks)} blocks")
    check(
        ascii_only and blocks[0]["ms"] >= 0 and all(len(b["a"]) <= 23 and len(b["b"]) <= 23 for b in blocks),
        "live LRCLIB: blocks are ASCII, timed, rows fit 23 characters",
    )
    missing = client._lookup_lyrics({"title": "zzzz qqqq no such song", "artist": "nobody xyz", "album": "", "duration": 100})
    check(missing == [], "live LRCLIB: unknown song -> no lines")


failures = []

if __name__ == "__main__":
    fake = ThreadingHTTPServer(("127.0.0.1", 0), Fake)
    BASE = f"http://127.0.0.1:{fake.server_address[1]}"
    threading.Thread(target=fake.serve_forever, daemon=True).start()

    if "live" in sys.argv:
        run_live_lrclib()
    else:
        bridge = ThreadingHTTPServer(("127.0.0.1", 0), app.Handler)
        SERVER = f"http://127.0.0.1:{bridge.server_address[1]}"
        threading.Thread(target=bridge.serve_forever, daemon=True).start()
        run_fake_tests()
        run_lyric_chunk_tests()
        run_cover_tests()
        run_http_tests()
        run_cover_http_tests()
        run_tuning_tests()

    print("\nFAILED: " + ", ".join(failures) if failures else "\nall passed")
    sys.exit(1 if failures else 0)
