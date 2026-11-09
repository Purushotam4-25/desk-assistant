import base64
import hashlib
import json
import os
import secrets
import sys
import threading
import webbrowser
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs, urlencode, urlparse
from urllib.request import Request, urlopen

import spotify

HOST, PORT = "127.0.0.1", 8888
REDIRECT = f"http://{HOST}:{PORT}/callback"
SCOPES = "user-read-playback-state user-modify-playback-state"


def main():
    client_id = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("SPOTIFY_CLIENT_ID", "")
    if not client_id:
        sys.exit("usage: python spotify_login.py <client id>   (or set SPOTIFY_CLIENT_ID)")

    verifier = secrets.token_urlsafe(64)
    challenge = base64.urlsafe_b64encode(hashlib.sha256(verifier.encode()).digest()).rstrip(b"=").decode()
    state = secrets.token_urlsafe(16)
    result = {}
    done = threading.Event()

    class Callback(BaseHTTPRequestHandler):
        def do_GET(self):
            url = urlparse(self.path)
            if url.path != "/callback":
                self.send_response(404)
                self.end_headers()
                return
            args = parse_qs(url.query)
            if args.get("state", [""])[0] != state or "code" not in args:
                result["error"] = args.get("error", ["state mismatch"])[0]
            else:
                result["code"] = args["code"][0]
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(b"Spotify linked. You can close this tab." if "code" in result else b"Linking failed.")
            done.set()

        def log_message(self, format, *args):
            pass

    server = HTTPServer((HOST, PORT), Callback)
    threading.Thread(target=server.serve_forever, daemon=True).start()

    query = urlencode(
        {
            "response_type": "code",
            "client_id": client_id,
            "scope": SCOPES,
            "redirect_uri": REDIRECT,
            "state": state,
            "code_challenge_method": "S256",
            "code_challenge": challenge,
        }
    )
    link = spotify.ACCOUNTS + "/authorize?" + query
    print("Opening the Spotify login in your browser. If nothing opens, visit:\n" + link)
    webbrowser.open(link)
    done.wait(timeout=300)
    server.shutdown()

    if "code" not in result:
        sys.exit("Login failed: " + result.get("error", "timed out"))

    body = urlencode(
        {
            "grant_type": "authorization_code",
            "code": result["code"],
            "redirect_uri": REDIRECT,
            "client_id": client_id,
            "code_verifier": verifier,
        }
    ).encode()
    request = Request(
        spotify.ACCOUNTS + "/api/token",
        data=body,
        headers={"Content-Type": "application/x-www-form-urlencoded", "User-Agent": spotify.USER_AGENT},
    )
    with urlopen(request, timeout=15) as response:
        granted = json.load(response)

    path = os.environ.get("SPOTIFY_DATA") or spotify.DEFAULT_DATA
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as handle:
        json.dump({"client_id": client_id, "refresh_token": granted["refresh_token"]}, handle)
    print("Saved " + path + ". Copy it to the data folder next to the bridge on the NAS.")


if __name__ == "__main__":
    main()
