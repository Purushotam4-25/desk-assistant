#pragma once
#include <Arduino.h>
#include <ESP32Encoder.h>
#include <OneButton.h>

class RotaryEncoderManager {
public:
    RotaryEncoderManager(uint8_t clkPin, uint8_t dtPin, uint8_t swPin);
    void begin();
    void update(); // Call frequently in loop()
    
    // Encoder methods
    int32_t getPosition();
    int32_t getDelta();
    void resetPosition(int32_t newPos = 0);
    
    // Button methods
    void setClickHandler(void (*callback)());
    void setDoubleClickHandler(void (*callback)());
    void setLongPressHandler(void (*callback)());
    void setTiming(unsigned int clickMs, int debounceMs);

private:
    ESP32Encoder _encoder;
    OneButton _button;
    
    int32_t _lastPosition;
    uint8_t _clkPin, _dtPin, _swPin;
    
    static void defaultClick() {}
    static void defaultDoubleClick() {}
    static void defaultLongPress() {}
};