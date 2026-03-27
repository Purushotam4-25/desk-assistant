# Desk Assistant — Project "Gold Standard"
**Last Updated:** 2026-03-27
**Repo:** https://github.com/Purushotam4-25/desk-assistant
**Status:** Component Audit Complete -> Starting "Bubblewrap" Overhaul

---

## 🎯 One-Line Summary
A modular ESP32-based desk assistant with an ILI9341 TFT display, rotary-encoder-driven page-based UI, SD card for external fonts/assets, RTC, DHT11 sensor, and simple audio alerts — designed for learning, extensibility, and clarity.

---

## 🛠 Hardware Specification
### Power Philosophy
- **ESP32 Logic:** 3.3V ONLY (not 5V tolerant).
- **Ground:** All grounds shared.

### Pin Configuration (Verified 2026-03-17)
| Component       | Signal      | GPIO | Remarks                        |
|----------------|-------------|------|--------------------------------|
| HW-203 SD Card | CS          | 5    |                                |
|                | MOSI        | 23   | Shared with TFT                |
|                | SCK         | 18   | Shared with TFT                |
|                | MISO        | 19   |                                |
| DHT11          | DAT         | 4    |                                |
| DS1302 RTC     | CLK         | 27   |                                |
|                | DAT         | 26   |                                |
|                | RST         | 25   |                                |
| Rotary Encoder | CLK         | 32   |                                |
|                | DT          | 33   |                                |
|                | SW (button) | 15   |                                |
| ILI9341 Display| CS          | 13   |                                |
|                | RESET       | 16   |                                |
|                | DC          | 17   |                                |
|                | SDI (MOSI)  | 23   | Shared with SD                 |
|                | SCK         | 18   | Shared with SD                 |
|                | MISO/SDO    | NC   | Write-only                     |

---

## 🧠 Software Architecture (bubblewrap)
Modular C++ on PlatformIO. **Mandatory Rule:** NO `delay()` allowed.

### Planned File Structure
```
/src
  main.cpp                      → Init managers, start loop()
  /screens
    ScreenManager.h / .cpp      → Routes input to active page
    BasePage.h                  → Abstract interface: init, update, draw
    MainPage.h / .cpp           → Default dashboard
    SettingsPage.h / .cpp       → Configuration menu
  /ui
    FontManager.h / .cpp        → SD font (.vlw) loading
    AssetManager.h / .cpp       → BMP loading
  /input
    RotaryEncoderManager.h/.cpp → Encoder abstraction (clicks, rotation)
  /sensors
    DHTManager.h / .cpp         → Non-blocking sensor reads
    RTCManager.h / .cpp         → Time/Date management
```

### Page Interface (BasePage)
```cpp
class ScreenPage {
public:
    virtual void init() = 0;
    virtual void update() = 0;
    virtual void draw() = 0;
    virtual void handleInput(RotaryInput input) = 0;
};
```

---

## 🎛 UI & Navigation (emptyroll)
- **Paradigm:** Only one page active at a time.
- **Input:** Rotary encoder only.

| Action       | Function                    |
|--------------|-----------------------------|
| Rotate       | Scroll / change selection   |
| Single Click | Select / enter              |
| Double Click | Return to HOME              |
| Long Press   | Open SETTINGS / Global Menu |

**Planned Pages:** HOME · CLOCK · SENSOR · TIMER · IDLE · SETTINGS

---

## 📂 Asset & Font System
- **Fonts:** TTF converted to `.vlw` (TFT_eSPI smooth font format).
- **Location:** SD card storage.
```
/SD
├── fonts/ (Inter-Regular-16.vlw, Inter-Bold-24.vlw, Mono-14.vlw)
├── images/ (icon_weather.bmp, icon_battery.bmp)
└── data/ (config.json)
```

---

## 🚦 Feature Status & Focus
| Feature           | Status |
|-------------------|--------|
| DHT11 Sensor      | ✅ Done|
| ILI9341 Display   | ✅ Done|
| DS1302 RTC        | ✅ Done|
| SD Card           | ✅ Done|
| Rotary Encoder    | ✅ Done|
| WiFi Connectivity | ✅ Done|
| Bubblewrap UI     | 🚧 IN PROGRESS |

### Current Focus (2026-03-27)
1. **ScreenManager System:** Build the core state machine for page routing.
2. **BasePage Implementation:** Define the interface for all future modules.
3. **Driver Encapsulation:** Move global RTC/DHT/SD functions into Manager classes.

---

## 📚 Libraries
- **Display:** `TFT_eSPI` (ILI9341)
- **Encoder:** `ESP32Encoder` (Hardware-accelerated)
- **Button:** `OneButton`
- **Storage:** `SdFat` (Adafruit fork)
- **RTC:** `makuna/RTC`
- **Sensor:** `DHT sensor library`

---
*Generated context for AI. Regenerate after major structural changes.*
