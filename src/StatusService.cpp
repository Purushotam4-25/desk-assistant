#include "StatusService.h"
#include <WiFi.h>
#include <math.h>

namespace {
constexpr uint32_t clockIntervalMs = 200;
constexpr uint32_t climateIntervalMs = 3000;
constexpr uint32_t wifiIntervalMs = 1000;
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

    if (_firstRun || now - _lastClimateRead >= climateIntervalMs) {
        _lastClimateRead = now;
        float t = _dht.getTemperature();
        float h = _dht.getHumidity();
        _status.hasClimate = !isnan(t) && !isnan(h);
        if (_status.hasClimate) {
            _status.temperatureC = t;
            _status.humidity = h;
        }
    }

    if (_firstRun || now - _lastWifiRead >= wifiIntervalMs) {
        _lastWifiRead = now;
        _status.wifiConnected = WiFi.status() == WL_CONNECTED;
    }

    _firstRun = false;
}
