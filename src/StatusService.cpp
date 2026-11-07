#include "StatusService.h"
#include <WiFi.h>
#include <math.h>

namespace {
constexpr uint32_t clockIntervalMs = 200;
constexpr uint32_t wifiIntervalMs = 1000;
constexpr float maxJumpC = 10.0f;
constexpr uint32_t staleIntervals = 3;
}

StatusService::StatusService(RTCManager& rtc, DHTManager& dht) : _rtc(rtc), _dht(dht) {}

void StatusService::setSdOk(bool ok) {
    _status.sdOk = ok;
}

void StatusService::update() {
    uint32_t now = millis();

    if (_firstRun || now - _lastClockRead >= clockIntervalMs) {
        _lastClockRead = now;
        _status.now = _rtc.getDateTime();
    }

    if (_firstRun || _forceClimate || now - _lastClimateRead >= _climateIntervalMs) {
        _lastClimateRead = now;
        _forceClimate = false;
        readClimate();
    }

    if (_firstRun || now - _lastWifiRead >= wifiIntervalMs) {
        _lastWifiRead = now;
        _status.wifiConnected = WiFi.status() == WL_CONNECTED;
    }

    _firstRun = false;
}

void StatusService::setSensorTuning(uint32_t intervalMs, int16_t tempOffsetTenths, int16_t humidityOffsetTenths) {
    _climateIntervalMs = intervalMs;
    _tempOffsetTenths = tempOffsetTenths;
    _humidityOffsetTenths = humidityOffsetTenths;
}

void StatusService::reinitSensor() {
    _dht.begin();
    _failures = 0;
    _jumpRejected = false;
    _forceClimate = true;
}

void StatusService::readClimate() {
    float t = _dht.getTemperature();
    float h = _dht.getHumidity();

    if (isnan(t) || isnan(h)) {
        if (_failures < 255) _failures++;
    } else if (t < -40.0f || t > 80.0f || h < 0.0f || h > 100.0f) {
        _outOfRange = true;
        _failures = 0;
    } else if (_everRead && fabsf(t - _lastRawTemperature) > maxJumpC && !_jumpRejected) {
        _jumpRejected = true;
    } else {
        _jumpRejected = false;
        _outOfRange = false;
        _failures = 0;
        _everRead = true;
        _lastGoodAt = millis();
        _lastRawTemperature = t;
        _status.temperatureC = t + _tempOffsetTenths / 10.0f;
        float adjusted = h + _humidityOffsetTenths / 10.0f;
        _status.humidity = adjusted < 0.0f ? 0.0f : (adjusted > 100.0f ? 100.0f : adjusted);
    }
    _status.hasClimate = _everRead && _failures < errorAfterFailures;
}

SensorState StatusService::sensorState() const {
    if (_failures >= errorAfterFailures) return _everRead ? SensorState::Error : SensorState::Absent;
    if (_outOfRange) return SensorState::OutOfRange;
    if (_everRead && millis() - _lastGoodAt > _climateIntervalMs * staleIntervals) return SensorState::Stale;
    return SensorState::Ok;
}
