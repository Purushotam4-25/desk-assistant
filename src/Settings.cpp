#include "Settings.h"
#include <stdio.h>
#include <stdlib.h>

namespace {
using K = SettingKind;
using F = SettingFormat;

const char* const timeFormatNames[] = {"12 hour", "24 hour"};
const char* const dateFormatNames[] = {"D/M/Y", "M/D/Y", "Y-M-D"};
const char* const patternNames[] = {"Off", "Short", "Double", "Triple", "Rising", "Alarm"};
const char* const autoDimNames[] = {"Off", "30 s", "1 min", "5 min", "10 min"};
const char* const screenOffNames[] = {"Never", "1 min", "5 min", "10 min", "30 min"};
const char* const standbyNames[] = {"Off", "1 min", "5 min", "10 min", "30 min"};
const char* const volumeNames[] = {"Low", "Medium", "High"};
const char* const alarmLengthNames[] = {"30 s", "1 min", "5 min", "Until off"};
const char* const spotifyPollNames[] = {"5 s", "10 s", "30 s", "1 min"};
const char* const weatherEveryNames[] = {"15 min", "30 min", "1 hour", "3 hours"};
const char* const calendarEveryNames[] = {"5 min", "15 min", "30 min", "1 hour"};
const char* const sensorIntervalNames[] = {"10 s", "30 s", "1 min", "5 min"};
const char* const repeatNames[] = {"Off", "1 hour"};
const char* const logLevelNames[] = {"Error", "Warn", "Info", "Debug"};

const SettingDef defs[] = {
    {"use24h", K::Choice, F::Plain, 0, 1, 1, 0, timeFormatNames},
    {"dateFmt", K::Choice, F::Plain, 0, 2, 1, 0, dateFormatNames},
    {"utcOff", K::Number, F::UtcOffset, -720, 840, 15, 330, nullptr},
    {"ntp", K::Toggle, F::Plain, 0, 1, 1, 1, nullptr},

    {"tmrDefault", K::Number, F::Duration, 30, 5970, 30, 600, nullptr},
    {"tmrAlert", K::Choice, F::Plain, 0, 5, 1, 3, patternNames},
    {"tmrRepeat", K::Toggle, F::Plain, 0, 1, 1, 0, nullptr},
    {"pomoFocus", K::Number, F::Minutes, 1, 60, 1, 25, nullptr},
    {"pomoShort", K::Number, F::Minutes, 1, 30, 1, 5, nullptr},
    {"pomoLong", K::Number, F::Minutes, 1, 60, 1, 15, nullptr},
    {"pomoRounds", K::Number, F::Plain, 2, 8, 1, 4, nullptr},
    {"pomoAuto", K::Toggle, F::Plain, 0, 1, 1, 1, nullptr},
    {"pomoAlert", K::Choice, F::Plain, 0, 5, 1, 2, patternNames},
    {"pomoStandby", K::Toggle, F::Plain, 0, 1, 1, 1, nullptr},

    {"bright", K::Number, F::Percent, 10, 100, 10, 80, nullptr},
    {"autoDim", K::Choice, F::Plain, 0, 4, 1, 2, autoDimNames},
    {"dimLevel", K::Number, F::Percent, 10, 50, 10, 20, nullptr},
    {"scrOff", K::Choice, F::Plain, 0, 4, 1, 0, screenOffNames},
    {"standby", K::Choice, F::Plain, 0, 4, 1, 2, standbyNames},

    {"nSensor", K::Toggle, F::Plain, 0, 1, 1, 1, nullptr},
    {"nSensorPat", K::Choice, F::Plain, 0, 5, 1, 3, patternNames},
    {"nService", K::Toggle, F::Plain, 0, 1, 1, 1, nullptr},
    {"nServicePat", K::Choice, F::Plain, 0, 5, 1, 2, patternNames},
    {"nWifi", K::Toggle, F::Plain, 0, 1, 1, 1, nullptr},
    {"nWifiPat", K::Choice, F::Plain, 0, 5, 1, 1, patternNames},
    {"nTimer", K::Toggle, F::Plain, 0, 1, 1, 1, nullptr},
    {"nTimerPat", K::Choice, F::Plain, 0, 5, 1, 5, patternNames},
    {"nCal", K::Toggle, F::Plain, 0, 1, 1, 1, nullptr},
    {"nCalPat", K::Choice, F::Plain, 0, 5, 1, 1, patternNames},
    {"toastSecs", K::Number, F::Seconds, 2, 10, 1, 4, nullptr},

    {"qhOn", K::Toggle, F::Plain, 0, 1, 1, 0, nullptr},
    {"qhStart", K::Number, F::ClockTime, 0, 1410, 30, 1320, nullptr},
    {"qhEnd", K::Number, F::ClockTime, 0, 1410, 30, 420, nullptr},
    {"qhMute", K::Toggle, F::Plain, 0, 1, 1, 1, nullptr},
    {"qhDim", K::Toggle, F::Plain, 0, 1, 1, 1, nullptr},
    {"qhTimers", K::Toggle, F::Plain, 0, 1, 1, 1, nullptr},
    {"qhSilent", K::Toggle, F::Plain, 0, 1, 1, 1, nullptr},

    {"buzzOn", K::Toggle, F::Plain, 0, 1, 1, 1, nullptr},
    {"buzzVol", K::Choice, F::Plain, 0, 2, 1, 1, volumeNames},
    {"buzzMax", K::Choice, F::Plain, 0, 3, 1, 1, alarmLengthNames},

    {"spotPoll", K::Choice, F::Plain, 0, 3, 1, 1, spotifyPollNames},
    {"wxEvery", K::Choice, F::Plain, 0, 3, 1, 2, weatherEveryNames},
    {"calEvery", K::Choice, F::Plain, 0, 3, 1, 1, calendarEveryNames},

    {"senEvery", K::Choice, F::Plain, 0, 3, 1, 1, sensorIntervalNames},
    {"senTOff", K::Number, F::TenthsC, -50, 50, 5, 0, nullptr},
    {"senHOff", K::Number, F::TenthsPercent, -100, 100, 10, 0, nullptr},
    {"senAlert", K::Toggle, F::Plain, 0, 1, 1, 1, nullptr},
    {"senLimits", K::Toggle, F::Plain, 0, 1, 1, 0, nullptr},
    {"senTHigh", K::Number, F::Celsius, 20, 50, 1, 40, nullptr},
    {"senTLow", K::Number, F::Celsius, 0, 20, 1, 5, nullptr},
    {"senRepeat", K::Choice, F::Plain, 0, 1, 1, 0, repeatNames},

    {"logLevel", K::Choice, F::Plain, 0, 3, 1, 2, logLevelNames},
    {"debugOvl", K::Toggle, F::Plain, 0, 1, 1, 0, nullptr},
};
static_assert(sizeof(defs) / sizeof(defs[0]) == Settings::count, "defs must match SettingId");
}

