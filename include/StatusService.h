#pragma once
#include "DeviceStatus.h"
#include "RTCManager.h"
#include "DHTManager.h"

enum class SensorState : uint8_t { Ok, Stale, Error, OutOfRange, Absent };

class StatusService {
public:
    static constexpr uint8_t errorAfterFailures = 3;

    StatusService(RTCManager& rtc, DHTManager& dht);
    void setSdOk(bool ok);
    void update();
    const DeviceStatus& status() const { return _status; }

    void setSensorTuning(uint32_t intervalMs, int16_t tempOffsetTenths, int16_t humidityOffsetTenths);
    void reinitSensor();
    SensorState sensorState() const;
    uint8_t sensorFailures() const { return _failures; }
    bool sensorEverRead() const { return _everRead; }
    uint32_t sensorLastGoodMs() const { return _lastGoodAt; }

private:
    RTCManager& _rtc;
    DHTManager& _dht;
    DeviceStatus _status;
    uint32_t _lastClockRead = 0;
    uint32_t _lastClimateRead = 0;
    uint32_t _lastWifiRead = 0;
    bool _firstRun = true;

    uint32_t _climateIntervalMs = 30000;
    int16_t _tempOffsetTenths = 0;
    int16_t _humidityOffsetTenths = 0;
    uint8_t _failures = 0;
    bool _everRead = false;
    bool _outOfRange = false;
    bool _jumpRejected = false;
    bool _forceClimate = false;
    uint32_t _lastGoodAt = 0;
    float _lastRawTemperature = 0;

    void readClimate();
};
