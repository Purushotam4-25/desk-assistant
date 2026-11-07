#pragma once
#include <Preferences.h>
#include <ArduinoJson.h>
#include "SDManager.h"

struct Config {
    int16_t utcOffsetMinutes = 330;
    float latitude = 19.08f;
    float longitude = 72.88f;
    bool use24Hour = false;
    bool ntp = true;
    char wifiSsid[33] = "";
    char wifiPassword[64] = "";
};

class ConfigService {
public:
    void begin(SDManager& sd);
    void update();
    const Config& config() const { return _config; }
    void setUse24Hour(bool value);

private:
    enum Setting : uint8_t { Use24Hour };

    Config _config;
    Preferences _prefs;
    JsonDocument _doc;
    SDManager* _sd = nullptr;
    uint32_t _pending = 0;
    uint32_t _saveAt = 0;
    uint8_t _failures = 0;
    bool _dirty = false;
    bool _saveBlocked = false;

    void loadMirror();
    void applyFile();
    void applySecrets();
    void writeMirror();
    void fillDocument();
    void markChanged(Setting setting);
    bool syncToCard();
};
