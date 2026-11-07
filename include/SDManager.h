#pragma once
#include <SdFat.h>
#include <ArduinoJson.h>

enum class JsonResult { Ok, Missing, Invalid };

class SDManager {
public:
    SDManager(uint8_t csPin);
    bool begin(uint8_t maxMhz = 4);
    bool present() const { return _present; }
    bool remount();
    bool capacityMb(uint32_t& totalMb, uint32_t& freeMb, bool refresh);

    JsonResult readJsonRecovering(const char* path, JsonDocument& doc);
    bool readJson(const char* path, JsonDocument& doc);
    bool writeJsonAtomic(const char* path, const JsonDocument& doc, bool useBackup = true);

private:
    uint8_t _csPin;
    uint8_t _mhz = 4;
    bool _present = false;
    bool _capacityKnown = false;
    uint32_t _totalMb = 0;
    uint32_t _freeMb = 0;
    SdFat _sd;

    bool replaceFile(const char* path, const char* text, size_t length, bool useBackup);
};
