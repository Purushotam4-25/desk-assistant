#include "rot_encoder.h"
#include "driver/pcnt.h"

RotaryEncoderManager::RotaryEncoderManager(uint8_t clkPin, uint8_t dtPin, uint8_t swPin) 
    : _clkPin(clkPin), _dtPin(dtPin), _swPin(swPin), _lastPosition(0) {}

void RotaryEncoderManager::begin() {
    // Initialize encoder with hardware PCNT
      pinMode(_clkPin, INPUT_PULLUP);
      pinMode(_dtPin, INPUT_PULLUP);


    _encoder.attachSingleEdge(_clkPin, _dtPin);

    
    _encoder.setFilter(500);
    _encoder.clearCount();
    
    // Initialize button
    _button = OneButton(_swPin, true, true);
    setClickHandler(defaultClick);
    setDoubleClickHandler(defaultDoubleClick);
    setLongPressHandler(defaultLongPress);
}

void RotaryEncoderManager::update() {
    _button.tick(); // Must be called frequently
}

int32_t RotaryEncoderManager::getPosition() {
    return _encoder.getCount();
}


int32_t RotaryEncoderManager::getDelta() {
    int32_t current = getPosition();
    int32_t delta = current - _lastPosition;
    _lastPosition = current;
    return delta;
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