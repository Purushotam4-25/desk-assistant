# Desk bridge

A small service for a home server (a Synology NAS) that does the internet work for the desk assistant. The ESP32 only makes plain HTTP calls on the home network and gets back a tiny JSON, so it needs no TLS and holds no API credentials.

It serves weather from Open-Meteo (no key needed) and the Spotify Music page (now playing, controls, album, lyrics). The calendar is meant to join later.

## Endpoints

| Path | Result |
|------|--------|
| `GET /weather?lat=19.07&lon=72.88` | Current conditions, today's high / low, UV, rain chance and the next three days. Cached for `CACHE_MINUTES` (default 15); if Open-Meteo fails the last good copy is served |
| `GET /health` | `{"ok":true}` |
| `GET /spotify/now` | What is playing: `playing`, `id`, `title`, `artist`, `albumId`, `album`, `track`, `pos` and `dur` in ms. `{"ok":true,"idle":true}` when a device is active but nothing plays. Cached 2 s |
| `GET /spotify/album?id=` | Album title, artist, year, total and up to 24 tracks `{id, t, ms}`. Cached 24 h |
| `GET /spotify/cover?id=<album id>` | The album cover as raw pixels, not JSON: exactly 15,488 bytes, 88 x 88, RGB565, high byte first, `Content-Type: application/octet-stream`. Cropped to a square from the centre, colour-corrected for the panel (see Cover colours), cached for the last 8 albums. `&fmt=png` returns the corrected cover as a PNG, `&fmt=orig` the uncorrected one, `&scale=1..8` enlarges it, for viewing on the PC. On failure it returns JSON `{"ok":false,"err":"no_cover"}` (or another code) with a JSON content type, so the device keeps its placeholder. Without `fmt` a browser shows it as a download or garbage; check the size is 15488 |
| `GET /spotify/lyrics?id=` | Synced lyrics from LRCLIB as blocks `{ms, a, b}`, one block per LRC line: `ms` is the line's own timestamp in milliseconds, each row is at most 23 characters, a line wraps to at most two rows per block (`b` is empty for a one-row line), and a line longer than two rows continues in a second block timed by an estimate (0.1 s per character); up to 120 blocks; `blocks` is empty when none exist. Metadata comes from the last `/spotify/now`; `title`, `artist`, `album` and `ms` query parameters override it |
| `POST /spotify/control` | Body `{"action":"play"\|"pause"\|"next"\|"prev"}` |
| `POST /spotify/play` | Body `{"album":"<album id>","index":<position>}` plays that track of the album |

Spotify problems come back as HTTP 200 with `{"ok":false,"err":"<code>"}`: `not_linked`, `no_device`, `premium_required`, `rate_limited`, `upstream`, `no_cover` (the album has no image, or it could not be decoded). Text sent to the device is reduced to plain ASCII (the display fonts have no other characters), so accents are dropped and non-Latin lyric lines are skipped.

Weather example response:

```json
{"cond":1,"sum":"Mostly clear","temp":18,"hum":62,"hi":23,"lo":14,"uv":5,"rain":10,"fc":[{"c":0,"hi":24,"lo":14},{"c":2,"hi":20,"lo":13},{"c":3,"hi":17,"lo":11}],"age":0}
```

`cond` / `c`: 0 Clear, 1 Mostly clear, 2 Cloudy, 3 Rain, 4 Fog, 5 Showers, 6 Storm (snow counts as Cloudy). Day or night is worked out on the device from the sun position (`SunCalc`), so the bridge sends no day flag.

## Spotify setup (once)

Spotify's rules for new developer apps (2026): the app owner needs Spotify Premium, and a development-mode app only works for a few authorised users. That is fine for one person.

1. Open the Spotify developer dashboard (developer.spotify.com/dashboard), create an app, and add this exact redirect URI: `http://127.0.0.1:8888/callback`. Tick the Web API. Copy the Client ID. There is no client secret to use.
2. On the PC, in this folder, run `python spotify_login.py <client id>`. A browser opens, you approve, and `data/spotify.json` is written. It holds a long-lived refresh token: keep it private (the folder is gitignored).
3. Copy the `data` folder next to `docker-compose.yml` on the NAS, so the container sees `/app/data/spotify.json`. Optionally put `SPOTIFY_CLIENT_ID=<client id>` in a `.env` file beside `docker-compose.yml` (also gitignored); the id is already stored in `spotify.json`, so this is not required.
4. Redeploy on the NAS: copy `app.py`, `spotify.py`, `tune_page.py`, `Dockerfile` and `docker-compose.yml` over, then `sudo docker compose up -d --build`. The Dockerfile installs Pillow (for the cover art), so the first rebuild downloads it and takes a little longer; later rebuilds reuse it. Pillow has ready-made packages for x86-64 and 64-bit ARM Synology models; a very old 32-bit ARM model may fail to build.
5. On the device, open Spotify on any device (phone, PC), start a song, then open the Music page. Settings > Connections > Spotify shows the status.

