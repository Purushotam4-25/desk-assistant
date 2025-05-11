# ESP32 Desk Assistant

An all-in-one smart desk companion built with an ESP32. Includes an OLED UI, SD card storage, RTC timekeeping, and more features coming soon!

## Features

- **OLED Display (128x64 0.96" OLED)**  
  Main screen with time, status, and system info

- **SD Card Support (HW-203)**  
  Stores configuration, logs, icons, and media files

- **RTC Module (DS1302)**  
  Keeps accurate time using a coin cell battery backup

- **Modular Codebase**  
  Organized with `.h` and `.cpp` files for each component

- **Voice & AI Integration (Coming Soon)**  
  Future plans to support voice recording and smart assistant features

## Hardware Used

| Component      | Model             |
|----------------|------------------|
| Microcontroller | ESP32 Dev Board  |
| Display        | SSD1306 OLED 128x64 |
| RTC Module     | DS1302            |
| Rotary Encoder | Null | 
| SD Card Module | HW-203 (with AMS1117) |
| Temp and Humid | DHT11 |
| Power Supply   | power system mentioned below |
| Battery        | CR2032 coin cell (RTC backup) |

## Wiring

PINOUT to be filled

## Libraries Used

- [`Makuna/RTC`](https://github.com/Makuna/Rtc) for DS1302
- `U8g2` library for OLED
- `SdFat` library
- `SPI`, `Wire` (standard)

## Hardware Used

