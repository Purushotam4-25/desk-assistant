#include "ConfigService.h"

namespace {
constexpr const char* configPath = "/config.json";
constexpr const char* secretsPath = "/secrets.json";
constexpr const char* nvsNamespace = "deskcfg";
constexpr int fileVersion = 1;
constexpr uint32_t saveDelayMs = 3000;
constexpr uint32_t retryDelayMs = 30000;
constexpr uint8_t maxFailures = 3;

bool bound(SettingId id) {
    return id == SettingId::TimeFormat || id == SettingId::TimeZone || id == SettingId::AutoSync;
}

bool validOffset(int minutes) { return minutes >= -840 && minutes <= 840; }
bool validLatitude(float degrees) { return degrees >= -90.0f && degrees <= 90.0f; }
bool validLongitude(float degrees) { return degrees >= -180.0f && degrees <= 180.0f; }
}

ConfigService::ConfigService() {
    applyDefaults();
}

void ConfigService::applyDefaults() {
    for (uint8_t i = 0; i < Settings::count; i++) _values[i] = Settings::def(static_cast<SettingId>(i)).fallback;
}

void ConfigService::syncBoundValues() {
    _values[static_cast<uint8_t>(SettingId::TimeFormat)] = _config.use24Hour;
    _values[static_cast<uint8_t>(SettingId::TimeZone)] = _config.utcOffsetMinutes;
    _values[static_cast<uint8_t>(SettingId::AutoSync)] = _config.ntp;
}

void ConfigService::begin(SDManager& sd) {
    _sd = &sd;
    _prefs.begin(nvsNamespace, false);
    _pending = _prefs.getULong64("pend64", _prefs.getUInt("pend", 0));
    loadMirror();

    bool needSync = false;
    JsonResult result = sd.readJsonRecovering(configPath, _doc);
    if (result == JsonResult::Ok) {
        Serial.println("Config: loaded config.json");
        applyFile();
        needSync = _pending != 0;
    } else if (!sd.present()) {
        Serial.println("Config: no SD card, using saved settings");
    } else if (result == JsonResult::Missing) {
        Serial.println("Config: no config.json, creating one");
        needSync = true;
    } else {
        Serial.println("Config: config.json is invalid, leaving it alone until it is fixed");
        _saveBlocked = true;
    }

    applySecrets();
    syncBoundValues();
    writeMirror();
    if (needSync && !_saveBlocked) syncToCard();
}

void ConfigService::loadMirror() {
    _config.utcOffsetMinutes = _prefs.getShort("utcOff", _config.utcOffsetMinutes);
    _config.latitude = _prefs.getFloat("lat", _config.latitude);
    _config.longitude = _prefs.getFloat("lon", _config.longitude);
    _config.use24Hour = _prefs.getBool("use24h", _config.use24Hour);
    _config.ntp = _prefs.getBool("ntp", _config.ntp);
    _prefs.getString("wssid", _config.wifiSsid, sizeof(_config.wifiSsid));
    _prefs.getString("wpass", _config.wifiPassword, sizeof(_config.wifiPassword));
    _prefs.getString("bridge", _config.bridgeUrl, sizeof(_config.bridgeUrl));

    for (uint8_t i = 0; i < Settings::count; i++) {
        SettingId id = static_cast<SettingId>(i);
        if (bound(id)) continue;
        _values[i] = Settings::clamp(id, _prefs.getShort(Settings::def(id).key, Settings::def(id).fallback));
    }
}

void ConfigService::applyFile() {
    JsonObject root = _doc.as<JsonObject>();

    if (!pending(SettingId::TimeZone) && root["utcOffsetMinutes"].is<int>() && validOffset(root["utcOffsetMinutes"])) {
        _config.utcOffsetMinutes = root["utcOffsetMinutes"];
    }
    if (root["latitude"].is<float>() && validLatitude(root["latitude"])) {
        _config.latitude = root["latitude"];
    }
    if (root["longitude"].is<float>() && validLongitude(root["longitude"])) {
        _config.longitude = root["longitude"];
    }
    if (!pending(SettingId::TimeFormat) && root["use24Hour"].is<bool>()) {
        _config.use24Hour = root["use24Hour"];
    }
    if (!pending(SettingId::AutoSync) && root["ntp"].is<bool>()) {
        _config.ntp = root["ntp"];
    }

    if (root["bridgeUrl"].is<const char*>()) {
        strlcpy(_config.bridgeUrl, root["bridgeUrl"], sizeof(_config.bridgeUrl));
    }

    JsonObject settings = root["settings"];
    for (uint8_t i = 0; i < Settings::count; i++) {
        SettingId id = static_cast<SettingId>(i);
        const SettingDef& d = Settings::def(id);
        if (bound(id) || pending(id) || !settings[d.key].is<int>()) continue;
        int fileValue = settings[d.key];
        if (fileValue >= d.min && fileValue <= d.max) _values[i] = fileValue;
    }
}

