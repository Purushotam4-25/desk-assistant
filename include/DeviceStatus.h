#pragma once
#include "RTCManager.h"

struct DeviceStatus {
    DateTime now = {2000, 1, 1, 6, 0, 0, 0};
    bool wifiConnected = false;
    bool sdOk = false;
    bool hasClimate = false;
    float temperatureC = 0;
    float humidity = 0;

    uint16_t minuteOfDay() const { return now.hour * 60 + now.minute; }
};
