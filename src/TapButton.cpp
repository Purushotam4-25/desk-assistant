#include "TapButton.h"

namespace {
constexpr uint32_t debounceMs = 20;
}

TapButton::TapButton(uint8_t pin, bool activeLow) : _pin(pin), _activeLow(activeLow) {}

void TapButton::begin() {
    pinMode(_pin, _activeLow ? INPUT_PULLUP : INPUT);
    _raw = _down = readRaw();
    _pressEdge = false;
    _changedAt = millis();
}

bool TapButton::readRaw() const {
    return digitalRead(_pin) == (_activeLow ? LOW : HIGH);
}

bool TapButton::wasPressed() {
    uint32_t now = millis();
    bool raw = readRaw();
    if (raw != _raw) {
        _raw = raw;
        _changedAt = now;
    }
    if (_raw != _down && now - _changedAt >= debounceMs) {
        _down = _raw;
        if (_down) _pressEdge = true;
    }
    bool pressed = _pressEdge;
    _pressEdge = false;
    return pressed;
}
