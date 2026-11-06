#undef FILE_READ
#undef FILE_WRITE

// Unused, but without it PlatformIO's dependency finder skips the networking libs that lib_deps needs and the build fails.
#include <FS.h>
#include <gpio_viewer.h>
#include <TFT_eSPI.h>
#include "SDManager.h"
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
#include "WeatherData.h"
#include "WeatherNowScreen.h"
#include "WeatherForecastScreen.h"
#include "WeatherRoomsScreen.h"
#include "WeatherAirScreen.h"

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

WeatherData weather = sampleWeather();
WeatherNowScreen weatherNowScreen(weather, nav);
WeatherForecastScreen weatherForecastScreen(weather, statusService.status(), nav);
WeatherRoomsScreen weatherRoomsScreen(weather, statusService.status(), nav);
WeatherAirScreen weatherAirScreen(weather, nav);

Screen* const mainSubPages[] = {&mainScreen, &todayScreen, &focusScreen};
Screen* const weatherSubPages[] = {&weatherNowScreen, &weatherForecastScreen, &weatherRoomsScreen, &weatherAirScreen};
const Page pages[] = {
    {"Main", Icon::Main, mainSubPages, sizeof(mainSubPages) / sizeof(mainSubPages[0])},
    {"Weather", Icon::Weather, weatherSubPages, sizeof(weatherSubPages) / sizeof(weatherSubPages[0])},
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
    statusService.update();
    focusSession.seedWeek(sampleWeekMinutes);

    tft.init();
    tft.setRotation(2);

    screens.begin();
}

void loop() {
    console.update();
    statusService.update();
    focusSession.update();
    screens.update();
    screens.draw();
}
