#include "SDManager.h"

namespace {
constexpr size_t maxFileBytes = 768;

void siblingPath(const char* path, const char* extension, char* out, size_t size) {
    strlcpy(out, path, size);
    char* dot = strrchr(out, '.');
    if (dot) *dot = '\0';
    strlcat(out, extension, size);
}
}

SDManager::SDManager(uint8_t csPin) : _csPin(csPin) {}

bool SDManager::begin(uint8_t maxMhz) {
    _present = _sd.begin(_csPin, SD_SCK_MHZ(maxMhz));
    if (!_present) {
        Serial.printf("SD init failed at %u MHz (error 0x%02X, data 0x%02X)\n", maxMhz, _sd.sdErrorCode(),
                      _sd.sdErrorData());
    }
    return _present;
}

bool SDManager::readJson(const char* path, JsonDocument& doc) {
    if (!_present) return false;
    File32 file = _sd.open(path, O_RDONLY);
    if (!file) return false;

    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error) {
        doc.clear();
        Serial.printf("%s: %s\n", path, error.c_str());
        return false;
    }
    return true;
}

JsonResult SDManager::readJsonRecovering(const char* path, JsonDocument& doc) {
    if (!_present) return JsonResult::Missing;
    if (_sd.exists(path)) return readJson(path, doc) ? JsonResult::Ok : JsonResult::Invalid;

    char temp[32], backup[32];
    siblingPath(path, ".tmp", temp, sizeof(temp));
    siblingPath(path, ".bak", backup, sizeof(backup));
    for (const char* candidate : {temp, backup}) {
        if (_sd.exists(candidate) && readJson(candidate, doc) && _sd.rename(candidate, path)) {
            Serial.printf("%s recovered from %s\n", path, candidate);
            return JsonResult::Ok;
        }
    }
    return JsonResult::Missing;
}

bool SDManager::writeJsonAtomic(const char* path, const JsonDocument& doc, bool useBackup) {
    if (!_present) return false;
    char temp[32], backup[32];
    siblingPath(path, ".tmp", temp, sizeof(temp));
    siblingPath(path, ".bak", backup, sizeof(backup));

    char text[maxFileBytes];
    size_t length = serializeJsonPretty(doc, text, sizeof(text));
    if (length == 0 || length >= sizeof(text)) {
        Serial.printf("%s: config too large to save\n", path);
        return false;
    }

    File32 file = _sd.open(temp, O_WRONLY | O_CREAT | O_TRUNC);
    if (!file) {
        Serial.printf("%s: cannot open for writing\n", temp);
        return false;
    }
    size_t wrote = file.write((const uint8_t*)text, length);
    bool synced = file.sync();
    uint32_t size = file.fileSize();
    file.close();
    if (wrote != length || !synced || size != length) {
        Serial.printf("%s: write failed (%u of %u bytes, size %u)\n", temp, (unsigned)wrote, (unsigned)length,
                      (unsigned)size);
        _sd.remove(temp);
        return false;
    }

    JsonDocument check;
    if (!readJson(temp, check)) {
        Serial.printf("%s: verification failed, keeping the old file\n", temp);
        _sd.remove(temp);
        return false;
    }

    if (!useBackup) {
        if (_sd.exists(path) && !_sd.remove(path)) return false;
        return _sd.rename(temp, path);
    }

    _sd.remove(backup);
    bool hadOld = _sd.exists(path);
    if (hadOld && !_sd.rename(path, backup)) return false;
    if (!_sd.rename(temp, path)) {
        if (hadOld) _sd.rename(backup, path);
        return false;
    }
    _sd.remove(backup);
    return true;
}
