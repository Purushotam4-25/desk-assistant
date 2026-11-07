# Desk bridge

A small service for a home server (a Synology NAS) that does the internet work for the desk assistant. The ESP32 only makes plain HTTP calls on the home network and gets back a tiny JSON, so it needs no TLS and holds no API credentials.

Today it serves weather from Open-Meteo (no key needed). Spotify and the calendar are meant to join it later.

## Endpoints

| Path | Result |
|------|--------|
| `GET /weather?lat=19.07&lon=72.88` | Current conditions, today's high / low, UV, rain chance and the next three days. Cached for `CACHE_MINUTES` (default 15); if Open-Meteo fails the last good copy is served |
| `GET /health` | `{"ok":true}` |

Example response:

```json
{"cond":1,"sum":"Mostly clear","temp":18,"hum":62,"hi":23,"lo":14,"uv":5,"rain":10,"fc":[{"c":0,"hi":24,"lo":14},{"c":2,"hi":20,"lo":13},{"c":3,"hi":17,"lo":11}],"age":0}
```

`cond` / `c`: 0 Clear, 1 Mostly clear, 2 Cloudy, 3 Rain (snow and fog count as Cloudy).

## Run on the Synology

1. Copy this `bridge/` folder to the NAS (for example `/volume1/docker/desk-bridge`).
2. In Container Manager, create a Project, point it at that folder and use `docker-compose.yml`. Build and start it.
3. Check `http://<nas-ip>:8765/health` from a browser on the home network.
4. Put the address in the device's `config.json` as `"bridgeUrl": "http://<nas-ip>:8765"`. Give the NAS a fixed IP (DHCP reservation) so the address does not change.

## Run on a PC for testing

```
python app.py
```

Then open `http://localhost:8765/weather?lat=19.07&lon=72.88`.

Do not expose the port to the internet. There is no authentication.
