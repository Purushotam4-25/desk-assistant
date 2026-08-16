#pragma once
#include <SdFat.h>

class SDManager {
public:
    SDManager(uint8_t csPin);
    bool begin();

    bool writeFile(const char* filepath, const char* data);
    String readFile(const char* filepath);

private:
    uint8_t _csPin;
    SdFat _sd;
    File32 _file;
};
