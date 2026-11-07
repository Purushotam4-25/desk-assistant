#pragma once
#include <Arduino.h>
#include <atomic>
#include "ConfigService.h"
#include "WifiService.h"
#include "WeatherData.h"

class WeatherService {
public:
    enum class State : uint8_t { NoBridge, WaitingForWifi, Fetching, Ok, Error };

    WeatherService(const ConfigService& config, const WifiService& wifi);
    void begin();
    void update(WeatherData& target);
    void refreshNow();

    State state() const { return _state; }
    bool everOk() const { return _lastOkMs != 0; }
    uint32_t lastOkMs() const { return _lastOkMs; }
    const char* lastError() const { return _error; }

private:
    const ConfigService& _config;
    const WifiService& _wifi;
    State _state = State::NoBridge;
    uint32_t _lastOkMs = 0;
    uint32_t _nextFetchMs = 0;
    bool _forceNext = false;
    char _error[24] = "";

    char _url[112] = "";
    WeatherData _result;
    char _taskError[24] = "";
    std::atomic<bool> _fetchRequested{false};
    std::atomic<bool> _resultReady{false};
    std::atomic<bool> _resultOk{false};

    uint32_t intervalMs() const;
    void startFetch();
    void finishFetch(WeatherData& target);
    void fetch();
    static void taskMain(void* self);
};
