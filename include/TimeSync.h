#pragma once
#include "RTCManager.h"
#include "ConfigService.h"
#include "WifiService.h"

class TimeSync {
public:
    TimeSync(RTCManager& rtc, const ConfigService& config, const WifiService& wifi);
    void update();
    bool synced() const { return _synced; }
    uint32_t lastSyncMs() const { return _lastSyncAt; }

private:
    RTCManager& _rtc;
    const ConfigService& _config;
    const WifiService& _wifi;
    bool _started = false;
    bool _synced = false;
    uint32_t _nextReport = 0;
    uint32_t _lastCheck = 0;
    uint32_t _lastSyncAt = 0;

    void check();
    void startSntp();
    void reportStalled();
};
