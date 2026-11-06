#include "TimerScreen.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"

extern TFT_eSPI tft;

namespace {
constexpr int labelCenterY = 40, labelClearY = 26, labelClearH = 30, labelScale = 2;
}

TimerScreen::TimerScreen(const NavState& nav, TimerService& service) : TimerFace(nav, service) {}

FaceMode TimerScreen::mode() const {
    switch (_service.timerState()) {
        case TimerState::Running: return FaceMode::Running;
        case TimerState::Paused: return FaceMode::Paused;
        case TimerState::Done: return FaceMode::Done;
        default: return FaceMode::Set;
    }
}

uint32_t TimerScreen::seconds() const { return _service.timerSeconds(); }
uint32_t TimerScreen::setSeconds() const { return _service.timerSetSeconds(); }
void TimerScreen::applySetSeconds(uint32_t total) { _service.setTimerSetSeconds(total); }
uint32_t TimerScreen::minSetSeconds() const { return TimerService::minTimerSeconds; }
uint32_t TimerScreen::maxSetSeconds() const { return TimerService::maxTimerSeconds; }

void TimerScreen::primary() {
    switch (mode()) {
        case FaceMode::Set: _service.timerStart(); break;
        case FaceMode::Running: _service.timerPause(); break;
        case FaceMode::Paused: _service.timerResume(); break;
        case FaceMode::Done: _service.timerAgain(); break;
    }
}

void TimerScreen::end() { _service.timerEnd(); }

void TimerScreen::resetExtraKeys() {
    _labelKey = -1;
}

void TimerScreen::drawTop() {
    FaceMode m = mode();
    int32_t key = m == FaceMode::Paused ? 1 : m == FaceMode::Done ? 2 : 0;
    if (key == _labelKey) return;
    _labelKey = key;

    const char* label = key == 1 ? "Paused" : key == 2 ? "Time's up" : "Timer";
    uint16_t color = key == 1 ? Theme::accent : key == 2 ? Theme::good : Theme::textMuted;
    tft.fillRect(0, labelClearY, 320, labelClearH, Theme::background);
    int y = labelCenterY - Fonts::capCentre(Fonts::title, labelScale, 'T');
    Widgets::inkTextCentered(label, 160, y, Fonts::title, labelScale, color);
}
