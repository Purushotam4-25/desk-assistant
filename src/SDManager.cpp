#include "SDManager.h"

SDManager::SDManager(uint8_t csPin) : _csPin(csPin) {}

bool SDManager::begin() {
    return _sd.begin(_csPin, SD_SCK_MHZ(4));
}

bool SDManager::writeFile(const char* filepath, const char* data) {
    _file = _sd.open(filepath, O_RDWR | O_CREAT);
    if (!_file) {
        Serial.println("Failed to open file for writing.");
        return false;
    }

    _file.print(data);
    _file.close();
    return true;
}

String SDManager::readFile(const char* filepath) {
    File32 file = _sd.open(filepath, O_RDONLY);
    if (!file) {
        Serial.println("Failed to open file for reading.");
        return "";
    }

    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }

    file.close();
    return content;
}
