#pragma once
#include <Arduino.h>

class TapButton {
public:
    TapButton(uint8_t pin, bool activeLow);
    void begin();
    bool wasPressed();

private:
    uint8_t _pin;
    bool _activeLow;
    bool _raw = false;
    bool _down = false;
    bool _pressEdge = false;
    uint32_t _changedAt = 0;

    bool readRaw() const;
};
