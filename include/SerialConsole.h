#pragma once
#include "RTCManager.h"

class SerialConsole {
public:
    explicit SerialConsole(RTCManager& rtc);
    void update();

private:
    RTCManager& _rtc;
    char _line[48];
    uint8_t _length = 0;

    void run(const char* line);
};
