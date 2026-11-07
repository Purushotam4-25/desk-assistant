#pragma once
#include "RTCManager.h"
#include "ConfigService.h"
#include "WifiService.h"

class TimeSync {
public:
    TimeSync(RTCManager& rtc, const ConfigService& config, const WifiService& wifi);
    void update();

private:
    RTCManager& _rtc;
    const ConfigService& _config;
    const WifiService& _wifi;
    bool _started = false;
    bool _synced = false;
    uint32_t _nextReport = 0;
    uint32_t _lastCheck = 0;

    void check();
    void startSntp();
    void reportStalled();
};
