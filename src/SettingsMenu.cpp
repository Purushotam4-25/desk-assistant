#include "SettingsMenu.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_system.h>

namespace {
constexpr const char* firmwareVersion = "0.1.0";

constexpr MenuItem setting(const char* label, SettingId id) {
    return {ItemKind::Setting, label, id, nullptr, nullptr, ActionId::Reboot};
}

constexpr MenuItem submenu(const char* label, const Menu& menu, InfoFn info = nullptr) {
    return {ItemKind::Submenu, label, SettingId::Count, &menu, info, ActionId::Reboot};
}

constexpr MenuItem info(const char* label, InfoFn fn) {
    return {ItemKind::Info, label, SettingId::Count, nullptr, fn, ActionId::Reboot};
}

constexpr MenuItem action(const char* label, ActionId id, InfoFn fn = nullptr) {
    return {ItemKind::Action, label, SettingId::Count, nullptr, fn, id};
}

template <size_t N>
constexpr Menu menu(const char* title, const MenuItem (&items)[N]) {
    return {title, items, static_cast<uint8_t>(N)};
}

void formatSpan(uint32_t seconds, char* out, size_t size) {
    if (seconds < 60) {
        snprintf(out, size, "%lu s", (unsigned long)seconds);
    } else if (seconds < 3600) {
        snprintf(out, size, "%lu min %lu s", (unsigned long)(seconds / 60), (unsigned long)(seconds % 60));
    } else if (seconds < 86400) {
        snprintf(out, size, "%lu h %lu min", (unsigned long)(seconds / 3600), (unsigned long)(seconds % 3600 / 60));
    } else {
        snprintf(out, size, "%lu d %lu h", (unsigned long)(seconds / 86400), (unsigned long)(seconds % 86400 / 3600));
    }
}

void formatAgo(uint32_t sinceMs, char* out, size_t size) {
    uint32_t seconds = (millis() - sinceMs) / 1000;
    if (seconds < 60) {
        snprintf(out, size, "%lu s ago", (unsigned long)seconds);
    } else if (seconds < 3600) {
        snprintf(out, size, "%lu min ago", (unsigned long)(seconds / 60));
    } else {
        snprintf(out, size, "%lu h ago", (unsigned long)(seconds / 3600));
    }
}

void formatAddress(const IPAddress& ip, char* out, size_t size) {
    if (ip == IPAddress(0, 0, 0, 0)) {
        snprintf(out, size, "-");
        return;
    }
    snprintf(out, size, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

Health wifiStatus(const SettingsContext& ctx, char* out, size_t size) {
    const char* name = ctx.wifi.stateName();
    snprintf(out, size, "%c%s", toupper(name[0]), name + 1);
    switch (ctx.wifi.state()) {
        case WifiService::State::Connected: return Health::Ok;
        case WifiService::State::Off: return Health::Off;
        default: return Health::Warn;
    }
}

Health wifiHub(const SettingsContext& ctx, char* out, size_t size) {
    if (ctx.wifi.connected() && ctx.config.config().wifiSsid[0]) {
        snprintf(out, size, "%s", ctx.config.config().wifiSsid);
        return Health::Ok;
    }
    return wifiStatus(ctx, out, size);
}

Health wifiSsid(const SettingsContext& ctx, char* out, size_t size) {
    const char* ssid = ctx.config.config().wifiSsid;
    snprintf(out, size, "%s", ssid[0] ? ssid : "-");
    return Health::None;
}

Health wifiIp(const SettingsContext& ctx, char* out, size_t size) {
    formatAddress(ctx.wifi.connected() ? WiFi.localIP() : IPAddress(0, 0, 0, 0), out, size);
    return Health::None;
}

Health wifiGateway(const SettingsContext& ctx, char* out, size_t size) {
    formatAddress(ctx.wifi.connected() ? WiFi.gatewayIP() : IPAddress(0, 0, 0, 0), out, size);
    return Health::None;
}

Health wifiDns(const SettingsContext& ctx, char* out, size_t size) {
    formatAddress(ctx.wifi.connected() ? WiFi.dnsIP() : IPAddress(0, 0, 0, 0), out, size);
    return Health::None;
}

Health wifiMac(const SettingsContext&, char* out, size_t size) {
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(out, size, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return Health::None;
}

Health wifiSignal(const SettingsContext& ctx, char* out, size_t size) {
    if (!ctx.wifi.connected()) {
        snprintf(out, size, "-");
        return Health::None;
    }
    int rssi = WiFi.RSSI();
    const char* word = rssi > -55 ? "Strong" : (rssi > -67 ? "Good" : (rssi > -75 ? "Fair" : "Weak"));
    snprintf(out, size, "%s %d dBm", word, rssi);
    return rssi > -67 ? Health::Ok : (rssi > -75 ? Health::Warn : Health::Error);
}

Health wifiChannel(const SettingsContext& ctx, char* out, size_t size) {
    if (ctx.wifi.connected()) {
        snprintf(out, size, "%d", WiFi.channel());
    } else {
        snprintf(out, size, "-");
    }
    return Health::None;
}

Health wifiUptime(const SettingsContext& ctx, char* out, size_t size) {
    if (ctx.wifi.connected()) {
        formatSpan((millis() - ctx.wifi.connectedSince()) / 1000, out, size);
    } else {
        snprintf(out, size, "-");
    }
    return Health::None;
}

Health wifiRetries(const SettingsContext& ctx, char* out, size_t size) {
    snprintf(out, size, "%u", ctx.wifi.retries());
    return Health::None;
}

Health wifiDropped(const SettingsContext& ctx, char* out, size_t size) {
    snprintf(out, size, "%s", ctx.wifi.lastDropReason());
    return Health::None;
}

Health syncStatus(const SettingsContext& ctx, char* out, size_t size) {
    if (!ctx.config.config().ntp) {
        snprintf(out, size, "Off");
        return Health::Off;
    }
    if (!ctx.wifi.connected()) {
        snprintf(out, size, "No Wi-Fi");
        return Health::Warn;
    }
    if (!ctx.timeSync.synced()) {
        snprintf(out, size, "Waiting");
        return Health::Warn;
    }
    snprintf(out, size, "Synced");
    return Health::Ok;
}

Health syncLast(const SettingsContext& ctx, char* out, size_t size) {
    if (ctx.timeSync.synced()) {
        formatAgo(ctx.timeSync.lastSyncMs(), out, size);
    } else {
        snprintf(out, size, "never");
    }
    return Health::None;
}

Health syncServer(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "pool.ntp.org");
    return Health::None;
}

Health syncZone(const SettingsContext& ctx, char* out, size_t size) {
    Settings::format(SettingId::TimeZone, ctx.config.value(SettingId::TimeZone), out, size);
    return Health::None;
}

Health weatherStatus(const SettingsContext& ctx, char* out, size_t size) {
    switch (ctx.weather.state()) {
        case WeatherService::State::NoBridge:
            snprintf(out, size, "No bridge");
            return Health::Off;
        case WeatherService::State::WaitingForWifi:
            snprintf(out, size, "No Wi-Fi");
            return Health::Warn;
        case WeatherService::State::Fetching:
            snprintf(out, size, "Updating");
            return ctx.weather.everOk() ? Health::Ok : Health::Warn;
        case WeatherService::State::Ok:
            snprintf(out, size, "Live");
            return Health::Ok;
        case WeatherService::State::Error:
            snprintf(out, size, ctx.weather.everOk() ? "Stale" : "Error");
            return ctx.weather.everOk() ? Health::Warn : Health::Error;
    }
    return Health::None;
}

Health weatherLastOk(const SettingsContext& ctx, char* out, size_t size) {
    if (ctx.weather.everOk()) {
        formatAgo(ctx.weather.lastOkMs(), out, size);
    } else {
        snprintf(out, size, "never");
    }
    return Health::None;
}

Health weatherError(const SettingsContext& ctx, char* out, size_t size) {
    snprintf(out, size, "%s", ctx.weather.lastError()[0] ? ctx.weather.lastError() : "none");
    return Health::None;
}

Health weatherSource(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "Open-Meteo");
    return Health::None;
}

Health spotifyStatus(const SettingsContext& ctx, char* out, size_t size) {
    switch (ctx.spotify.link()) {
        case SpotifyService::Link::NoBridge:
            snprintf(out, size, "No bridge");
            return Health::Off;
        case SpotifyService::Link::WaitingForWifi:
            snprintf(out, size, "No Wi-Fi");
            return Health::Warn;
        case SpotifyService::Link::Connecting:
            snprintf(out, size, "Not polled");
            return Health::Off;
        case SpotifyService::Link::Ok:
            snprintf(out, size, "Live");
            return Health::Ok;
        case SpotifyService::Link::NotLinked:
            snprintf(out, size, "Not linked");
            return Health::Warn;
        case SpotifyService::Link::PremiumRequired:
            snprintf(out, size, "Premium");
            return Health::Error;
        case SpotifyService::Link::Error:
            snprintf(out, size, ctx.spotify.everOk() ? "Stale" : "Error");
            return ctx.spotify.everOk() ? Health::Warn : Health::Error;
    }
    return Health::None;
}

Health spotifyLastOk(const SettingsContext& ctx, char* out, size_t size) {
    if (ctx.spotify.everOk()) {
        formatAgo(ctx.spotify.lastOkMs(), out, size);
    } else {
        snprintf(out, size, "never");
    }
    return Health::None;
}

Health spotifyError(const SettingsContext& ctx, char* out, size_t size) {
    snprintf(out, size, "%s", ctx.spotify.lastError()[0] ? ctx.spotify.lastError() : "none");
    return Health::None;
}

Health notBuilt(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "Not built");
    return Health::Off;
}

Health notSetUp(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "Not set up");
    return Health::Off;
}

