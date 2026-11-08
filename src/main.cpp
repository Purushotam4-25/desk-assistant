#undef FILE_READ
#undef FILE_WRITE

// Unused, but without it PlatformIO's dependency finder skips the networking libs that lib_deps needs and the build fails.
#include <FS.h>
#include <gpio_viewer.h>
#include <TFT_eSPI.h>
#include "SDManager.h"
#include "ConfigService.h"
#include "WifiService.h"
#include "TimeSync.h"
#include "DHTManager.h"
#include "RTCManager.h"
#include "rot_encoder.h"
#include "TapButton.h"
#include "InputRouter.h"
#include "NavState.h"
#include "Page.h"
#include "ScreensMenu.h"
#include "ScreenManager.h"
#include "StatusService.h"
#include "SerialConsole.h"
#include "Agenda.h"
#include "MainScreen.h"
#include "TodayScreen.h"
#include "FocusSession.h"
#include "FocusScreen.h"
#include "FocusStatsScreen.h"
#include "QuickSettings.h"
#include "QuickMenuScreen.h"
#include "WeatherData.h"
#include "WeatherService.h"
#include "SunCalc.h"
#include "DateMath.h"
#include "WeatherNowScreen.h"
#include "WeatherForecastScreen.h"
#include "WeatherRoomsScreen.h"
#include "WeatherAirScreen.h"
#include "TimerService.h"
#include "TimerScreen.h"
#include "PomodoroScreen.h"
#include "WorldClockScreen.h"
#include "CalendarDayScreen.h"
#include "MusicPlayer.h"
#include "MusicNowScreen.h"
#include "MusicTracksScreen.h"
#include "MusicLyricsScreen.h"
#include "SettingsScreen.h"

const MusicLyric lowTideLyrics[] = {
    {8, "Footprints fade slowly", "along the water line"},
    {20, "The lamps come on one", "by one across the bay"},
    {32, "I count the quiet hours", "till the morning sings"},
    {44, "Salt wind in my coat", "and nowhere left to be"},
    {56, "Let the tide decide", "which way we drift"},
    {68, "Low tide, low tide", "carry me home"},
};

const MusicLyric paperSkyLyrics[] = {
    {6, "Folded maps and paper", "wings above the street"},
    {18, "Nobody looks up", "so I keep it to myself"},
    {30, "Kite string in my hand", "pulling at the grey"},
    {42, "Come down slow", "come down kind"},
    {54, "Paper sky, paper sky", "hold the light a while"},
};

const MusicLyric northboundLyrics[] = {
    {7, "Headlights on the ridge", "counting every mile"},
    {19, "The radio hums low", "a song I almost know"},
    {31, "Northbound, northbound", "never looking back"},
    {43, "Dawn is just a rumour", "somewhere past the hill"},
    {55, "We keep the engine warm", "and the window down"},
};

const MusicTrack sampleTracks[] = {
    {"Low Tide", 187, lowTideLyrics, sizeof(lowTideLyrics) / sizeof(lowTideLyrics[0])},
    {"Paper Sky", 203, paperSkyLyrics, sizeof(paperSkyLyrics) / sizeof(paperSkyLyrics[0])},
    {"Northbound", 226, northboundLyrics, sizeof(northboundLyrics) / sizeof(northboundLyrics[0])},
    {"Amber", 158, nullptr, 0},
    {"Static", 241, nullptr, 0},
    {"Hush", 132, nullptr, 0},
    {"Slow Burn", 214, nullptr, 0},
    {"Lantern", 195, nullptr, 0},
    {"Drift", 176, nullptr, 0},
    {"Afterglow", 252, nullptr, 0},
    {"Daybreak", 168, nullptr, 0},
};

const MusicAlbum sampleAlbum = {"Tidal", "Kestrel", "2024"};

const AgendaEvent sampleEvents[] = {
    {8 * 60 + 30, 9 * 60, "Standup"},
    {10 * 60, 11 * 60, "Design review"},
    {12 * 60 + 30, 13 * 60 + 30, "Lunch"},
    {15 * 60, 16 * 60, "Deep work"},
    {18 * 60, 19 * 60, "Gym"},
};

const uint16_t sampleWeekMinutes[FocusSession::weekDays] = {95, 140, 60, 110, 85, 30, 45};

TFT_eSPI tft = TFT_eSPI();
SDManager sdManager(5);
ConfigService configService;
WifiService wifi;
RotaryEncoderManager encoder(32, 33, 15);
RTCManager rtc(27, 26, 25);
DHTManager dhtSensor(4);
StatusService statusService(rtc, dhtSensor);
SerialConsole console(rtc, wifi);
TimeSync timeSync(rtc, configService, wifi);
Agenda agenda(sampleEvents, sizeof(sampleEvents) / sizeof(sampleEvents[0]));
TapButton leftButton(21, true);
TapButton rightButton(22, true);
InputRouter input(encoder, leftButton, rightButton);

