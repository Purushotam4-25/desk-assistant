#include "WeatherService.h"
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

namespace {
constexpr uint32_t retryMs = 60000;
constexpr uint16_t httpTimeoutMs = 5000;
constexpr uint32_t refreshMinutes[] = {15, 30, 60, 180};
constexpr uint8_t httpPrefixLength = 7;

WeatherCondition conditionFrom(int id) {
    switch (id) {
        case 0: return WeatherCondition::Clear;
        case 1: return WeatherCondition::MostlyClear;
        case 3: return WeatherCondition::Rain;
        case 4: return WeatherCondition::Fog;
        case 5: return WeatherCondition::Showers;
        case 6: return WeatherCondition::Storm;
        default: return WeatherCondition::Cloudy;
    }
}

bool sameData(const WeatherData& a, const WeatherData& b) {
    if (a.condition != b.condition || a.isDay != b.isDay || a.temperatureC != b.temperatureC || a.hi != b.hi || a.lo != b.lo) return false;
    if (a.humidity != b.humidity || a.uvIndex != b.uvIndex || a.rainPercent != b.rainPercent) return false;
    if (strcmp(a.summary, b.summary) != 0) return false;
    for (int i = 0; i < 3; i++) {
        if (a.forecast[i].condition != b.forecast[i].condition || a.forecast[i].hi != b.forecast[i].hi ||
            a.forecast[i].lo != b.forecast[i].lo) {
            return false;
        }
    }
    return true;
}
}

WeatherService::WeatherService(const ConfigService& config, const WifiService& wifi) : _config(config), _wifi(wifi) {}

void WeatherService::begin() {
    xTaskCreatePinnedToCore(taskMain, "weather", 8192, this, 1, nullptr, 0);
}

uint32_t WeatherService::intervalMs() const {
    int16_t choice = _config.value(SettingId::WeatherEvery);
    return refreshMinutes[choice] * 60000UL;
}

void WeatherService::refreshNow() {
    _forceNext = true;
}

void WeatherService::update(WeatherData& target) {
    if (_resultReady) {
        finishFetch(target);
        return;
    }
    if (_state == State::Fetching) return;

    const Config& config = _config.config();
    if (config.bridgeUrl[0] == '\0') {
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

void WeatherService::startFetch() {
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
    snprintf(_url, sizeof(_url), "%.*s/weather?lat=%.4f&lon=%.4f", (int)length, config.bridgeUrl, config.latitude,
             config.longitude);

    _forceNext = false;
    _state = State::Fetching;
    _fetchRequested = true;
}

void WeatherService::finishFetch(WeatherData& target) {
    _resultReady = false;
    if (_resultOk) {
        _result.revision = target.revision;
        _result.isDay = target.isDay;
        _result.valid = true;
        if (!target.valid || !sameData(_result, target)) {
            target = _result;
            target.revision++;
        }
        _state = State::Ok;
        _lastOkMs = millis();
        _error[0] = '\0';
        _nextFetchMs = millis() + intervalMs();
        Serial.println("Weather: updated");
    } else {
        strlcpy(_error, _taskError, sizeof(_error));
        _state = State::Error;
        _nextFetchMs = millis() + retryMs;
        Serial.printf("Weather: fetch failed (%s)\n", _error);
    }
}

void WeatherService::taskMain(void* self) {
    WeatherService* service = static_cast<WeatherService*>(self);
    for (;;) {
        if (service->_fetchRequested) {
            service->fetch();
            service->_fetchRequested = false;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void WeatherService::fetch() {
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

    JsonArray forecast = doc["fc"];
    if (parsed || !doc["temp"].is<int>() || forecast.size() < 3) {
        snprintf(_taskError, sizeof(_taskError), "Bad data");
        _resultReady = true;
        return;
    }

    WeatherData data;
    data.condition = conditionFrom(doc["cond"] | 2);
    data.temperatureC = doc["temp"];
    data.outdoorTemperatureC = data.temperatureC;
    data.humidity = doc["hum"] | 0;
    data.outdoorHumidity = data.humidity;
    data.hi = doc["hi"] | 0;
    data.lo = doc["lo"] | 0;
    data.uvIndex = doc["uv"] | 0;
    data.rainPercent = doc["rain"] | 0;
    strlcpy(data.summary, doc["sum"] | "", sizeof(data.summary));
    for (int i = 0; i < 3; i++) {
        data.forecast[i].condition = conditionFrom(forecast[i]["c"] | 2);
        data.forecast[i].hi = forecast[i]["hi"] | 0;
        data.forecast[i].lo = forecast[i]["lo"] | 0;
    }

    _result = data;
    _resultOk = true;
    _resultReady = true;
}