Health never(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "never");
    return Health::None;
}

Health noneYet(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "none");
    return Health::None;
}

Health notLinked(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "Not linked");
    return Health::None;
}

Health zeroLoaded(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "0 loaded");
    return Health::None;
}

Health locationText(const SettingsContext& ctx, char* out, size_t size) {
    snprintf(out, size, "%.2f, %.2f", ctx.config.config().latitude, ctx.config.config().longitude);
    return Health::None;
}

Health comingSoon(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "Coming soon");
    return Health::Off;
}

Health fixedCities(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "5 fixed");
    return Health::None;
}

Health backlightNote(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "No backlight PWM");
    return Health::Off;
}

Health buzzerHardware(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "Not installed");
    return Health::Off;
}

template <SettingId Enabled, SettingId Pattern>
Health categorySummary(const SettingsContext& ctx, char* out, size_t size) {
    if (!ctx.config.value(Enabled)) {
        snprintf(out, size, "Off");
        return Health::Off;
    }
    char pattern[16];
    Settings::format(Pattern, ctx.config.value(Pattern), pattern, sizeof(pattern));
    snprintf(out, size, "On, %s", pattern);
    return Health::None;
}

Health sensorHealth(SensorState state) {
    switch (state) {
        case SensorState::Ok: return Health::Ok;
        case SensorState::Stale:
        case SensorState::OutOfRange: return Health::Warn;
        default: return Health::Error;
    }
}

