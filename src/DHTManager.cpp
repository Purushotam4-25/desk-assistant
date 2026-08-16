#include "DHTManager.h"

DHTManager::DHTManager(uint8_t pin) : _dht(pin, DHT11) {}

void DHTManager::begin() {
    _dht.begin();
}

float DHTManager::getTemperature() {
    return _dht.readTemperature();
}

float DHTManager::getHumidity() {
    return _dht.readHumidity();
}