NavState nav;
MainScreen mainScreen(statusService.status(), agenda, nav);
TodayScreen todayScreen(statusService.status(), agenda, nav);
FocusSession focusSession(statusService.status(), 120);
FocusStatsScreen focusStatsScreen(statusService.status(), focusSession, nav);
FocusScreen focusScreen(statusService.status(), focusSession, nav, focusStatsScreen);
QuickSettings quickSettings;
QuickMenuScreen quickMenuScreen(statusService.status(), nav, quickSettings);

WeatherData weather;
WeatherService weatherService(configService, wifi);
WeatherNowScreen weatherNowScreen(weather, nav);
WeatherForecastScreen weatherForecastScreen(weather, statusService.status(), nav);
WeatherRoomsScreen weatherRoomsScreen(weather, statusService.status(), nav);
WeatherAirScreen weatherAirScreen(weather, nav);

TimerService timerService;
TimerScreen timerScreen(nav, timerService);
PomodoroScreen pomodoroScreen(nav, timerService);

WorldClockScreen worldClockScreen(statusService.status(), configService);

CalendarDayScreen calendarMorning(statusService.status(), agenda, CalendarDay::morningStartHour);
CalendarDayScreen calendarAfternoon(statusService.status(), agenda, CalendarDay::afternoonStartHour);

MusicPlayer musicPlayer;
MusicLyricsScreen musicLyricsScreen(musicPlayer, nav);
MusicNowScreen musicNowScreen(musicPlayer, nav, musicLyricsScreen);
MusicTracksScreen musicTracksScreen(musicPlayer, nav);

void updateDaylight() {
    static int32_t lastStamp = -1;
    const DeviceStatus& status = statusService.status();
    int32_t days = Date::daysFromCivil(status.now.year, status.now.month, status.now.day);
    int32_t stamp = days * 1440 + status.minuteOfDay();
    if (stamp == lastStamp) return;
    lastStamp = stamp;

    const Config& config = configService.config();
    bool daylight = SunCalc::isDaylight(config.latitude, config.longitude, days, config.utcOffsetMinutes, status.minuteOfDay());
    if (daylight != weather.isDay) {
        weather.isDay = daylight;
        weather.revision++;
    }
}

void rebootDevice() {
    configService.commit();
    ESP.restart();
}

void refreshWeather() {
    weatherService.refreshNow();
}

void reconnectWifi() {
    wifi.reconnect();
}

void reinitSensor() {
    statusService.reinitSensor();
}

SettingsContext settingsContext = {configService, statusService.status(), statusService, wifi, timeSync, sdManager, weatherService};
SettingsActions settingsActions = {refreshWeather, rebootDevice, reconnectWifi, reinitSensor};
SettingsScreen settingsScreen(settingsContext, settingsActions);

Screen* const mainSubPages[] = {&mainScreen, &todayScreen, &focusScreen, &quickMenuScreen};
Screen* const calendarSubPages[] = {&calendarMorning, &calendarAfternoon};
Screen* const weatherSubPages[] = {&weatherNowScreen, &weatherForecastScreen, &weatherRoomsScreen, &weatherAirScreen};
Screen* const worldSubPages[] = {&worldClockScreen};
Screen* const timerSubPages[] = {&timerScreen, &pomodoroScreen};
Screen* const musicSubPages[] = {&musicNowScreen, &musicTracksScreen};
Screen* const settingsSubPages[] = {&settingsScreen};
uint8_t calendarEntry() {
    uint8_t index = CalendarDayScreen::windowFor(statusService.status().minuteOfDay());
    (index == 0 ? calendarMorning : calendarAfternoon).followNow();
    return index;
}

uint8_t settingsEntry() {
    settingsScreen.openRoot();
    return 0;
}

const Page pages[] = {
    {"Main", Icon::Main, mainSubPages, sizeof(mainSubPages) / sizeof(mainSubPages[0])},
    {"Calendar", Icon::Calendar, calendarSubPages, sizeof(calendarSubPages) / sizeof(calendarSubPages[0]), calendarEntry},
    {"Weather", Icon::Weather, weatherSubPages, sizeof(weatherSubPages) / sizeof(weatherSubPages[0])},
    {"World", Icon::World, worldSubPages, sizeof(worldSubPages) / sizeof(worldSubPages[0])},
    {"Timer", Icon::Timer, timerSubPages, sizeof(timerSubPages) / sizeof(timerSubPages[0])},
    {"Music", Icon::Music, musicSubPages, sizeof(musicSubPages) / sizeof(musicSubPages[0])},
    {"Settings", Icon::Settings, settingsSubPages, sizeof(settingsSubPages) / sizeof(settingsSubPages[0]), settingsEntry},
};
constexpr uint8_t pageCount = sizeof(pages) / sizeof(pages[0]);