Health sensorHub(const SettingsContext& ctx, char* out, size_t size) {
    if (ctx.status.hasClimate) {
        snprintf(out, size, "%.1f C  %d%%", ctx.status.temperatureC, (int)(ctx.status.humidity + 0.5f));
    } else {
        snprintf(out, size, "--");
    }
    return sensorHealth(ctx.sensors.sensorState());
}

Health sensorType(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "DHT11");
    return Health::None;
}

Health sensorStatus(const SettingsContext& ctx, char* out, size_t size) {
    SensorState state = ctx.sensors.sensorState();
    const char* names[] = {"OK", "Stale", "Error", "Out of range", "Absent"};
    snprintf(out, size, "%s", names[static_cast<uint8_t>(state)]);
    return sensorHealth(state);
}

Health sensorTemperature(const SettingsContext& ctx, char* out, size_t size) {
    if (ctx.status.hasClimate) {
        snprintf(out, size, "%.1f C", ctx.status.temperatureC);
    } else {
        snprintf(out, size, "--");
    }
    return Health::None;
}

Health sensorHumidity(const SettingsContext& ctx, char* out, size_t size) {
    if (ctx.status.hasClimate) {
        snprintf(out, size, "%d%%", (int)(ctx.status.humidity + 0.5f));
    } else {
        snprintf(out, size, "--");
    }
    return Health::None;
}

