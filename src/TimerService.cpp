#include "TimerService.h"

void Countdown::start(uint32_t seconds) {
    _remainingMs = seconds * 1000UL;
    _lastTick = millis();
    _state = _remainingMs ? State::Running : State::Done;
}

void Countdown::update() {
    if (_state != State::Running) return;
    uint32_t now = millis();
    uint32_t delta = now - _lastTick;
    _lastTick = now;
    if (delta >= _remainingMs) {
        _remainingMs = 0;
        _state = State::Done;
    } else {
        _remainingMs -= delta;
    }
}

void Countdown::pause() {
    update();
    if (_state == State::Running) _state = State::Paused;
}

void Countdown::resume() {
    if (_state != State::Paused) return;
    _lastTick = millis();
    _state = State::Running;
}

void Countdown::stop() {
    _state = State::Idle;
    _remainingMs = 0;
}

void TimerService::update() {
    _timer.update();
    _pomo.update();
    if (_pomo.state() == Countdown::State::Done) advancePomodoro();
}

TimerState TimerService::timerState() const {
    switch (_timer.state()) {
        case Countdown::State::Running: return TimerState::Running;
        case Countdown::State::Paused: return TimerState::Paused;
        case Countdown::State::Done: return TimerState::Done;
        default: return TimerState::Set;
    }
}

uint32_t TimerService::timerSeconds() const {
    if (_timer.state() == Countdown::State::Idle) return _timerSetSeconds;
    return _timer.remainingSeconds();
}

void TimerService::setTimerSetSeconds(uint32_t seconds) {
    if (seconds < minTimerSeconds) seconds = minTimerSeconds;
    if (seconds > maxTimerSeconds) seconds = maxTimerSeconds;
    _timerSetSeconds = seconds;
}

void TimerService::timerStart() {
    if (_timer.state() == Countdown::State::Idle) _timer.start(_timerSetSeconds);
}

void TimerService::timerPause() { _timer.pause(); }
void TimerService::timerResume() { _timer.resume(); }
void TimerService::timerEnd() { _timer.stop(); }

void TimerService::timerAgain() {
    if (_timer.state() == Countdown::State::Done) _timer.start(_timerSetSeconds);
}

PomoState TimerService::pomoState() const {
    switch (_pomo.state()) {
        case Countdown::State::Paused: return PomoState::Paused;
        case Countdown::State::Idle: return PomoState::Set;
        default: return PomoState::Running;
    }
}

uint32_t TimerService::pomoSeconds() const {
    if (_pomo.state() == Countdown::State::Idle) return _focusMinutes * 60UL;
    return _pomo.remainingSeconds();
}

void TimerService::setFocusMinutes(int minutes) {
    if (minutes < minFocusMinutes) minutes = minFocusMinutes;
    if (minutes > maxFocusMinutes) minutes = maxFocusMinutes;
    _focusMinutes = minutes;
}

void TimerService::pomoStart() {
    if (_pomo.state() != Countdown::State::Idle) return;
    _round = 1;
    _phase = PomoPhase::Focus;
    _pomo.start(_focusMinutes * 60UL);
}

void TimerService::pomoPause() { _pomo.pause(); }
void TimerService::pomoResume() { _pomo.resume(); }

void TimerService::pomoEnd() {
    _pomo.stop();
    _round = 1;
    _phase = PomoPhase::Focus;
}

void TimerService::advancePomodoro() {
    switch (_phase) {
        case PomoPhase::Focus:
            if (_round < rounds) {
                _phase = PomoPhase::Break;
                _pomo.start(shortBreakMinutes * 60UL);
            } else {
                _phase = PomoPhase::LongBreak;
                _pomo.start(longBreakMinutes * 60UL);
            }
            break;
        case PomoPhase::Break:
            _round++;
            _phase = PomoPhase::Focus;
            _pomo.start(_focusMinutes * 60UL);
            break;
        case PomoPhase::LongBreak:
            pomoEnd();
            break;
    }
}