The bridge refreshes access tokens itself and keeps a rotated refresh token. If the token is ever revoked, the Music page says "Not connected": repeat step 2 and 3.

Lyrics come from LRCLIB (lrclib.net), a free public service with no key. Not every song has synced lyrics.

## Cover colours

The panel shows green stronger than the RGB numbers say, which washes out covers and turns pinks and browns orange. The bridge corrects each cover before sending it. These environment variables tune the correction; all are optional, and the defaults are the values the user tuned by eye on the panel on 2026-11-09, so adjust from there. Put them in a `.env` file next to `docker-compose.yml` (gitignored), for example `COVER_GAMMA_G=1.6`.

| Variable | Default | Effect |
|----------|---------|--------|
| `COVER_SATURATION` | 1.2 | Colour strength (1.0 = unchanged) |
| `COVER_CONTRAST` | 1.225 | Contrast (1.0 = unchanged) |
| `COVER_GAMMA` | 1.85 | All channels; above 1 darkens the mid-tones |
| `COVER_GAMMA_G` | 1.3 | Extra gamma on green only; above 1 reduces green in the mid-tones, black and white stay put |
| `COVER_GAIN_R`, `COVER_GAIN_G`, `COVER_GAIN_B` | 0.8, 1.1, 1.0 | Multiplier per channel |

All at 1.0 sends the cover untouched. Bad values fall back to the default; extremes are clamped. The values in use are printed in the container log when the bridge starts.

To tune live, open `http://<nas-ip>:8765/spotify/tune` in a browser. It has a slider and number box per value, a reset button, a preview of the playing album's cover (original next to what is sent) and the current values as `.env` lines. Every change is applied at once and saved to `data/cover_tuning.json`, which wins over `.env` and the defaults until you press Reset (that deletes the file and goes back to `.env` / defaults). The bridge adds a tuning version (`cv`) to `/spotify/now`; with the Music page open, the device sees it change on its next poll and fetches the cover again, so set Settings > Connections > Spotify > Poll every to 5 s while tuning. The cover on the panel flashes to the placeholder for a moment while it reloads. Like the player controls, the page has no authentication: never forward the port.

The old way still works: change `.env`, run `sudo docker compose up -d` (the restart clears the cover cache), then on the device open Settings > Connections > Spotify > Test now and go back to the Music page; the cover is fetched again. Pinks still orange or washed out: raise `COVER_GAMMA_G` (1.6, 2.0) or lower `COVER_GAIN_G`. Greys and whites turn magenta: lower `COVER_GAMMA_G` toward 1.0. Too dark: lower `COVER_GAMMA`. Still flat: raise `COVER_SATURATION` or `COVER_CONTRAST`. Blue looks weak: raise `COVER_GAIN_B`.

To see what the bridge sends without the device, open `http://<nas-ip>:8765/spotify/cover?id=<album id>&fmt=png&scale=4` (corrected) next to `...&fmt=orig&scale=4` (original). A PC screen is not the panel, so this only shows the direction of the change.

## Run on the Synology

1. Copy this `bridge/` folder to the NAS (for example `/volume1/docker/desk-bridge`).
2. In Container Manager, create a Project, point it at that folder and use `docker-compose.yml`. Build and start it (or `sudo docker compose up -d --build` over SSH).
3. Check `http://<nas-ip>:8765/health` from a browser on the home network.
4. Put the address in the device's `config.json` as `"bridgeUrl": "http://<nas-ip>:8765"`. Give the NAS a fixed IP (DHCP reservation) so the address does not change.

## Run on a PC for testing

```
python app.py
```

Then open `http://localhost:8765/weather?lat=19.07&lon=72.88`.

`python test_spotify.py` runs the Spotify tests against a fake Spotify and a fake LRCLIB on the PC (no account needed; the cover tests need `pip install pillow` on the PC). `python test_spotify.py live` also checks the real LRCLIB.

Do not expose the port to the internet. There is no authentication, and the Spotify control endpoints would let anyone who can reach the port skip your tracks.
