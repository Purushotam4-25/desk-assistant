#include "SDManager.h"
#include <SPI.h>

namespace {
constexpr size_t maxFileBytes = 2560;
constexpr uint8_t initAttempts = 3;
constexpr uint8_t retryMhz = 1;
constexpr uint16_t retryDelayMs = 100;
constexpr uint16_t flushBytes = 600;

// A card reset in the middle of a block transfer keeps that block pending and ignores CMD0.
// Clocking 0xFF with CS low lets it finish; CS high with more clocks then puts it back to idle.
void flushCard(uint8_t csPin) {
    SPI.begin();
    SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
    digitalWrite(csPin, LOW);
    for (uint16_t i = 0; i < flushBytes; i++) SPI.transfer(0xFF);
    digitalWrite(csPin, HIGH);
    for (uint8_t i = 0; i < 16; i++) SPI.transfer(0xFF);
    SPI.endTransaction();
}

void siblingPath(const char* path, const char* extension, char* out, size_t size) {
    strlcpy(out, path, size);
    char* dot = strrchr(out, '.');
    if (dot) *dot = '\0';
    strlcat(out, extension, size);
}
}

SDManager::SDManager(uint8_t csPin) : _csPin(csPin) {}

bool SDManager::begin(uint8_t maxMhz) {
    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH);

    _mhz = maxMhz;
    uint8_t mhz = maxMhz;
    for (uint8_t attempt = 1; attempt <= initAttempts; attempt++) {
        flushCard(_csPin);
        _present = _sd.begin(_csPin, SD_SCK_MHZ(mhz));
        if (_present) {
            if (attempt > 1) Serial.printf("SD init worked on attempt %u\n", attempt);
            return true;
        }
        Serial.printf("SD init failed at %u MHz (error 0x%02X, data 0x%02X, attempt %u of %u)\n", mhz,
                      _sd.sdErrorCode(), _sd.sdErrorData(), attempt, initAttempts);
        if (mhz > retryMhz) mhz = retryMhz;
        delay(retryDelayMs);
    }
    return false;
}

bool SDManager::capacityMb(uint32_t& totalMb, uint32_t& freeMb, bool refresh) {
    if (!_present) return false;
    if (refresh || !_capacityKnown) {
        int32_t freeClusters = _sd.vol()->freeClusterCount();
        if (freeClusters < 0) return false;
        _totalMb = _sd.card()->sectorCount() / 2048;
        _freeMb = (uint64_t)freeClusters * _sd.vol()->bytesPerCluster() / (1024UL * 1024UL);
        _capacityKnown = true;
    }
    totalMb = _totalMb;
    freeMb = _freeMb;
    return true;
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

bool SDManager::remount() {
    _present = false;
    _capacityKnown = false;
    return begin(_mhz);
}

// A card that browned out and reset itself fails every command until it is initialised again,
// while SdFat still believes it is mounted. So one failed save gets one remount and one retry.
bool SDManager::writeJsonAtomic(const char* path, const JsonDocument& doc, bool useBackup) {
    if (!_present) return false;

    char text[maxFileBytes];
    size_t length = serializeJsonPretty(doc, text, sizeof(text));
    if (length == 0 || length >= sizeof(text)) {
        Serial.printf("%s: config too large to save\n", path);
        return false;
    }

    if (replaceFile(path, text, length, useBackup)) return true;
    Serial.println("SD: save failed, remounting the card to try once more");
    if (!remount()) return false;
    return replaceFile(path, text, length, useBackup);
}

bool SDManager::replaceFile(const char* path, const char* text, size_t length, bool useBackup) {
    char temp[32], backup[32];
    siblingPath(path, ".tmp", temp, sizeof(temp));
    siblingPath(path, ".bak", backup, sizeof(backup));

    File32 file = _sd.open(temp, O_WRONLY | O_CREAT | O_TRUNC);
    if (!file) {
        Serial.printf("%s: cannot open for writing (sd error 0x%02X/0x%02X)\n", temp, _sd.sdErrorCode(),
                      _sd.sdErrorData());
        return false;
    }
    size_t wrote = file.write((const uint8_t*)text, length);
    bool synced = file.sync();
    uint32_t size = file.fileSize();
    file.close();
    if (wrote != length || !synced || size != length) {
        Serial.printf("%s: write failed (%u of %u bytes, size %u, sd error 0x%02X/0x%02X)\n", temp,
                      (unsigned)wrote, (unsigned)length, (unsigned)size, _sd.sdErrorCode(), _sd.sdErrorData());
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
