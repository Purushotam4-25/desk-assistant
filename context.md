# Desk Assistant
**Last updated:** 2026-03-27
**Repo:** https://github.com/Purushotam4-25/desk-assistant
**Notion (ground truth):** https://www.notion.so/1ef7d1d58f65807c8394f8396f04b010

This file is for AI assistants and developers.
The authoritative project documentation lives in Notion.
---

## What this is
ESP32-based desk assistant. Modular C++ firmware on PlatformIO/Arduino framework.
ILI9341 TFT display, rotary encoder UI, SD card for fonts/assets, DS1302 RTC,
DHT11 sensor, simple audio alerts. Built for learning — understand before implementing.

## Architecture keywords
- `bubblewrap` — overall modular architecture (ScreenManager, BasePage, pages/)
- `emptyroll` — page navigation / scroll-to-switch behaviour

## Hard rules
- Never use delay() — entire codebase is non-blocking
- ILI9341 only — not ST7735 or any other display
- Always verify pin assignments against the table below before writing config code
- Explain before implementing — don't dump code without reasoning

## Pin Configuration (confirmed 2026-03-17)
| Component       | Signal | GPIO |
|----------------|--------|------|
| SD Card (HW-203)| CS    | 5    |
|                 | MOSI  | 23   | (shared with TFT)
|                 | SCK   | 18   | (shared with TFT)
|                 | MISO  | 19   |
| DHT11           | DAT   | 4    |
| DS1302 RTC      | CLK   | 27   |
|                 | DAT   | 26   |
|                 | RST   | 25   |
| Rotary Encoder  | CLK   | 32   |
|                 | DT    | 33   |
|                 | SW    | 15   |
| ILI9341 Display | CS    | 13   |
|                 | RESET | 16   |
|                 | DC    | 17   |
|                 | MOSI  | 23   | (shared with SD)
|                 | SCK   | 18   | (shared with SD)
|                 | MISO  | NC   | (write-only, not connected)

## Libraries
| Purpose    | Library                    |
|------------|----------------------------|
| Display    | TFT_eSPI                   |
| Encoder    | ESP32Encoder               |
| Button     | OneButton                  |
| Storage    | SD / SdFat (Adafruit fork) |
| RTC        | makuna/RTC                 |
| Sensor     | DHT sensor library         |

## Architecture (bubblewrap)
```
src/
 ├── main.cpp                      → setup(), loop(), state routing
 ├── ScreenManager.cpp / .h        → owns TFT instance + active page
 ├── RotaryEncoderManager.cpp / .h → encoder abstraction
 ├── FontManager.cpp / .h          → SD font loading (planned)
 └── pages/
      ├── BasePage.cpp / .h        → abstract: onEnter, onExit, update, draw
      ├── HomePage.cpp / .h
      ├── SettingsPage.cpp / .h
      └── (more TBD)
```

## UI / Navigation (emptyroll)
| Action       | Function              |
|--------------|-----------------------|
| Rotate       | Scroll / change value |
| Single click | Select / enter        |
| Double click | Return to home        |
| Long press   | Open settings         |

Planned pages: HOME · CLOCK · SENSOR · TIMER · IDLE · SETTINGS

## Feature Status
| Feature           | Status     |
|-------------------|------------|
| DHT11 sensor      | ✅ Done    |
| ILI9341 display   | ✅ Done    |
| DS1302 RTC        | ✅ Done    |
| SD Card           | ✅ Done    |
| Rotary Encoder    | ✅ Done    |
| WiFi              | ✅ Done    |
| Better knob UX    | 🔲 TODO   |
| Battery system    | 🔲 TODO   |

## Current focus (2026-03-27)
Resuming after a pause. Doing component-by-component audit:
1. Display — ILI9341 bring-up, TFT_eSPI User_Setup.h
2. Rotary Encoder — input handling, all button events
3. UI State Machine — ScreenManager + page routing
4. Sensor pages — DHT11 + DS1302 display

## SD Card structure
/SD
 ├── fonts/   (Inter-Regular-16.vlw, Inter-Bold-24.vlw, Mono-14.vlw — already on card)
 ├── images/  (icon_weather.bmp, icon_battery.bmp)
 └── data/    (future use)

# Reminder: Did you update the Notion build log?
# https://www.notion.so/1ef7d1d58f65807c8394f8396f04b010