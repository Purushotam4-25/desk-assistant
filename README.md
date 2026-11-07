# ESP32 Desk Assistant

A modular desk companion built on an ESP32. A 2.8" TFT shows a page-based UI driven by a single rotary encoder, with an RTC for time (kept right over WiFi with NTP), a DHT11 for room temperature and humidity, and an SD card holding settings and, later, assets.

Built with PlatformIO and the Arduino framework. This is a learning project, so the code is split into small managers rather than one sketch.

## Hardware

| Part | Model | Notes |
|------|-------|-------|
| Microcontroller | ESP32 DevKit (`esp32doit-devkit-v1`) | 3.3V logic only |
| Display | 2.8" SPI TFT, 320x240, landscape | Native-landscape panel, so `platformio.ini` uses `ILI9342_DRIVER` and `setRotation(2)`. MISO not connected |
| Input | Rotary encoder with push button | Rotate, click, double click, long press |
| Input | Two side push buttons | Previous / next sub-page; touch pads are planned |
| Storage | HW-203 SD card module | Settings and WiFi credentials, assets later |
| Clock | DS1302 RTC | CR2032 backup battery |
| Sensor | DHT11 | Temperature and humidity |
| Network | ESP32 built-in WiFi | 2.4 GHz networks only |

All grounds are shared. The ESP32 runs on USB or VIN (5V). The SD module's VCC goes to 5V (its onboard regulator needs the headroom; on 3.3V the card gets only about 2V and fails intermittently). Everything else sits on 3.3V.

## Wiring

| Component | Signal | GPIO |
|-----------|--------|------|
| TFT | CS | 13 |
| | DC | 17 |
| | RESET | 16 |
| | MOSI (SDI) | 23 |
| | SCK | 18 |
| | MISO (SDO) | not connected |
| | LED | 3.3V |
| SD card | CS | 5 |
| | MOSI | 23 (shared with TFT) |
| | SCK | 18 (shared with TFT) |
| | MISO | 19 |
| Rotary encoder | CLK | 32 |
| | DT | 33 |
| | SW | 15 |
| DS1302 RTC | CLK | 27 |
| | DAT | 26 |
| | RST | 25 |
| DHT11 | DATA | 4 |
| Side buttons | Left | 21 |
| | Right | 22 |

The side buttons wire between the pin and GND (internal pull-ups). The TFT pins are set in `platformio.ini` build flags, so there is no `User_Setup.h` to edit. The wiring may be redone, so treat this table as current-build only.

## Controls

| Action | Result |
|--------|--------|
| Rotate | Move focus to the next / previous item |
| Click | Press the focused item |
| Double click | Open the Screens menu |
| Long press | Go Home |
| Left / right side button | Previous / next sub-page |

## Software

```
src/
  main.cpp              builds everything, setup and main loop
  ScreenManager.cpp     holds the active screen; handles Home, Menu, sub-pages and detail views
  InputRouter.cpp       turns the encoder and the side buttons into one input stream
  *Screen.cpp           the pages: Main, Today, Focus, Weather (4), World clock, Timer, Pomodoro,
                        Music (3), Calendar (2), Quick menu, plus the Screens menu
  Widgets.cpp, Icons.cpp  shared drawing: status bar, icons, cards, buttons, scroll bar, ink-based text
  Fonts.cpp             the three GFX fonts and their glyph metrics
  StatusService.cpp     reads RTC, DHT11 and WiFi into one DeviceStatus
  ConfigService.cpp     settings from the SD config files, saved copy in NVS, write-back
  WifiService.cpp       non-blocking WiFi connect and reconnect
  TimeSync.cpp          NTP into the RTC
  FocusSession, TimerService, MusicPlayer
                        background services that keep running when you leave a page
  Agenda, SunCalc, WorldMap  event list, sunrise / sunset maths, the dot-matrix world map
  RTCManager, DHTManager, SDManager, rot_encoder, TapButton
                        hardware drivers
include/
  Theme.h               every colour, in one place
  screens.h             Screen interface and Input events
  fonts/                GFX font headers
```

Screens never talk to the drivers. `StatusService` polls them on a timer and fills a `DeviceStatus`, and each screen is given a read-only reference to it. Screens only redraw the part of the display whose data changed, so the clock digits update once a minute and the colon blinks without redrawing the whole screen.