Health sensorLastRead(const SettingsContext& ctx, char* out, size_t size) {
    if (ctx.sensors.sensorEverRead()) {
        formatAgo(ctx.sensors.sensorLastGoodMs(), out, size);
    } else {
        snprintf(out, size, "never");
    }
    return Health::None;
}

Health storageText(const SettingsContext& ctx, char* out, size_t size) {
    uint32_t total = 0, free = 0;
    if (!ctx.sd.capacityMb(total, free, false)) {
        snprintf(out, size, "No card");
        return Health::Off;
    }
    snprintf(out, size, "%lu of %lu MB free", (unsigned long)free, (unsigned long)total);
    return free * 10 < total ? Health::Warn : Health::None;
}

Health aboutVersion(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "%s", firmwareVersion);
    return Health::None;
}

Health aboutBuild(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "%s", __DATE__);
    return Health::None;
}

Health aboutChip(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "%s rev %d", ESP.getChipModel(), ESP.getChipRevision());
    return Health::None;
}

Health aboutUptime(const SettingsContext&, char* out, size_t size) {
    formatSpan(millis() / 1000, out, size);
    return Health::None;
}

Health aboutMemory(const SettingsContext&, char* out, size_t size) {
    snprintf(out, size, "%lu KB free", (unsigned long)(ESP.getFreeHeap() / 1024));
    return Health::None;
}

Health aboutReset(const SettingsContext&, char* out, size_t size) {
    const char* name = "Unknown";
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON: name = "Power on"; break;
        case ESP_RST_SW: name = "Software"; break;
        case ESP_RST_EXT: name = "Reset pin"; break;
        case ESP_RST_PANIC: name = "Crash"; break;
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT: name = "Watchdog"; break;
        case ESP_RST_BROWNOUT: name = "Brownout"; break;
        case ESP_RST_DEEPSLEEP: name = "Deep sleep"; break;
        default: break;
    }
    snprintf(out, size, "%s", name);
    return Health::None;
}

constexpr MenuItem timeItems[] = {
    setting("Time format", SettingId::TimeFormat),
    setting("Date format", SettingId::DateFormat),
    setting("Time zone", SettingId::TimeZone),
    setting("Auto-sync", SettingId::AutoSync),
    info("Cities", fixedCities),
};
constexpr Menu timeMenu = menu("Time", timeItems);

constexpr MenuItem countdownItems[] = {
    setting("Duration", SettingId::TimerDefault),
    setting("Alert", SettingId::TimerAlert),
    setting("Auto-repeat", SettingId::TimerRepeat),
};
constexpr Menu countdownMenu = menu("Countdown", countdownItems);

constexpr MenuItem pomodoroItems[] = {
    setting("Focus", SettingId::PomoFocus),
    setting("Short break", SettingId::PomoShort),
    setting("Long break", SettingId::PomoLong),
    setting("Rounds", SettingId::PomoRounds),
    setting("Auto-start", SettingId::PomoAuto),
    setting("Alert", SettingId::PomoAlert),
    setting("On standby", SettingId::PomoStandby),
};
constexpr Menu pomodoroMenu = menu("Pomodoro", pomodoroItems);

constexpr MenuItem timerItems[] = {
    submenu("Countdown", countdownMenu),
    submenu("Pomodoro", pomodoroMenu),
};
constexpr Menu timerMenu = menu("Timer", timerItems);

constexpr MenuItem displayItems[] = {
    setting("Brightness", SettingId::Brightness),
    setting("Auto-dim", SettingId::AutoDim),
    setting("Dim level", SettingId::DimLevel),
    setting("Screen off", SettingId::ScreenOff),
    setting("Standby", SettingId::Standby),
    info("Note", backlightNote),
};
constexpr Menu displayMenu = menu("Display", displayItems);

constexpr MenuItem sensorAlertItems[] = {
    setting("Errors", SettingId::NotifSensor),
    setting("Buzzer", SettingId::PatternSensor),
};
constexpr Menu sensorAlertMenu = menu("Sensor alerts", sensorAlertItems);

constexpr MenuItem serviceAlertItems[] = {
    setting("Enabled", SettingId::NotifService),
    setting("Buzzer", SettingId::PatternService),
};
constexpr Menu serviceAlertMenu = menu("Service errors", serviceAlertItems);

