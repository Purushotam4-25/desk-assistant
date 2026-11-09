#pragma once
#include <stddef.h>
#include "CalendarService.h"
#include "ConfigService.h"
#include "DeviceStatus.h"
#include "SDManager.h"
#include "Settings.h"
#include "SettingsWidgets.h"
#include "SpotifyService.h"
#include "StatusService.h"
#include "TimeSync.h"
#include "WeatherService.h"
#include "WifiService.h"

struct SettingsContext {
    ConfigService& config;
    const DeviceStatus& status;
    const StatusService& sensors;
    const WifiService& wifi;
    const TimeSync& timeSync;
    SDManager& sd;
    const WeatherService& weather;
    const SpotifyService& spotify;
    const CalendarService& calendar;
};

struct SettingsActions {
    void (*weatherRefresh)();
    void (*spotifyRefresh)();
    void (*calendarRefresh)();
    void (*reboot)();
    void (*wifiReconnect)();
    void (*sensorReinit)();
};

enum class ItemKind : uint8_t { Setting, Submenu, Info, Action };
enum class ActionId : uint8_t {
    Reboot, ResetSettings, FactoryReset, WifiReconnect, SensorReinit, PhoneSetup, BuzzerTest, CalendarTest, WeatherTest, SpotifyTest, Relink, RunChecks
};

using InfoFn = Health (*)(const SettingsContext& ctx, char* out, size_t size);

struct Menu;

struct MenuItem {
    ItemKind kind;
    const char* label;
    SettingId setting;
    const Menu* submenu;
    InfoFn info;
    ActionId action;
};

struct Menu {
    const char* title;
    const MenuItem* items;
    uint8_t count;
};

namespace SettingsMenu {
const Menu& root();
}
