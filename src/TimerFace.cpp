#include "TimerFace.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Widgets.h"
#include "ListFocus.h"

extern TFT_eSPI tft;

namespace {
constexpr uint32_t fastSpinMs = 80;
constexpr int fastStep = 5;
}

TimerFace::TimerFace(const NavState& nav, TimerService& service) : SectionScreen(nav), _service(service) {}

void TimerFace::resetKeys() {
    _groupKeys[0] = _groupKeys[1] = -1;
    _colonKey = _underlineKey = _layoutKey = _primaryKey = _endKey = -1;
    resetExtraKeys();
}

int TimerFace::fieldCount() const {
    if (mode() != FaceMode::Set) return 0;
    return secondsEditable() ? 2 : 1;
}

int TimerFace::itemCount() const {
    return mode() == FaceMode::Set ? fieldCount() + 1 : 2;
}

int TimerFace::primaryIndex() const {
    return mode() == FaceMode::Set ? fieldCount() : 0;
}

void TimerFace::init() {
    SectionScreen::init();
    if (_editing) {
        applySetSeconds(_editOriginal);
        _editing = false;
    }
    _readout = TimerUi::makeReadout();
    _seenMode = -1;
    syncMode();
}

void TimerFace::update() {
    syncMode();
}

void TimerFace::syncMode() {
    int current = (int)mode();
    if (current == _seenMode) return;
    _seenMode = current;
    _focus = primaryIndex();
    _editing = false;
}

void TimerFace::adjust(int direction) {
    uint32_t now = millis();
    int step = now - _lastEditMs < fastSpinMs ? fastStep : 1;
    _lastEditMs = now;

    uint32_t total = setSeconds();
    int minutes = total / 60;
    int secs = total % 60;
    int minMinutes = minSetSeconds() / 60;
    int maxMinutes = maxSetSeconds() / 60;

    if (_editField == 0) {
        minutes += direction * step;
        if (minutes < minMinutes) minutes = minMinutes;
        if (minutes > maxMinutes) minutes = maxMinutes;
    } else {
        secs += direction * step;
        if (secs < 0) secs = 0;
        if (secs > 59) secs = 59;
    }

    int32_t next = minutes * 60 + secs;
    if (next < (int32_t)minSetSeconds()) next = minSetSeconds();
    if (next > (int32_t)maxSetSeconds()) next = maxSetSeconds();
    applySetSeconds(next);
}

void TimerFace::handleInput(Input input) {
    if (_editing) {
        if (input == Input::FocusNext) adjust(1);
        if (input == Input::FocusPrev) adjust(-1);
        if (input == Input::Select) _editing = false;
        return;
    }
    if (input == Input::FocusNext) _focus = stepIndex(_focus, 1, itemCount());
    if (input == Input::FocusPrev) _focus = stepIndex(_focus, -1, itemCount());
    if (input == Input::Select) activate();
}

void TimerFace::activate() {
    if (_focus < fieldCount()) {
        _editing = true;
        _editField = _focus;
        _editOriginal = setSeconds();
        return;
    }
    if (_focus == primaryIndex()) primary();
    else end();
}

uint16_t TimerFace::digitColor(FaceMode m, int field) const {
    if (_editing && field == _editField) return Theme::accent;
    if (m == FaceMode::Paused) return Theme::textMuted;
    if (m == FaceMode::Done) return Theme::good;
    return Theme::text;
}

uint16_t TimerFace::colonColor(FaceMode m) const {
    if (m == FaceMode::Paused) return Theme::textMuted;
    if (m == FaceMode::Done) return Theme::good;
    return Theme::accent;
}

void TimerFace::drawRegions() {
    drawTop();
    drawReadout();
    drawButtons();
    drawFooter();
}

void TimerFace::drawReadout() {
    FaceMode m = mode();
    uint32_t total = seconds();
    unsigned parts[2] = {(unsigned)(total / 60), (unsigned)(total % 60)};
    if (parts[0] > 99) parts[0] = 99;

    for (int i = 0; i < 2; i++) {
        int tone = _editing && i == _editField ? 4 : (int)m;
        int32_t key = parts[i] * 8 + tone;
        if (key == _groupKeys[i]) continue;
        _groupKeys[i] = key;
        TimerUi::drawGroup(_readout, i, parts[i], digitColor(m, i));
    }

    bool colonOn = m != FaceMode::Running || total % 2 == 0;
    int32_t colonKey = (colonOn ? 8 : 0) + (int)m;
    if (colonKey != _colonKey) {
        _colonKey = colonKey;
        TimerUi::drawColon(_readout, colonOn ? colonColor(m) : Theme::background);
    }

    int field = _editing ? _editField : _focus;
    int level = _editing ? 2 : (_focus < fieldCount() ? 1 : 0);
    int32_t underlineKey = level * 4 + field;
    if (underlineKey != _underlineKey) {
        _underlineKey = underlineKey;
        TimerUi::drawUnderline(_readout, field, level);
    }
}

void TimerFace::drawButtons() {
    FaceMode m = mode();
    bool set = m == FaceMode::Set;
    int top = buttonTop();

    int32_t layout = set;
    if (layout != _layoutKey) {
        _layoutKey = layout;
        tft.fillRect(0, top - 4, 320, TimerUi::buttonH + 8, Theme::background);
        _primaryKey = _endKey = -1;
    }

    int count = set ? 1 : 2;
    bool primaryFocused = _focus == primaryIndex();
    int32_t primaryKey = (int)m * 2 + primaryFocused;
    if (primaryKey != _primaryKey) {
        _primaryKey = primaryKey;
        const char* label = m == FaceMode::Set ? "Start" : m == FaceMode::Running ? "Pause"
                          : m == FaceMode::Paused ? "Resume" : "Again";
        Widgets::button(TimerUi::buttonX(count, 0), top, TimerUi::buttonW, TimerUi::buttonH, label, primaryFocused);
    }

    if (set) return;
    bool endFocused = _focus == 1;
    if (endFocused != _endKey) {
        _endKey = endFocused;
        Widgets::button(TimerUi::buttonX(count, 1), top, TimerUi::buttonW, TimerUi::buttonH, "End", endFocused);
    }
}