constexpr MenuItem wifiAlertItems[] = {
    setting("Enabled", SettingId::NotifWifi),
    setting("Buzzer", SettingId::PatternWifi),
};
constexpr Menu wifiAlertMenu = menu("Wi-Fi lost", wifiAlertItems);

constexpr MenuItem timerAlertItems[] = {
    setting("Enabled", SettingId::NotifTimer),
    setting("Buzzer", SettingId::PatternTimer),
};
constexpr Menu timerAlertMenu = menu("Timer alerts", timerAlertItems);

constexpr MenuItem calendarAlertItems[] = {
    setting("Enabled", SettingId::NotifCalendar),
    setting("Buzzer", SettingId::PatternCalendar),
};
constexpr Menu calendarAlertMenu = menu("Calendar", calendarAlertItems);

constexpr MenuItem notificationItems[] = {
    submenu("Sensors", sensorAlertMenu, categorySummary<SettingId::NotifSensor, SettingId::PatternSensor>),
    submenu("Services", serviceAlertMenu, categorySummary<SettingId::NotifService, SettingId::PatternService>),
    submenu("Wi-Fi lost", wifiAlertMenu, categorySummary<SettingId::NotifWifi, SettingId::PatternWifi>),
    submenu("Timers", timerAlertMenu, categorySummary<SettingId::NotifTimer, SettingId::PatternTimer>),
    submenu("Calendar", calendarAlertMenu, categorySummary<SettingId::NotifCalendar, SettingId::PatternCalendar>),
    setting("Toast time", SettingId::ToastSeconds),
};
constexpr Menu notificationMenu = menu("Notifications", notificationItems);

constexpr MenuItem quietItems[] = {
    setting("Enabled", SettingId::QuietOn),
    setting("Start", SettingId::QuietStart),
    setting("End", SettingId::QuietEnd),
    setting("Mute buzzer", SettingId::QuietMute),
    setting("Dim screen", SettingId::QuietDim),
    setting("Timers ring", SettingId::QuietTimers),
    setting("Crit silent", SettingId::QuietSilent),
};
constexpr Menu quietMenu = menu("Quiet hours", quietItems);

constexpr MenuItem buzzerItems[] = {
    setting("Enabled", SettingId::BuzzerOn),
    setting("Volume", SettingId::BuzzerVolume),
    setting("Alarm max", SettingId::AlarmLength),
    action("Test sound", ActionId::BuzzerTest),
    info("Buzzer", buzzerHardware),
};
constexpr Menu buzzerMenu = menu("Buzzer", buzzerItems);

constexpr MenuItem alertItems[] = {
    submenu("Notify", notificationMenu),
    submenu("Quiet hours", quietMenu),
    submenu("Buzzer", buzzerMenu),
};
constexpr Menu alertMenu = menu("Alerts", alertItems);

constexpr MenuItem wifiItems[] = {
    info("Status", wifiStatus),
    info("SSID", wifiSsid),
    info("IP", wifiIp),
    info("Gateway", wifiGateway),
    info("DNS", wifiDns),
    info("MAC", wifiMac),
    info("Signal", wifiSignal),
    info("Channel", wifiChannel),
    info("Up for", wifiUptime),
    info("Retries", wifiRetries),
    info("Dropped", wifiDropped),
    action("Reconnect", ActionId::WifiReconnect),
    action("Phone setup", ActionId::PhoneSetup),
};
constexpr Menu wifiMenu = menu("Wi-Fi", wifiItems);

constexpr MenuItem bluetoothItems[] = {
    info("Status", notBuilt),
};
constexpr Menu bluetoothMenu = menu("Bluetooth", bluetoothItems);

constexpr MenuItem spotifyItems[] = {
    info("Status", spotifyStatus),
    info("Last OK", spotifyLastOk),
    info("Error", spotifyError),
    action("Test now", ActionId::SpotifyTest),
    setting("Poll every", SettingId::SpotifyPoll),
    action("Re-link", ActionId::Relink),
};
constexpr Menu spotifyMenu = menu("Spotify", spotifyItems);