const uint16_t sensorIntervalSeconds[] = {10, 30, 60, 300};

struct AppliedSettings {
    int16_t timerDefault = -1;
    int16_t focus = -1;
    int16_t shortBreak = -1;
    int16_t longBreak = -1;
    int16_t sensorInterval = -1;
    int16_t timeFormat = -1;
    int16_t dateFormat = -1;
    int16_t timerRepeat = -1;
    int16_t pomoAuto = -1;
    int16_t tempOffset = 0x7FFF;
    int16_t humidityOffset = 0x7FFF;
} applied;
uint32_t appliedRevision = UINT32_MAX;

void applySettings() {
    if (configService.revision() == appliedRevision) return;
    bool deferred = false;

    int16_t timerDefault = configService.value(SettingId::TimerDefault);
    if (timerDefault != applied.timerDefault) {
        if (timerService.timerState() == TimerState::Set) {
            timerService.setTimerSetSeconds(timerDefault);
            applied.timerDefault = timerDefault;
        } else {
            deferred = true;
        }
    }

    int16_t focus = configService.value(SettingId::PomoFocus);
    if (focus != applied.focus) {
        if (timerService.pomoState() == PomoState::Set) {
            timerService.setFocusMinutes(focus);
            applied.focus = focus;
        } else {
            deferred = true;
        }
    }

    int16_t shortBreak = configService.value(SettingId::PomoShort);
    int16_t longBreak = configService.value(SettingId::PomoLong);
    if (shortBreak != applied.shortBreak || longBreak != applied.longBreak) {
        timerService.setBreakMinutes(shortBreak, longBreak);
        applied.shortBreak = shortBreak;
        applied.longBreak = longBreak;
    }

    int16_t timeFormat = configService.value(SettingId::TimeFormat);
    int16_t dateFormat = configService.value(SettingId::DateFormat);
    if (timeFormat != applied.timeFormat || dateFormat != applied.dateFormat) {
        statusService.setFormats(timeFormat, dateFormat);
        applied.timeFormat = timeFormat;
        applied.dateFormat = dateFormat;
    }

    int16_t timerRepeat = configService.value(SettingId::TimerRepeat);
    int16_t pomoAuto = configService.value(SettingId::PomoAuto);
    if (timerRepeat != applied.timerRepeat || pomoAuto != applied.pomoAuto) {
        timerService.setTimerRepeat(timerRepeat);
        timerService.setPomoAutoStart(pomoAuto);
        applied.timerRepeat = timerRepeat;
        applied.pomoAuto = pomoAuto;
    }

    int16_t interval = configService.value(SettingId::SensorInterval);
    int16_t tempOffset = configService.value(SettingId::SensorTempOffset);
    int16_t humidityOffset = configService.value(SettingId::SensorHumOffset);
    if (interval != applied.sensorInterval || tempOffset != applied.tempOffset || humidityOffset != applied.humidityOffset) {
        statusService.setSensorTuning(sensorIntervalSeconds[interval] * 1000UL, tempOffset, humidityOffset);
        applied.sensorInterval = interval;
        applied.tempOffset = tempOffset;
        applied.humidityOffset = humidityOffset;
    }

    if (!deferred) appliedRevision = configService.revision();
}

ScreensMenu screensMenu(pages, pageCount);
ScreenManager screens(input, nav, pages, pageCount, screensMenu);

void setup() {
    Serial.begin(115200);

    rtc.begin();
    dhtSensor.begin();
    encoder.begin();

    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    bool sdOk = sdManager.begin();
    Serial.println(sdOk ? "SD init success" : "SD init failed");
    statusService.setSdOk(sdOk);
    configService.begin(sdManager);
    applySettings();
    statusService.update();
    focusSession.seedWeek(sampleWeekMinutes);
    musicPlayer.setAlbum(sampleAlbum, sampleTracks, sizeof(sampleTracks) / sizeof(sampleTracks[0]));

    tft.init();
    tft.setRotation(2);

    screens.begin();
    wifi.begin(configService.config());
    weatherService.begin();
}

void loop() {
    console.update();
    configService.update();
    applySettings();
    wifi.update();
    timeSync.update();
    weatherService.update(weather);
    statusService.update();
    updateDaylight();
    focusSession.update();
    timerService.update();
    musicPlayer.update();
    screens.update();
    screens.draw();
}
