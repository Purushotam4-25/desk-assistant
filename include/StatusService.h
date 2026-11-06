#pragma once
#include "DeviceStatus.h"
#include "RTCManager.h"
#include "DHTManager.h"

class StatusService {
public:
    StatusService(RTCManager& rtc, DHTManager& dht);
    void setSdOk(bool ok);
    void update();
    const DeviceStatus& status() const { return _status; }

private:
    RTCManager& _rtc;
    DHTManager& _dht;
    DeviceStatus _status;
    uint32_t _lastClockRead = 0;
    uint32_t _lastClimateRead = 0;
    uint32_t _lastWifiRead = 0;
    bool _firstRun = true;
};