constexpr MenuItem weatherItems[] = {
    info("Status", weatherStatus),
    info("Last OK", weatherLastOk),
    info("Error", weatherError),
    action("Test now", ActionId::WeatherTest),
    setting("Refresh", SettingId::WeatherEvery),
    info("City", locationText),
    info("Source", weatherSource),
};
constexpr Menu weatherMenu = menu("Weather", weatherItems);

constexpr MenuItem calendarItems[] = {
    info("Status", notSetUp),
    info("Last OK", never),
    info("Error", noneYet),
    action("Test now", ActionId::TestService),
    setting("Refresh", SettingId::CalendarEvery),
    info("Events", zeroLoaded),
    info("Account", notLinked),
};
constexpr Menu calendarMenu = menu("Calendar", calendarItems);

constexpr MenuItem syncItems[] = {
    info("Status", syncStatus),
    info("Last OK", syncLast),
    info("Server", syncServer),
    info("Zone", syncZone),
    setting("Auto-sync", SettingId::AutoSync),
};
constexpr Menu syncMenu = menu("Time sync", syncItems);

constexpr MenuItem diagnosticsItems[] = {
    info("Status", comingSoon),
    action("Run checks", ActionId::RunChecks),
};
constexpr Menu diagnosticsMenu = menu("Diagnostics", diagnosticsItems);

constexpr MenuItem connectionItems[] = {
    submenu("Wi-Fi", wifiMenu, wifiHub),
    submenu("Bluetooth", bluetoothMenu, notBuilt),
    submenu("Spotify", spotifyMenu, spotifyStatus),
    submenu("Weather", weatherMenu, weatherStatus),
    submenu("Calendar", calendarMenu, notSetUp),
    submenu("Time sync", syncMenu, syncStatus),
    submenu("Diagnostics", diagnosticsMenu),
};
constexpr Menu connectionMenu = menu("Connections", connectionItems);

constexpr MenuItem sensorLimitItems[] = {
    setting("Alert error", SettingId::SensorAlerts),
    setting("Use limits", SettingId::SensorLimits),
    setting("High temp", SettingId::SensorHigh),
    setting("Low temp", SettingId::SensorLow),
    setting("Repeat", SettingId::SensorRepeat),
};
constexpr Menu sensorLimitMenu = menu("Sensor alerts", sensorLimitItems);

constexpr MenuItem sensorDetailItems[] = {
    info("Type", sensorType),
    info("Status", sensorStatus),
    info("Temp", sensorTemperature),
    info("Humidity", sensorHumidity),
    info("Last read", sensorLastRead),
    setting("Temp offset", SettingId::SensorTempOffset),
    setting("Hum offset", SettingId::SensorHumOffset),
    setting("Read every", SettingId::SensorInterval),
    action("Re-init", ActionId::SensorReinit),
    submenu("Alerts", sensorLimitMenu),
};
constexpr Menu sensorDetailMenu = menu("Room sensor", sensorDetailItems);

constexpr MenuItem sensorItems[] = {
    submenu("Room", sensorDetailMenu, sensorHub),
};
constexpr Menu sensorMenu = menu("Sensors", sensorItems);

constexpr MenuItem systemItems[] = {
    info("Storage", storageText),
    setting("Log level", SettingId::LogLevel),
    setting("Debug info", SettingId::DebugOverlay),
    action("Reboot", ActionId::Reboot),
    action("Reset all", ActionId::ResetSettings),
    action("Factory", ActionId::FactoryReset),
};
constexpr Menu systemMenu = menu("System", systemItems);

constexpr MenuItem aboutItems[] = {
    info("Version", aboutVersion),
    info("Build", aboutBuild),
    info("Chip", aboutChip),
    info("Uptime", aboutUptime),
    info("Memory", aboutMemory),
    info("Reset", aboutReset),
};
constexpr Menu aboutMenu = menu("About", aboutItems);

constexpr MenuItem rootItems[] = {
    submenu("Time", timeMenu),
    submenu("Timer", timerMenu),
    submenu("Display", displayMenu),
    submenu("Alerts", alertMenu),
    submenu("Connections", connectionMenu),
    submenu("Sensors", sensorMenu),
    submenu("System", systemMenu),
    submenu("About", aboutMenu),
};
constexpr Menu rootMenu = menu("Settings", rootItems);
}

namespace SettingsMenu {
const Menu& root() {
    return rootMenu;
}
}
