#pragma once
#include "RTCManager.h"
#include "WifiService.h"

class SerialConsole {
public:
    SerialConsole(RTCManager& rtc, const WifiService& wifi);
    void update();

private:
    RTCManager& _rtc;
    const WifiService& _wifi;
    char _line[48];
    uint8_t _length = 0;

    void run(const char* line);
};
