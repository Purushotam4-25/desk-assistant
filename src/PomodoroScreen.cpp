#include "PomodoroScreen.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"

extern TFT_eSPI tft;

namespace {
constexpr int nodeSpacing = 72, nodeY = 44, nodeRadius = 7, linkInset = 10;
constexpr int trackLabelCenterY = 23, trackClearY = 12, trackClearH = 44;
constexpr int footerCenterY = 196, footerClearY = 186, footerClearH = 22, footerGap = 12;
}

PomodoroScreen::PomodoroScreen(const NavState& nav, TimerService& service) : TimerFace(nav, service) {}

FaceMode PomodoroScreen::mode() const {
    switch (_service.pomoState()) {
        case PomoState::Running: return FaceMode::Running;
        case PomoState::Paused: return FaceMode::Paused;
        default: return FaceMode::Set;
    }
}

uint32_t PomodoroScreen::seconds() const { return _service.pomoSeconds(); }
uint32_t PomodoroScreen::setSeconds() const { return _service.focusMinutes() * 60UL; }
void PomodoroScreen::applySetSeconds(uint32_t total) { _service.setFocusMinutes(total / 60); }
uint32_t PomodoroScreen::minSetSeconds() const { return TimerService::minFocusMinutes * 60UL; }
uint32_t PomodoroScreen::maxSetSeconds() const { return TimerService::maxFocusMinutes * 60UL; }

void PomodoroScreen::primary() {
    switch (mode()) {
        case FaceMode::Set: _service.pomoStart(); break;
        case FaceMode::Running: _service.pomoPause(); break;
        default: _service.pomoResume(); break;
    }
}

void PomodoroScreen::end() { _service.pomoEnd(); }

void PomodoroScreen::resetExtraKeys() {
    _trackKey = _footerKey = -1;
}

int PomodoroScreen::trackStep() const {
    if (_service.pomoState() == PomoState::Set) return 0;
    switch (_service.pomoPhase()) {
        case PomoPhase::Focus: return _service.pomoRound() - 1;
        case PomoPhase::Break: return _service.pomoRound();
        default: return TimerService::rounds - 1;
    }
}

void PomodoroScreen::drawTop() {
    int step = trackStep();
    int length = _service.focusMinutes();
    int32_t key = step * 128 + length;
    if (key == _trackKey) return;
    _trackKey = key;

    tft.fillRect(0, trackClearY, 320, trackClearH, Theme::background);

    int first = 160 - ((TimerService::rounds - 1) * nodeSpacing) / 2;
    char label[4];
    snprintf(label, sizeof(label), "%d", length);
    int labelY = trackLabelCenterY - Fonts::capCentre(Fonts::body, 1, '0');

    for (int i = 0; i < TimerService::rounds; i++) {
        int centerX = first + i * nodeSpacing;

        if (i < TimerService::rounds - 1) {
            tft.fillRect(centerX + linkInset, nodeY - 1, nodeSpacing - 2 * linkInset, 2,
                         i < step ? Theme::accent : Theme::outline);
        }

        if (i < step) {
            tft.fillCircle(centerX, nodeY, nodeRadius, Theme::accent);
        } else if (i == step) {
            tft.fillCircle(centerX, nodeY, nodeRadius, Theme::text);
            tft.fillCircle(centerX, nodeY, 4, Theme::accent);
        } else {
            tft.drawCircle(centerX, nodeY, nodeRadius, Theme::outline);
            tft.drawCircle(centerX, nodeY, nodeRadius - 1, Theme::outline);
        }

        Widgets::inkTextCentered(label, centerX, labelY, Fonts::body, 1,
                                 i == step ? Theme::text : Theme::textMuted);
    }
}

void PomodoroScreen::drawFooter() {
    PomoState state = _service.pomoState();
    PomoPhase phase = _service.pomoPhase();
    int round = _service.pomoRound();
    int32_t key = ((int)state * 3 + (int)phase) * 8 + round;
    if (key == _footerKey) return;
    _footerKey = key;

    const char* name = "Focus";
    uint16_t color = Theme::text;
    if (state == PomoState::Paused) {
        name = "Paused";
        color = Theme::accent;
    } else if (phase == PomoPhase::Break) {
        name = "Break";
        color = Theme::good;
    } else if (phase == PomoPhase::LongBreak) {
        name = "Long break";
        color = Theme::good;
    }

    char count[8];
    snprintf(count, sizeof(count), "%d/%d", round, TimerService::rounds);
    int nameWidth = Widgets::inkWidth(name, Fonts::body, 1);
    int countWidth = Widgets::inkWidth(count, Fonts::body, 1);
    int x = 160 - (nameWidth + footerGap + countWidth) / 2;
    int y = footerCenterY - Fonts::capCentre(Fonts::body, 1, 'F');

    tft.fillRect(0, footerClearY, 320, footerClearH, Theme::background);
    Widgets::inkText(name, x, y, Fonts::body, 1, color);
    Widgets::inkText(count, x + nameWidth + footerGap, y, Fonts::body, 1, Theme::textMuted);
}