namespace Settings {
const SettingDef& def(SettingId id) {
    return defs[static_cast<uint8_t>(id)];
}

int16_t clamp(SettingId id, int value) {
    const SettingDef& d = def(id);
    if (value < d.min) value = d.min;
    if (value > d.max) value = d.max;
    return static_cast<int16_t>(value);
}

void format(SettingId id, int16_t value, char* out, size_t size) {
    const SettingDef& d = def(id);
    if (d.kind == SettingKind::Toggle) {
        snprintf(out, size, "%s", value ? "On" : "Off");
        return;
    }
    if (d.kind == SettingKind::Choice) {
        int index = value < d.min ? d.min : (value > d.max ? d.max : value);
        snprintf(out, size, "%s", d.choices[index]);
        return;
    }
    switch (d.format) {
        case SettingFormat::Minutes: snprintf(out, size, "%d min", value); break;
        case SettingFormat::Seconds: snprintf(out, size, "%d s", value); break;
        case SettingFormat::Duration: snprintf(out, size, "%d:%02d", value / 60, value % 60); break;
        case SettingFormat::UtcOffset:
            snprintf(out, size, "UTC%c%d:%02d", value < 0 ? '-' : '+', abs(value) / 60, abs(value) % 60);
            break;
        case SettingFormat::ClockTime: snprintf(out, size, "%02d:%02d", value / 60, value % 60); break;
        case SettingFormat::Percent: snprintf(out, size, "%d%%", value); break;
        case SettingFormat::TenthsC:
            snprintf(out, size, "%c%d.%d C", value < 0 ? '-' : '+', abs(value) / 10, abs(value) % 10);
            break;
        case SettingFormat::TenthsPercent:
            snprintf(out, size, "%c%d.%d%%", value < 0 ? '-' : '+', abs(value) / 10, abs(value) % 10);
            break;
        case SettingFormat::Celsius: snprintf(out, size, "%d C", value); break;
        default: snprintf(out, size, "%d", value); break;
    }
}
}
