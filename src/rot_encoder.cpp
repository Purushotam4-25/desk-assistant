#include "rot_encoder.h"
#include "driver/pcnt.h"

namespace {
constexpr int32_t countsPerDetent = 4;
constexpr uint16_t pcntFilter = 1023;
}

RotaryEncoderManager::RotaryEncoderManager(uint8_t clkPin, uint8_t dtPin, uint8_t swPin) 
    : _clkPin(clkPin), _dtPin(dtPin), _swPin(swPin), _lastPosition(0) {}

void RotaryEncoderManager::begin() {
      pinMode(_clkPin, INPUT_PULLUP);
      pinMode(_dtPin, INPUT_PULLUP);


    _encoder.attachFullQuad(_clkPin, _dtPin);
    _encoder.setFilter(pcntFilter);
    _encoder.clearCount();
    _lastPosition = 0;

    _button = OneButton(_swPin, true, true);
    setClickHandler(defaultClick);
    setDoubleClickHandler(defaultDoubleClick);
    setLongPressHandler(defaultLongPress);
}

void RotaryEncoderManager::update() {
    _button.tick(); 
}

int32_t RotaryEncoderManager::getPosition() {
    return _encoder.getCount();
}


int32_t RotaryEncoderManager::getDelta() {
    int32_t steps = (getPosition() - _lastPosition) / countsPerDetent;
    _lastPosition += steps * countsPerDetent;
    return steps;
}

void RotaryEncoderManager::resetPosition(int32_t newPos) {
    _encoder.setCount(newPos);
    _lastPosition = newPos;
}

// Button handler setters
void RotaryEncoderManager::setClickHandler(void (*callback)()) {
    _button.attachClick(callback);
}

void RotaryEncoderManager::setDoubleClickHandler(void (*callback)()) {
    _button.attachDoubleClick(callback);
}

void RotaryEncoderManager::setLongPressHandler(void (*callback)()) {
    _button.attachLongPressStart(callback);
}

void RotaryEncoderManager::setTiming(unsigned int clickMs, int debounceMs) {
    _button.setClickMs(clickMs);
    _button.setDebounceMs(debounceMs);
}