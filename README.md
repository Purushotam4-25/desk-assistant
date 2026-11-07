# ESP32 Desk Assistant

A modular desk companion built on an ESP32. A 2.8" TFT shows a page-based UI driven by a single rotary encoder, with an RTC for time, a DHT11 for room temperature and humidity, and an SD card holding fonts and assets.

Built with PlatformIO and the Arduino framework. This is a learning project, so the code is split into small managers rather than one sketch.

## Hardware

| Part | Model | Notes |
|------|-------|-------|
| Microcontroller | ESP32 DevKit (`esp32doit-devkit-v1`) | 3.3V logic only |
| Display | 2.8" SPI TFT, 320x240, landscape | Native-landscape panel, so `platformio.ini` uses `ILI9342_DRIVER` and `setRotation(0)`. MISO not connected |
| Input | Rotary encoder with push button | Rotate, click, double click, long press |
| Storage | HW-203 SD card module | Fonts and assets |
| Clock | DS1302 RTC | CR2032 backup battery |
| Sensor | DHT11 | Temperature and humidity |

All grounds are shared. The ESP32 runs on USB or VIN (5V); everything else sits on 3.3V.

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

The TFT pins are set in `platformio.ini` build flags, so there is no `User_Setup.h` to edit. The wiring may be redone, so treat this table as current-build only.

## Controls

| Action | Result |
|--------|--------|
| Rotate | Scroll or change selection |
| Click | Select / enter |
| Double click | Back to home |
| Long press | Open settings / global menu |

## Software

```
src/
  main.cpp             setup and main loop
  ScreenManager.cpp    routes encoder input to the active screen
  MainScreen.cpp       Main page: clock, room climate, next event
  TodayScreen.cpp      Today page: agenda
  Widgets.cpp          shared drawing: status bar, icons, cards, text helpers
  Fonts.cpp            the three GFX fonts, defined once
  StatusService.cpp    reads RTC, DHT11 and WiFi into one DeviceStatus
  Agenda.cpp           event list; done / now / upcoming are computed from the time
  rot_encoder.cpp      encoder + button abstraction
  RTCManager.cpp       time and date
  DHTManager.cpp       temperature and humidity
  SDManager.cpp        SD card, JSON read and crash-safe write
  ConfigService.cpp    settings from the SD config files, saved copy in NVS, write-back
include/
  Theme.h              every colour, in one place
  screens.h            Screen interface and RotaryInput events
  fonts/               GFX font headers
```

Screens never talk to the drivers. `StatusService` polls them on a timer and fills a `DeviceStatus`, and each screen is given a read-only reference to it. Screens only redraw the part of the display whose data changed, so the clock digits update once a minute and the colon blinks without redrawing the whole screen.

The `Agenda` currently holds sample events in `main.cpp`. Swap that for a real source (SD file, calendar over WiFi) and the Main and Today pages follow.

Every page implements the `Screen` interface (`init`, `update`, `draw`, `handleInput`). `ScreenManager` holds the active screen, turns encoder events into `RotaryInput` values, and forwards them.

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
/config.json    settings (home UTC offset, location, 12/24 hour clock)
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

Upload is set to `COM4` in `platformio.ini`; change it to match your board. A second environment, `esp32dev_test_display`, builds only `test/display_test.cpp` for checking the screen on its own.

## Status

Drivers for the display, encoder, RTC, DHT11, SD card and WiFi work. The Main page shows the real RTC clock, date, room temperature and humidity, a day-progress bar and the next event; the Today page shows the agenda. Both are driven by live data, but the events are sample data and neither page has been tested on the board yet. There is no way to switch pages yet. Still to do: page navigation, the remaining pages, and a real calendar source.
