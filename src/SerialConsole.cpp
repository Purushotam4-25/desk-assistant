#include "SerialConsole.h"
#include <stdio.h>
#include <string.h>

SerialConsole::SerialConsole(RTCManager& rtc) : _rtc(rtc) {}

void SerialConsole::update() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (_length > 0) {
                _line[_length] = 0;
                run(_line);
                _length = 0;
            }
        } else if (_length < sizeof(_line) - 1) {
            _line[_length++] = c;
        }
    }
}

void SerialConsole::run(const char* line) {
    int year, month, day, hour, minute, second;

    if (strcmp(line, "time") == 0) {
        Serial.printf("RTC: %s %s\n", _rtc.getDate().c_str(), _rtc.getTime().c_str());
    } else if (sscanf(line, "time %d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
        bool valid = year >= 2000 && year <= 2099 && month >= 1 && month <= 12 && day >= 1 && day <= 31 &&
                     hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59;
        if (valid) {
            _rtc.setDateTime(year, month, day, hour, minute, second);
            Serial.printf("RTC set to %s %s\n", _rtc.getDate().c_str(), _rtc.getTime().c_str());
        } else {
            Serial.println("Out of range");
        }
    } else {
        Serial.println("Commands: time | time YYYY-MM-DD HH:MM:SS");
    }
}
