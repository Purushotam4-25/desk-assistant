#pragma once
#include <Arduino.h>
#include <RtcDS1302.h>
#include <ThreeWire.h>

struct DateTime {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

class RTCManager {
public:
    RTCManager(uint8_t clkPin, uint8_t datPin, uint8_t rstPin);
    void begin();

    DateTime getDateTime();
    String getTime();
    String getDate();
    void setDateTime(int year, int month, int day, int hour, int min, int sec);

private:
    ThreeWire _wire;
    RtcDS1302<ThreeWire> _rtc;
};
