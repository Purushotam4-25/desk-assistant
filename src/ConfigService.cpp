#include "ConfigService.h"

namespace {
constexpr const char* configPath = "/config.json";
constexpr const char* secretsPath = "/secrets.json";
constexpr const char* nvsNamespace = "deskcfg";
constexpr int fileVersion = 1;
constexpr uint32_t saveDelayMs = 3000;
constexpr uint32_t retryDelayMs = 30000;
constexpr uint8_t maxFailures = 3;

bool validOffset(int minutes) { return minutes >= -840 && minutes <= 840; }
bool validLatitude(float degrees) { return degrees >= -90.0f && degrees <= 90.0f; }
bool validLongitude(float degrees) { return degrees >= -180.0f && degrees <= 180.0f; }
}

void ConfigService::begin(SDManager& sd) {
    _sd = &sd;
    _prefs.begin(nvsNamespace, false);
    _pending = _prefs.getUInt("pend", 0);
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
    writeMirror();
    if (needSync && !_saveBlocked) syncToCard();
}

void ConfigService::loadMirror() {
    _config.utcOffsetMinutes = _prefs.getShort("utcOff", _config.utcOffsetMinutes);
    _config.latitude = _prefs.getFloat("lat", _config.latitude);
    _config.longitude = _prefs.getFloat("lon", _config.longitude);
    _config.use24Hour = _prefs.getBool("use24h", _config.use24Hour);
    _prefs.getString("wssid", _config.wifiSsid, sizeof(_config.wifiSsid));
    _prefs.getString("wpass", _config.wifiPassword, sizeof(_config.wifiPassword));
}

void ConfigService::applyFile() {
    JsonObject root = _doc.as<JsonObject>();

    if (root["utcOffsetMinutes"].is<int>() && validOffset(root["utcOffsetMinutes"])) {
        _config.utcOffsetMinutes = root["utcOffsetMinutes"];
    }
    if (root["latitude"].is<float>() && validLatitude(root["latitude"])) {
        _config.latitude = root["latitude"];
    }
    if (root["longitude"].is<float>() && validLongitude(root["longitude"])) {
        _config.longitude = root["longitude"];
    }
    if (!(_pending & (1u << Use24Hour)) && root["use24Hour"].is<bool>()) {
        _config.use24Hour = root["use24Hour"];
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
    _prefs.putString("wssid", _config.wifiSsid);
    _prefs.putString("wpass", _config.wifiPassword);
    _prefs.putUInt("pend", _pending);
}

void ConfigService::fillDocument() {
    _doc["version"] = fileVersion;
    _doc["utcOffsetMinutes"] = _config.utcOffsetMinutes;
    _doc["latitude"] = _config.latitude;
    _doc["longitude"] = _config.longitude;
    _doc["use24Hour"] = _config.use24Hour;
}

void ConfigService::setUse24Hour(bool value) {
    if (_config.use24Hour == value) return;
    _config.use24Hour = value;
    markChanged(Use24Hour);
}

void ConfigService::markChanged(Setting setting) {
    _pending |= 1u << setting;
    _dirty = true;
    _saveAt = millis() + saveDelayMs;
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
        _prefs.putUInt("pend", 0);
        Serial.println("Config: saved config.json");
        return true;
    }

    _failures++;
    Serial.println(_failures >= maxFailures ? "Config: giving up saving until the next boot"
                                            : "Config: saving config.json failed, will retry");
    return false;
}
