#pragma once
#include <Arduino.h>
#include <DHT.h>

class DHTManager {
public:
    DHTManager(uint8_t pin);
    void begin();

    float getTemperature();
    float getHumidity();

private:
    DHT _dht;
};