void ConfigService::applySecrets() {
    JsonDocument secrets;
    if (!_sd->readJson(secretsPath, secrets)) {
        Serial.println("Config: secrets.json not read, using saved WiFi");
        return;
    }
    Serial.println("Config: loaded secrets.json");

    JsonObject wifi = secrets["wifi"];
    if (wifi["ssid"].is<const char*>()) {
        strlcpy(_config.wifiSsid, wifi["ssid"], sizeof(_config.wifiSsid));
    }
    if (wifi["password"].is<const char*>()) {
        strlcpy(_config.wifiPassword, wifi["password"], sizeof(_config.wifiPassword));
    }
}

void ConfigService::writeMirror() {
    _prefs.putShort("utcOff", _config.utcOffsetMinutes);
    _prefs.putFloat("lat", _config.latitude);
    _prefs.putFloat("lon", _config.longitude);
    _prefs.putBool("use24h", _config.use24Hour);
    _prefs.putBool("ntp", _config.ntp);
    _prefs.putString("wssid", _config.wifiSsid);
    _prefs.putString("wpass", _config.wifiPassword);
    _prefs.putString("bridge", _config.bridgeUrl);
    _prefs.putULong64("pend64", _pending);

    for (uint8_t i = 0; i < Settings::count; i++) {
        SettingId id = static_cast<SettingId>(i);
        if (!bound(id)) _prefs.putShort(Settings::def(id).key, _values[i]);
    }
}

void ConfigService::fillDocument() {
    _doc["version"] = fileVersion;
    _doc["utcOffsetMinutes"] = _config.utcOffsetMinutes;
    _doc["latitude"] = _config.latitude;
    _doc["longitude"] = _config.longitude;
    _doc["use24Hour"] = _config.use24Hour;
    _doc["ntp"] = _config.ntp;
    _doc["bridgeUrl"] = _config.bridgeUrl;

    JsonObject settings = _doc["settings"].to<JsonObject>();
    for (uint8_t i = 0; i < Settings::count; i++) {
        SettingId id = static_cast<SettingId>(i);
        if (!bound(id)) settings[Settings::def(id).key] = _values[i];
    }
}

void ConfigService::setValue(SettingId id, int16_t value) {
    value = Settings::clamp(id, value);
    if (_values[static_cast<uint8_t>(id)] == value) return;
    _values[static_cast<uint8_t>(id)] = value;
    switch (id) {
        case SettingId::TimeFormat: _config.use24Hour = value; break;
        case SettingId::TimeZone: _config.utcOffsetMinutes = value; break;
        case SettingId::AutoSync: _config.ntp = value; break;
        default: break;
    }
    markChanged(id);
    _revision++;
}

void ConfigService::markChanged(SettingId id) {
    _pending |= 1ULL << static_cast<uint8_t>(id);
    _dirty = true;
    _saveAt = millis() + saveDelayMs;
}

void ConfigService::flushNow() {
    _pending = ~0ULL >> (64 - Settings::count);
    _revision++;
    writeMirror();
    _dirty = !(_sd && _sd->present() && !_saveBlocked && syncToCard());
    _saveAt = millis() + retryDelayMs;
}

void ConfigService::resetSettings() {
    applyDefaults();
    _config.use24Hour = _values[static_cast<uint8_t>(SettingId::TimeFormat)];
    _config.utcOffsetMinutes = _values[static_cast<uint8_t>(SettingId::TimeZone)];
    _config.ntp = _values[static_cast<uint8_t>(SettingId::AutoSync)];
    flushNow();
}

void ConfigService::factoryReset() {
    _prefs.clear();
    _config = Config();
    applyDefaults();
    flushNow();
}

void ConfigService::commit() {
    if (!_dirty) return;
    writeMirror();
    if (_sd && _sd->present() && !_saveBlocked && _failures < maxFailures) syncToCard();
    _dirty = false;
}

void ConfigService::update() {
    if (!_dirty || (int32_t)(millis() - _saveAt) < 0) return;
    writeMirror();

    if (!_sd->present() || _saveBlocked || _failures >= maxFailures) {
        _dirty = false;
        return;
    }
    if (syncToCard()) {
        _dirty = false;
    } else {
        _saveAt = millis() + retryDelayMs;
    }
}

bool ConfigService::syncToCard() {
    fillDocument();
    if (_sd->writeJsonAtomic(configPath, _doc)) {
        _failures = 0;
        _pending = 0;
        _prefs.putULong64("pend64", 0);
        Serial.println("Config: saved config.json");
        return true;
    }

    _failures++;
    Serial.println(_failures >= maxFailures ? "Config: giving up saving until the next boot"
                                            : "Config: saving config.json failed, will retry");
    return false;
}
