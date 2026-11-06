#pragma once
#include <Arduino.h>
#include "DeviceStatus.h"

enum class FocusState : uint8_t { Idle, Running, Paused };

class FocusSession {
public:
    static constexpr uint8_t weekDays = 7;

    FocusSession(const DeviceStatus& status, uint16_t goalMinutes);

    void seedWeek(const uint16_t (&minutes)[weekDays]);
    void update();

    void start();
    void pause();
    void resume();
    void end();

    FocusState state() const { return _state; }
    uint32_t elapsedSeconds() const;
    uint32_t goalSeconds() const { return _goalSeconds; }
    uint32_t todaySeconds() const;
    uint32_t daySeconds(uint8_t dayIndex) const;
    uint32_t weekSeconds() const;
    uint8_t sessionsToday() const { return _sessionsToday; }
    uint8_t todayIndex() const { return _todayIndex; }
    int32_t weekStartDay() const { return _weekStart; }

private:
    const DeviceStatus& _status;
    uint32_t _goalSeconds;

    FocusState _state = FocusState::Idle;
    uint32_t _accumulatedMs = 0;
    uint32_t _runStartedAt = 0;

    uint32_t _historySeconds[weekDays] = {};
    uint16_t _seedMinutes[weekDays] = {};
    bool _seedPending = false;

    bool _dateKnown = false;
    int32_t _today = 0;
    int32_t _weekStart = 0;
    uint8_t _todayIndex = 0;
    uint8_t _sessionsToday = 0;
};