The `Agenda` currently holds sample events in `main.cpp`. Swap that for a real source (SD file, calendar over WiFi) and the Main and Today pages follow.

Every page implements the `Screen` interface (`init`, `update`, `draw`, `handleInput`). `ScreenManager` holds the active screen and forwards the `Input` values that `InputRouter` produces. Pages that need data from a service (focus sessions, timers, music, settings) read it through a reference and change it only through that service.

One rule: no `delay()` in the running code. Everything is timed with `millis()` so input and drawing never block each other.

## UI designs

The screens are mocked up in [Lopaka](https://lopaka.app) (`tft-espi`, 320x240) and kept locally in `DeskAssUI/`, which is not tracked in git. They are visual references only; the firmware draws everything itself from live data, and colours come from `Theme.h`.

| Group | Screens |
|-------|---------|
| Main | Main, Today |
| Calendar | Calendar, afternoon view |
| Focus | Focus, running, paused, stats |
| Timer | Timer, set, running, paused, done; Pomodoro, running, paused, break |
| Music | Now playing, album tracks, lyrics |
| Weather | Now, forecast, rooms, air |
| World clock | Local, other zones, 12-hour |
| Menus | Quick menu, screens menu |

## SD card layout

```
/config.json    settings (home UTC offset, location, 12/24 hour clock, NTP on/off)
/secrets.json   WiFi name and password
```

Both files are optional; see [`sd-card/`](sd-card/) for examples. The device keeps working with no card by falling back to built-in defaults and the last settings it saved to its own memory. Settings changed on the device are written back to `config.json` a few seconds after the change. The real files are gitignored and must never be committed.

The UI uses GFX fonts (`Pixellari`, `VHS Gothic`, `Org_01`) compiled into flash from `include/fonts/`, so the card is not needed for fonts right now.

## Libraries

- `TFT_eSPI` for the display
- `ESP32Encoder` and `OneButton` for the encoder
- `SdFat` (Adafruit fork) for the SD card
- `ArduinoJson` for the config files
- `makuna/RTC` for the DS1302
- `DHT sensor library` for the DHT11
- `gpio_viewer`, `AsyncTCP` and `ESPAsyncWebServer` for debugging over WiFi

`platformio.ini` still lists `U8g2`, `Adafruit SSD1306` and `Adafruit GFX` from the earlier OLED version of this project.

## Build and upload

```
pio run -t upload
pio device monitor
```

Upload is set to `COM4` in `platformio.ini`; change it to match your board. The firmware uses the `huge_app.csv` partition table (one 3 MB app slot, no over-the-air updates) because the WiFi stack alone takes about 600 KB; the first upload after changing the table rewrites it. There are also small hardware test environments (`colortest`, `palettetest`, `paneltest`, `sdtest`); their sources live in `test/`, which is not fully tracked, so they may not build from a fresh clone.

## Status

Everything runs on the real board except the newest pages, which are compiled but not yet checked on it. Drivers for the display, encoder, side buttons, RTC, DHT11, SD card and WiFi work. The device connects to WiFi in the background, sets the clock from NTP, and loads its settings from the SD card (falling back to its own saved copy with no card).

Built pages: Main (clock, date, day bar, room climate, next event) with Today, Focus and the Quick menu as sub-pages; Focus stats; Calendar (Morning and Afternoon day views, scrolled with the knob); Weather (Now, Forecast, Rooms, Air); World clock; Timer and Pomodoro; Music (Now playing, Album tracks, Lyrics); Settings (Time, Timer, Display, Alerts, Connections, Sensors, System, About); and the Screens menu. Much of the data is sample data: agenda events, weather, focus history, and an invented album on a simulated music player (there is no audio hardware or music source yet). The Quick menu's Wi-Fi, Night and Brightness buttons only change on-screen state.

The Settings page stores every setting in the SD config and NVS; time, timer, Pomodoro and sensor settings take effect, while display, alert and buzzer settings are stored only until the hardware and an alert service exist.

Still to do: a real calendar and weather source, a music source, brightness control (needs a transistor on the backlight), a buzzer with alerts, and the Pet page.
