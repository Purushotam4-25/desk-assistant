#include "WifiService.h"

namespace {
constexpr uint32_t attemptTimeoutMs = 15000;
constexpr uint32_t firstRetryMs = 5000;
constexpr uint32_t maxRetryMs = 60000;
constexpr const char* hostname = "desk-assistant";
}

void WifiService::begin(const Config& config) {
    _config = &config;
    if (config.wifiSsid[0] == '\0') {
        Serial.println("WiFi: no credentials, staying off");
        return;
    }

    _retryDelayMs = firstRetryMs;
    WiFi.persistent(false);
    WiFi.setHostname(hostname);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    startAttempt();
}

void WifiService::update() {
    if (_state == State::Off) return;

    wl_status_t status = WiFi.status();
    uint32_t elapsed = millis() - _stateSince;

    switch (_state) {
        case State::Connecting:
            if (status == WL_CONNECTED) {
                _retryDelayMs = firstRetryMs;
                Serial.printf("WiFi: connected, IP %s\n", WiFi.localIP().toString().c_str());
                setState(State::Connected);
            } else if (status == WL_NO_SSID_AVAIL || status == WL_CONNECT_FAILED || elapsed >= attemptTimeoutMs) {
                failAttempt(status);
            }
            break;
        case State::Connected:
            if (status != WL_CONNECTED) {
                Serial.printf("WiFi: connection lost, reconnecting in %lu s\n", (unsigned long)(_retryDelayMs / 1000));
                setState(State::Waiting);
            }
            break;
        case State::Waiting:
            if (elapsed >= _retryDelayMs) {
                _retryDelayMs = min(_retryDelayMs * 2, maxRetryMs);
                startAttempt();
            }
            break;
        default:
            break;
    }
}

const char* WifiService::stateName() const {
    switch (_state) {
        case State::Off: return "off";
        case State::Connecting: return "connecting";
        case State::Connected: return "connected";
        case State::Waiting: return "waiting to retry";
    }
    return "";
}

void WifiService::setState(State state) {
    _state = state;
    _stateSince = millis();
}

void WifiService::startAttempt() {
    WiFi.disconnect();
    WiFi.begin(_config->wifiSsid, _config->wifiPassword[0] ? _config->wifiPassword : nullptr);
    Serial.println("WiFi: connecting");
    setState(State::Connecting);
}

void WifiService::failAttempt(wl_status_t status) {
    WiFi.disconnect();
    Serial.printf("WiFi: attempt failed (status %d), retrying in %lu s\n", (int)status,
                  (unsigned long)(_retryDelayMs / 1000));
    setState(State::Waiting);
}
