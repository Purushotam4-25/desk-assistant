#pragma once
#include <stdint.h>
#include <stddef.h>

enum class SettingId : uint8_t {
    TimeFormat, DateFormat, TimeZone, AutoSync,
    TimerDefault, TimerAlert, TimerRepeat,
    PomoFocus, PomoShort, PomoLong, PomoRounds, PomoAuto, PomoAlert, PomoStandby,
    Brightness, AutoDim, DimLevel, ScreenOff, Standby,
    NotifSensor, PatternSensor, NotifService, PatternService, NotifWifi, PatternWifi,
    NotifTimer, PatternTimer, NotifCalendar, PatternCalendar, ToastSeconds,
    QuietOn, QuietStart, QuietEnd, QuietMute, QuietDim, QuietTimers, QuietSilent,
    BuzzerOn, BuzzerVolume, AlarmLength,
    SpotifyPoll, WeatherEvery, CalendarEvery,
    SensorInterval, SensorTempOffset, SensorHumOffset, SensorAlerts, SensorLimits, SensorHigh, SensorLow, SensorRepeat,
    LogLevel, DebugOverlay,
    Count
};

enum class SettingKind : uint8_t { Toggle, Number, Choice };

enum class SettingFormat : uint8_t { Plain, Minutes, Seconds, Duration, UtcOffset, ClockTime, Percent, TenthsC, TenthsPercent, Celsius };

struct SettingDef {
    const char* key;
    SettingKind kind;
    SettingFormat format;
    int16_t min;
    int16_t max;
    int16_t step;
    int16_t fallback;
    const char* const* choices;
};

namespace Settings {
constexpr uint8_t count = static_cast<uint8_t>(SettingId::Count);
static_assert(count <= 64, "pending flags are a 64-bit mask");

const SettingDef& def(SettingId id);
int16_t clamp(SettingId id, int value);
void format(SettingId id, int16_t value, char* out, size_t size);
}
