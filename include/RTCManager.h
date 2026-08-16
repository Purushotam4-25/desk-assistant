#pragma once
#include <Arduino.h>
#include <RtcDS1302.h>
#include <ThreeWire.h>

class RTCManager {
public:
    RTCManager(uint8_t clkPin, uint8_t datPin, uint8_t rstPin);
    void begin();

    String getTime();
    String getDate();
    void setDateTime(int year, int month, int day, int hour, int min, int sec);

private:
    ThreeWire _wire;
    RtcDS1302<ThreeWire> _rtc;
};
