#pragma once
#include <Arduino.h>
#include <atomic>
#include "Agenda.h"
#include "ConfigService.h"
#include "DeviceStatus.h"
#include "WifiService.h"

class CalendarService {
public:
    enum class State : uint8_t { NoBridge, WaitingForWifi, Fetching, Ok, Error };

    CalendarService(const ConfigService& config, const WifiService& wifi);
    void begin();
    void update(Agenda& target, const DeviceStatus& status);
    void refreshNow();

    State state() const { return _state; }
    bool everOk() const { return _lastOkMs != 0; }
    uint32_t lastOkMs() const { return _lastOkMs; }
    const char* lastError() const { return _error; }
    size_t eventCount() const { return _shownCount; }

private:
    const ConfigService& _config;
    const WifiService& _wifi;
    State _state = State::NoBridge;
    uint32_t _lastOkMs = 0;
    uint32_t _nextFetchMs = 0;
    bool _forceNext = false;
    size_t _shownCount = 0;
    char _error[24] = "";

    char _url[112] = "";
    AgendaEvent _result[agendaCapacity] = {};
    size_t _resultCount = 0;
    uint32_t _resultDate = 0;
    char _taskError[24] = "";
    std::atomic<bool> _fetchRequested{false};
    std::atomic<bool> _resultReady{false};
    std::atomic<bool> _resultOk{false};

    uint32_t intervalMs() const;
    void startFetch();
    void finishFetch(Agenda& target, uint32_t todayKey);
    void fetch();
    static uint32_t dateKey(const DateTime& now);
    static void taskMain(void* self);
};
