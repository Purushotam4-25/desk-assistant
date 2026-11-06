#include "FocusSession.h"
#include "DateMath.h"

namespace {
constexpr uint32_t minSessionSeconds = 60;
}

FocusSession::FocusSession(const DeviceStatus& status, uint16_t goalMinutes)
    : _status(status), _goalSeconds(goalMinutes * 60UL) {}

void FocusSession::seedWeek(const uint16_t (&minutes)[weekDays]) {
    for (uint8_t i = 0; i < weekDays; i++) _seedMinutes[i] = minutes[i];
    _seedPending = true;
}

void FocusSession::update() {
    const DateTime& now = _status.now;
    int32_t days = Date::daysFromCivil(now.year, now.month, now.day);
    if (_dateKnown && days == _today) return;

    int32_t weekStart = days - Date::mondayIndex(days);
    bool firstDate = !_dateKnown;

    if (firstDate || weekStart != _weekStart) {
        for (uint8_t i = 0; i < weekDays; i++) _historySeconds[i] = 0;
        _weekStart = weekStart;
    }

    _today = days;
    _todayIndex = days - weekStart;
    _sessionsToday = 0;
    _dateKnown = true;

    if (firstDate && _seedPending) {
        for (uint8_t i = 0; i < _todayIndex; i++) _historySeconds[i] = _seedMinutes[i] * 60UL;
        _seedPending = false;
    }
}

void FocusSession::start() {
    if (_state != FocusState::Idle) return;
    _accumulatedMs = 0;
    _runStartedAt = millis();
    _state = FocusState::Running;
}

void FocusSession::pause() {
    if (_state != FocusState::Running) return;
    _accumulatedMs += millis() - _runStartedAt;
    _state = FocusState::Paused;
}

void FocusSession::resume() {
    if (_state != FocusState::Paused) return;
    _runStartedAt = millis();
    _state = FocusState::Running;
}

void FocusSession::end() {
    if (_state == FocusState::Idle) return;
    uint32_t seconds = elapsedSeconds();
    _historySeconds[_todayIndex] += seconds;
    if (seconds >= minSessionSeconds && _sessionsToday < 255) _sessionsToday++;
    _accumulatedMs = 0;
    _state = FocusState::Idle;
}

uint32_t FocusSession::elapsedSeconds() const {
    uint32_t ms = _accumulatedMs;
    if (_state == FocusState::Running) ms += millis() - _runStartedAt;
    return ms / 1000;
}

uint32_t FocusSession::todaySeconds() const {
    return daySeconds(_todayIndex);
}

uint32_t FocusSession::daySeconds(uint8_t dayIndex) const {
    uint32_t seconds = _historySeconds[dayIndex];
    if (dayIndex == _todayIndex) seconds += elapsedSeconds();
    return seconds;
}

uint32_t FocusSession::weekSeconds() const {
    uint32_t total = 0;
    for (uint8_t i = 0; i < weekDays; i++) total += daySeconds(i);
    return total;
}
