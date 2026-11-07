#pragma once
#include <Preferences.h>
#include <ArduinoJson.h>
#include "SDManager.h"
#include "Settings.h"

struct Config {
    int16_t utcOffsetMinutes = 330;
    float latitude = 19.08f;
    float longitude = 72.88f;
    bool use24Hour = false;
    bool ntp = true;
    char wifiSsid[33] = "";
    char wifiPassword[64] = "";
    char bridgeUrl[64] = "";
};

class ConfigService {
public:
    ConfigService();
    void begin(SDManager& sd);
    void update();
    void commit();
    const Config& config() const { return _config; }
    int16_t value(SettingId id) const { return _values[static_cast<uint8_t>(id)]; }
    void setValue(SettingId id, int16_t value);
    void setUse24Hour(bool value) { setValue(SettingId::TimeFormat, value); }
    uint32_t revision() const { return _revision; }
    void resetSettings();
    void factoryReset();

private:
    Config _config;
    int16_t _values[Settings::count];
    uint32_t _revision = 0;
    Preferences _prefs;
    JsonDocument _doc;
    SDManager* _sd = nullptr;
    uint64_t _pending = 0;
    uint32_t _saveAt = 0;
    uint8_t _failures = 0;
    bool _dirty = false;
    bool _saveBlocked = false;

    void loadMirror();
    void applyFile();
    void applySecrets();
    void writeMirror();
    void fillDocument();
    void markChanged(SettingId id);
    bool syncToCard();
    void syncBoundValues();
    void applyDefaults();
    void flushNow();
    bool pending(SettingId id) const { return _pending & (1ULL << static_cast<uint8_t>(id)); }
};
