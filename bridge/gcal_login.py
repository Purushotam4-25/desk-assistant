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

import gcal

HOST, PORT = "127.0.0.1", 8889
REDIRECT = f"http://{HOST}:{PORT}/callback"
SCOPE = "https://www.googleapis.com/auth/calendar.readonly"
AUTHORIZE = "https://accounts.google.com/o/oauth2/v2/auth"


def main():
    if len(sys.argv) < 2:
        sys.exit("usage: python gcal_login.py <path to the OAuth client JSON from Google Cloud>")
    with open(sys.argv[1]) as handle:
        client = json.load(handle).get("installed")
    if not client or not client.get("client_id") or not client.get("client_secret"):
        sys.exit("That file is not a Desktop app OAuth client (it needs an 'installed' section).")

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
            self.wfile.write(b"Calendar linked. You can close this tab." if "code" in result else b"Linking failed.")
            done.set()

        def log_message(self, format, *args):
            pass

    server = HTTPServer((HOST, PORT), Callback)
    threading.Thread(target=server.serve_forever, daemon=True).start()

    query = urlencode(
        {
            "response_type": "code",
            "client_id": client["client_id"],
            "redirect_uri": REDIRECT,
            "scope": SCOPE,
            "state": state,
            "code_challenge_method": "S256",
            "code_challenge": challenge,
            "access_type": "offline",
            "prompt": "consent",
        }
    )
    link = AUTHORIZE + "?" + query
    print("Opening the Google login in your browser. Approve read-only calendar access. If nothing opens, visit:\n" + link)
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
            "client_id": client["client_id"],
            "client_secret": client["client_secret"],
            "code_verifier": verifier,
        }
    ).encode()
    request = Request(
        client.get("token_uri") or gcal.TOKEN_URL,
        data=body,
        headers={"Content-Type": "application/x-www-form-urlencoded", "User-Agent": gcal.USER_AGENT},
    )
    with urlopen(request, timeout=15) as response:
        granted = json.load(response)
    if not granted.get("refresh_token"):
        sys.exit("Google returned no refresh token. Remove this app's access at myaccount.google.com/permissions and try again.")

    path = os.environ.get("GOOGLE_DATA") or gcal.DEFAULT_DATA
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as handle:
        json.dump(
            {
                "client_id": client["client_id"],
                "client_secret": client["client_secret"],
                "refresh_token": granted["refresh_token"],
            },
            handle,
        )
    print("Saved " + path + ". Copy it into the data folder next to the bridge on the NAS (it is gitignored).")


if __name__ == "__main__":
    main()
