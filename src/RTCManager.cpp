#include "RTCManager.h"

RTCManager::RTCManager(uint8_t clkPin, uint8_t datPin, uint8_t rstPin)
    : _wire(datPin, clkPin, rstPin), _rtc(_wire) {}

void RTCManager::begin() {
    _rtc.Begin();

    if (!_rtc.IsDateTimeValid()) {
        Serial.println("RTC has invalid DateTime. reset time");
        _rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
    }
    if (_rtc.GetIsWriteProtected()) {
        _rtc.SetIsWriteProtected(false);
    }
    if (!_rtc.GetIsRunning()) {
        _rtc.SetIsRunning(true);
    }
}

String RTCManager::getTime() {
    RtcDateTime time = _rtc.GetDateTime();
    char buf[9];
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u", time.Hour(), time.Minute(), time.Second());
    return String(buf);
}

String RTCManager::getDate() {
    RtcDateTime date = _rtc.GetDateTime();
    char buf[11];
    snprintf(buf, sizeof(buf), "%02u/%02u/%04u", date.Day(), date.Month(), date.Year());
    return String(buf);
}

void RTCManager::setDateTime(int year, int month, int day, int hour, int min, int sec) {
    _rtc.SetDateTime(RtcDateTime(year, month, day, hour, min, sec));
}
