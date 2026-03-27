# Gemini CLI Project Context

This document consolidates all relevant information about the "ESP32 Desk Assistant" project, gathered from user interactions and provided handoff messages.

## 1. High-Level Goal

Build a **modular ESP32-based desk assistant** with a display, rotary encoder navigation, real-time clock, sensors, SD storage, and simple audio feedback. The project prioritises learning, clean modular structure, and easy extensibility. The device sits under a monitor, angled slightly upward.

## 2. Hardware Components & Pin Assignments

### Core Components
*   **ESP32 Dev Board** (PlatformIO / Arduino framework)
*   **2.4” TFT SPI Display (ILI9341)** – primary screen
*   **Rotary Encoder with Push Button** – main UI input
*   **HW-203 SD Card Module** (AMS1117 onboard regulator)
*   **DS1302 RTC module** (coin cell backed)
*   **DHT11 temperature & humidity sensor**
*   **Speaker + small amplifier module** (for alerts only, not music)
*   *(Optional future)* PCF8574 GPIO expander (DIP-16)

### Power & Voltage Philosophy
*   **ESP32 logic is 3.3V ONLY.**
*   Almost everything runs on **3.3V**.
*   Only **speaker + amp** and optionally **SD module** use **5V**.
*   Grounds are always shared.
*   ESP32 is powered via USB or VIN (5V). 5V peripherals are powered by branching the same 5V line.

### Confirmed GPIO Assignments

| Component          | Signal        | GPIO | Notes                                |
| :----------------- | :------------ | :--- | :----------------------------------- |
| **SD Card (SPI)**  | CS            | 5    |                                      |
|                    | MOSI          | 23   | Shared with TFT MOSI                 |
|                    | SCK           | 18   | Shared with TFT SCK                  |
|                    | MISO          | 19   |                                      |
| **Sensors**        | DHT11 DATA    | 4    |                                      |
|                    | DS1302 CLK    | 27   |                                      |
|                    | DS1302 DAT    | 26   |                                      |
|                    | DS1302 RST    | 25   |                                      |
| **Rotary Encoder** | CLK           | 32   | Confirmed from `src/main.cpp`        |
|                    | DT            | 33   | Confirmed from `src/main.cpp`        |
|                    | SW (button)   | 15   | Confirmed from `src/main.cpp`        |
| **TFT (ILI9341)**  | VCC           | 3.3V |                                      |
|                    | GND           | GND  |                                      |
|                    | MOSI / SDI    | 23   | Shared with SD Card MOSI             |
|                    | SCK           | 18   | Shared with SD Card SCK              |
|                    | CS            | 13   |                                      |
|                    | DC            | 17   |                                      |
|                    | RESET         | 16   |                                      |
|                    | LED (backlight) | 3.3V |                                      |
|                    | MISO / SDO    | NC   | **NOT connected** (write-only display)|

## 3. Libraries Used / Intended

*   **Display:** `TFT_eSPI` (handles ILI9341, supports SD card fonts)
*   **Input:** `ESP32Encoder` (hardware-accelerated), `OneButton` (button handling)
*   **Storage:** `SD` / `SD_MMC`
*   **Other:** `SdFat` (Adafruit Fork), `Adafruit SSD1306`, `Adafruit GFX Library`, `DHT sensor library`, `makuna/RTC`, `olikraus/U8g2`, `gpio_viewer`, `AsyncTCP`, `ESPAsyncWebServer` (from `platformio.ini`)

## 4. Fonts & Assets

*   Fonts are TTF, converted to `.vlw` format (TFT_eSPI compatible).
*   Stored **on the SD card**, not in flash, for dynamic loading and easy updates.
*   **SD Card Structure (Current / Intended):**
    ```
    /SD
     ├── fonts/
     │    ├── Inter-Regular-16.vlw
     │    ├── Inter-Bold-24.vlw
     │    └── Mono-14.vlw
     ├── images/
     │    ├── icon_weather.bmp
     │    └── icon_battery.bmp
     └── data/
          └── (future use)
    ```

## 5. Software Architecture (`bubblewrap`)

Modular C++ project, not a single `.ino` sketch.

*   **`main.cpp`:** Entry point, initializes screen, SD card, encoder, ScreenManager. Calls update loops.
*   **`RotaryEncoderManager`:** Abstracts encoder logic (getDelta, button events).
*   **`ScreenManager` (planned):** Owns TFT instance and active page. Routes encoder input and draw calls. Handles page switching.
*   **`BasePage` (planned):** Abstract parent class defining common interface (`onEnter()`, `onExit()`, `update()`, `draw()`).
*   **Individual Pages (e.g., `HomePage`, `SettingsPage`):** Implement UI logic, drawing, and reaction to encoder movement.

## 6. UI & Navigation Model (`emptyroll`)

*   **Page-based UI:** Only one page is active at a time.
*   **Rotary encoder:**
    *   Rotate → scroll / change selection
    *   Single click → select / enter
    *   Double click → return to home
    *   Long press → open settings / global menu
*   **Planned UI States:** `HOME`, `CLOCK`, `SENSOR`, `TIMER`, `IDLE`, `SETTINGS`. Each state has its own draw function.
*   Non-blocking delays (`delay()` avoided).

## 7. Audio Use

*   Only for **alerts** (timer finished, notification). Not music.
*   Output via **ESP32 DAC** → amplifier input. No speaker directly connected to GPIO.

## 8. Current Status

*   All components understood and wired previously.
*   Project paused, user needs to **cleanly resume**.
*   Rotary encoder code exists and works.  
*   SD card file structure set up; `.vlw` font files generated and uploaded.
*   **User wants to:**
    *   Restart cleanly.
    *   Understand **what every piece of code does**.
    *   Rebuild confidence step by step.
    *   Detailed explanations, file-by-file breakdowns, no unexplained abstractions.

## 9. Immediate Next Steps (User's Proposed Order)

1.  Test **Display** bring-up.
2.  Test **Encoder** input handling.
3.  Implement **UI state machine**.
4.  Implement **Sensor display pages**.

## 10. Platform

*   ESP32
*   PlatformIO preferred
*   Arduino framework
