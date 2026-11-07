#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "ConfigService.h"

class WifiService {
public:
    enum class State : uint8_t { Off, Connecting, Connected, Waiting };

    void begin(const Config& config);
    void update();
    State state() const { return _state; }
    bool connected() const { return _state == State::Connected; }
    const char* stateName() const;

private:
    const Config* _config = nullptr;
    State _state = State::Off;
    uint32_t _stateSince = 0;
    uint32_t _retryDelayMs = 0;

    void setState(State state);
    void startAttempt();
    void failAttempt(wl_status_t status);
};
