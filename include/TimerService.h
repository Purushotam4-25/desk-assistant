#pragma once
#include <Arduino.h>

class Countdown {
public:
    enum class State : uint8_t { Idle, Running, Paused, Done };

    void start(uint32_t seconds);
    void pause();
    void resume();
    void stop();
    void update();

    State state() const { return _state; }
    uint32_t remainingSeconds() const { return (_remainingMs + 999) / 1000; }

private:
    State _state = State::Idle;
    uint32_t _remainingMs = 0;
    uint32_t _lastTick = 0;
};

enum class TimerState : uint8_t { Set, Running, Paused, Done };
enum class PomoState : uint8_t { Set, Running, Paused };
enum class PomoPhase : uint8_t { Focus, Break, LongBreak };

class TimerService {
public:
    static constexpr int rounds = 4;
    static constexpr uint32_t minTimerSeconds = 1, maxTimerSeconds = 99 * 60 + 59;
    static constexpr int minFocusMinutes = 1, maxFocusMinutes = 60;

    void update();

    TimerState timerState() const;
    uint32_t timerSeconds() const;
    uint32_t timerSetSeconds() const { return _timerSetSeconds; }
    void setTimerSetSeconds(uint32_t seconds);
    void timerStart();
    void timerPause();
    void timerResume();
    void timerEnd();
    void timerAgain();

    PomoState pomoState() const;
    PomoPhase pomoPhase() const { return _phase; }
    int pomoRound() const { return _round; }
    uint32_t pomoSeconds() const;
    int focusMinutes() const { return _focusMinutes; }
    void setFocusMinutes(int minutes);
    int shortBreakMinutes() const { return _shortBreakMinutes; }
    int longBreakMinutes() const { return _longBreakMinutes; }
    void setBreakMinutes(int shortMinutes, int longMinutes);
    void pomoStart();
    void pomoPause();
    void pomoResume();
    void pomoEnd();

private:
    Countdown _timer;
    Countdown _pomo;
    uint32_t _timerSetSeconds = 10 * 60;
    int _focusMinutes = 25;
    int _shortBreakMinutes = 5;
    int _longBreakMinutes = 15;
    int _round = 1;
    PomoPhase _phase = PomoPhase::Focus;

    void advancePomodoro();
};
