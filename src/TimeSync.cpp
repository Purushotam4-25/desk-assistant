#include "TimeSync.h"
#include <time.h>
#include <esp_sntp.h>
#include "DateMath.h"

namespace {
constexpr time_t firstValidEpoch = 1700000000;
constexpr uint32_t pollMs = 500;
constexpr uint32_t recheckMs = 600000;
constexpr uint32_t stalledReportMs = 30000;
constexpr const char* primaryServer = "pool.ntp.org";
constexpr const char* secondaryServer = "time.cloudflare.com";
constexpr int32_t correctionThresholdSeconds = 2;
constexpr int32_t secondsPerDay = 86400;
}

TimeSync::TimeSync(RTCManager& rtc, const ConfigService& config, const WifiService& wifi)
    : _rtc(rtc), _config(config), _wifi(wifi) {}

void TimeSync::update() {
    if (!_config.config().ntp || !_wifi.connected()) {
        _started = false;
        return;
    }

    uint32_t now = millis();
    if (!_started) {
        startSntp();
        _started = true;
        _lastCheck = now;
        _nextReport = now + stalledReportMs;
        Serial.println("Time: asking NTP");
    }

    if (now - _lastCheck < (_synced ? recheckMs : pollMs)) return;
    _lastCheck = now;
    check();

    if (!_synced && (int32_t)(now - _nextReport) >= 0) {
        _nextReport = now + stalledReportMs;
        reportStalled();
        startSntp();
    }
}

void TimeSync::startSntp() {
    configTime(0, 0, primaryServer, secondaryServer);
}

void TimeSync::reportStalled() {
    const ip_addr_t* server = esp_sntp_getserver(0);
    const char* resolved = server && !ip_addr_isany(server) ? ipaddr_ntoa(server) : "not resolved";
    Serial.printf("Time: NTP not answering (server %s, sync status %d, DNS %s), restarting\n", resolved,
                  (int)sntp_get_sync_status(), WiFi.dnsIP().toString().c_str());
}

void TimeSync::check() {
    time_t utc = time(nullptr);
    if (utc < firstValidEpoch) return;

    int64_t local = (int64_t)utc + (int64_t)_config.config().utcOffsetMinutes * 60;
    int32_t days = (int32_t)(local / secondsPerDay);
    int32_t seconds = (int32_t)(local % secondsPerDay);
    int year;
    unsigned month, day;
    Date::civilFromDays(days, year, month, day);

    DateTime rtc = _rtc.getDateTime();
    int32_t rtcSeconds = Date::daysFromCivil(rtc.year, rtc.month, rtc.day) * secondsPerDay + rtc.hour * 3600 +
                         rtc.minute * 60 + rtc.second;
    int32_t drift = (int32_t)(local - (int64_t)rtcSeconds);

    if (drift >= correctionThresholdSeconds || drift <= -correctionThresholdSeconds) {
        _rtc.setDateTime(year, month, day, seconds / 3600, seconds % 3600 / 60, seconds % 60);
        Serial.printf("Time: RTC set from NTP (was off by %ld s)\n", (long)drift);
    } else if (!_synced) {
        Serial.println("Time: RTC already in sync");
    }
    _synced = true;
    _lastSyncAt = millis();
}
