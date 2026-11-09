#include "CalendarService.h"
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

namespace {
constexpr uint32_t retryMs = 60000;
constexpr uint16_t httpTimeoutMs = 8000;
constexpr uint32_t refreshMinutes[] = {5, 15, 30, 60};
constexpr uint8_t httpPrefixLength = 7;
constexpr int minutesPerDay = 24 * 60;
}

CalendarService::CalendarService(const ConfigService& config, const WifiService& wifi) : _config(config), _wifi(wifi) {}

void CalendarService::begin() {
    xTaskCreatePinnedToCore(taskMain, "calendar", 8192, this, 1, nullptr, 0);
}

uint32_t CalendarService::dateKey(const DateTime& now) {
    return now.year * 10000UL + now.month * 100UL + now.day;
}

uint32_t CalendarService::intervalMs() const {
    int16_t choice = _config.value(SettingId::CalendarEvery);
    return refreshMinutes[choice < 0 || choice > 3 ? 1 : choice] * 60000UL;
}

void CalendarService::refreshNow() {
    _forceNext = true;
}

void CalendarService::update(Agenda& target, const DeviceStatus& status) {
    uint32_t today = dateKey(status.now);
    if (_resultReady) {
        finishFetch(target, today);
        return;
    }

    if (target.synced() && target.dateKey() != today) {
        target.clear();
        _forceNext = true;
    }
    if (_state == State::Fetching) return;

    if (_config.config().bridgeUrl[0] == '\0') {
        _state = State::NoBridge;
        return;
    }
    if (!_wifi.connected()) {
        if (_state != State::Error) _state = State::WaitingForWifi;
        return;
    }
    if (_state == State::NoBridge || _state == State::WaitingForWifi) _nextFetchMs = millis();

    bool due = (int32_t)(millis() - _nextFetchMs) >= 0;
    if (due || _forceNext) startFetch();
}

void CalendarService::startFetch() {
    const Config& config = _config.config();
    if (strncmp(config.bridgeUrl, "http://", httpPrefixLength) != 0) {
        snprintf(_error, sizeof(_error), "Bridge must be http://");
        _state = State::Error;
        _nextFetchMs = millis() + retryMs;
        _forceNext = false;
        return;
    }

    size_t length = strlen(config.bridgeUrl);
    while (length > 0 && config.bridgeUrl[length - 1] == '/') length--;
    snprintf(_url, sizeof(_url), "%.*s/calendar/today?off=%d", (int)length, config.bridgeUrl, config.utcOffsetMinutes);

    _forceNext = false;
    _state = State::Fetching;
    _fetchRequested = true;
}

void CalendarService::finishFetch(Agenda& target, uint32_t todayKey) {
    _resultReady = false;
    if (_resultOk && _resultDate == todayKey) {
        target.replace(_result, _resultCount, _resultDate);
        _shownCount = _resultCount;
        _state = State::Ok;
        _lastOkMs = millis();
        _error[0] = '\0';
        _nextFetchMs = millis() + intervalMs();
        Serial.printf("Calendar: %u events\n", (unsigned)_resultCount);
        return;
    }

    if (_resultOk) {
        snprintf(_error, sizeof(_error), "Clock differs");
    } else {
        strlcpy(_error, _taskError, sizeof(_error));
    }
    _state = State::Error;
    _nextFetchMs = millis() + retryMs;
    Serial.printf("Calendar: fetch failed (%s)\n", _error);
}

void CalendarService::taskMain(void* self) {
    CalendarService* service = static_cast<CalendarService*>(self);
    for (;;) {
        if (service->_fetchRequested) {
            service->fetch();
            service->_fetchRequested = false;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void CalendarService::fetch() {
    _resultOk = false;
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(httpTimeoutMs);
    http.setConnectTimeout(httpTimeoutMs);

    if (!http.begin(client, _url)) {
        snprintf(_taskError, sizeof(_taskError), "Bad bridge address");
        _resultReady = true;
        return;
    }

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        if (code > 0) {
            snprintf(_taskError, sizeof(_taskError), "Bridge HTTP %d", code);
        } else {
            snprintf(_taskError, sizeof(_taskError), "%s", HTTPClient::errorToString(code).c_str());
        }
        http.end();
        _resultReady = true;
        return;
    }

    JsonDocument doc;
    DeserializationError parsed = deserializeJson(doc, http.getString());
    http.end();

    if (parsed) {
        snprintf(_taskError, sizeof(_taskError), "Bad data");
        _resultReady = true;
        return;
    }

    if (!(doc["ok"] | false)) {
        const char* err = doc["err"] | "";
        if (strcmp(err, "not_linked") == 0) {
            snprintf(_taskError, sizeof(_taskError), "Not linked");
        } else if (strcmp(err, "rate_limited") == 0) {
            snprintf(_taskError, sizeof(_taskError), "Rate limited");
        } else {
            snprintf(_taskError, sizeof(_taskError), "Google %s", err[0] ? err : "error");
        }
        _resultReady = true;
        return;
    }

    unsigned year = 0, month = 0, day = 0;
    JsonArray events = doc["events"];
    if (sscanf(doc["date"] | "", "%4u-%2u-%2u", &year, &month, &day) != 3 || events.isNull()) {
        snprintf(_taskError, sizeof(_taskError), "Bad data");
        _resultReady = true;
        return;
    }

    size_t count = 0;
    for (JsonObject event : events) {
        int start = event["s"] | -1;
        int end = event["e"] | -1;
        if (start < 0 || start >= minutesPerDay || end <= start || end > minutesPerDay) continue;
        if (count == agendaCapacity) break;
        AgendaEvent& out = _result[count++];
        out.startMinute = start;
        out.endMinute = end;
        strlcpy(out.title, event["t"] | "", sizeof(out.title));
    }

    _resultCount = count;
    _resultDate = year * 10000UL + month * 100UL + day;
    _resultOk = true;
    _resultReady = true;
}
