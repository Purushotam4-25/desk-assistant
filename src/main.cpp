#undef FILE_READ
#undef FILE_WRITE

// Unused, but without it PlatformIO's dependency finder skips the networking libs that lib_deps needs and the build fails.
#include <FS.h>
#include <gpio_viewer.h>
#include <TFT_eSPI.h>
#include "SDManager.h"
#include "ConfigService.h"
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
#include "WeatherNowScreen.h"
#include "WeatherForecastScreen.h"
#include "WeatherRoomsScreen.h"
#include "WeatherAirScreen.h"
#include "TimerService.h"
#include "TimerScreen.h"
#include "PomodoroScreen.h"
#include "WorldClockScreen.h"
#include "MusicPlayer.h"
#include "MusicNowScreen.h"
#include "MusicTracksScreen.h"
#include "MusicLyricsScreen.h"

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

WeatherData sampleWeather() {
    WeatherData w;
    w.revision = 1;
    w.condition = WeatherCondition::MostlyClear;
    w.temperatureC = 18;
    w.hi = 23;
    w.lo = 14;
    strlcpy(w.summary, "Mostly clear", sizeof(w.summary));
    w.forecast[0] = {WeatherCondition::Clear, 14, 24};
    w.forecast[1] = {WeatherCondition::Cloudy, 13, 20};
    w.forecast[2] = {WeatherCondition::Rain, 11, 17};
    w.outdoorTemperatureC = 17;
    w.outdoorHumidity = 62;
    w.humidity = 62;
    w.uvIndex = 5;
    w.rainPercent = 10;
    return w;
}

TFT_eSPI tft = TFT_eSPI();
SDManager sdManager(5);
ConfigService configService;
RotaryEncoderManager encoder(32, 33, 15);
RTCManager rtc(27, 26, 25);
DHTManager dhtSensor(4);
StatusService statusService(rtc, dhtSensor);
SerialConsole console(rtc);
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

WeatherData weather = sampleWeather();
WeatherNowScreen weatherNowScreen(weather, nav);
WeatherForecastScreen weatherForecastScreen(weather, statusService.status(), nav);
WeatherRoomsScreen weatherRoomsScreen(weather, statusService.status(), nav);
WeatherAirScreen weatherAirScreen(weather, nav);

TimerService timerService;
TimerScreen timerScreen(nav, timerService);
PomodoroScreen pomodoroScreen(nav, timerService);

WorldClockScreen worldClockScreen(statusService.status(), configService);

MusicPlayer musicPlayer;
MusicLyricsScreen musicLyricsScreen(musicPlayer, nav);
MusicNowScreen musicNowScreen(musicPlayer, nav, musicLyricsScreen);
MusicTracksScreen musicTracksScreen(musicPlayer, nav);

Screen* const mainSubPages[] = {&mainScreen, &todayScreen, &focusScreen, &quickMenuScreen};
Screen* const weatherSubPages[] = {&weatherNowScreen, &weatherForecastScreen, &weatherRoomsScreen, &weatherAirScreen};
Screen* const worldSubPages[] = {&worldClockScreen};
Screen* const timerSubPages[] = {&timerScreen, &pomodoroScreen};
Screen* const musicSubPages[] = {&musicNowScreen, &musicTracksScreen};
const Page pages[] = {
    {"Main", Icon::Main, mainSubPages, sizeof(mainSubPages) / sizeof(mainSubPages[0])},
    {"Weather", Icon::Weather, weatherSubPages, sizeof(weatherSubPages) / sizeof(weatherSubPages[0])},
    {"World", Icon::World, worldSubPages, sizeof(worldSubPages) / sizeof(worldSubPages[0])},
    {"Timer", Icon::Timer, timerSubPages, sizeof(timerSubPages) / sizeof(timerSubPages[0])},
    {"Music", Icon::Music, musicSubPages, sizeof(musicSubPages) / sizeof(musicSubPages[0])},
};
constexpr uint8_t pageCount = sizeof(pages) / sizeof(pages[0]);

ScreensMenu screensMenu(pages, pageCount);
ScreenManager screens(input, nav, pages, pageCount, screensMenu);

void setup() {
    Serial.begin(115200);

    rtc.begin();
    dhtSensor.begin();
    encoder.begin();

    bool sdOk = sdManager.begin();
    Serial.println(sdOk ? "SD init success" : "SD init failed");
    statusService.setSdOk(sdOk);
    configService.begin(sdManager);
    statusService.update();
    focusSession.seedWeek(sampleWeekMinutes);
    musicPlayer.setAlbum(sampleAlbum, sampleTracks, sizeof(sampleTracks) / sizeof(sampleTracks[0]));

    tft.init();
    tft.setRotation(2);

    screens.begin();
}

void loop() {
    console.update();
    configService.update();
    statusService.update();
    focusSession.update();
    timerService.update();
    musicPlayer.update();
    screens.update();
    screens.draw();
}
