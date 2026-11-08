#include "WeatherRoomsScreen.h"
#include <TFT_eSPI.h>
#include <math.h>
#include "Theme.h"
#include "Fonts.h"
#include "Icons.h"
#include "Widgets.h"
#include "WeatherWidgets.h"

extern TFT_eSPI tft;

namespace {
constexpr int panelX = 10, panelY = 20, panelW = 300, panelH = 184;
constexpr int dividerX = panelX + 210, panelRight = panelX + panelW;
constexpr int labelX = panelX + 16, labelScale = 2;
constexpr int tempTop = panelY + 65, humidityTop = panelY + 119, maxValueScale = 3;
constexpr int indoorX = panelX + 134, columnMargin = 10;
constexpr int headerCenterY = panelY + 23;
constexpr int valuesTop = panelY + 50, valuesHeight = 124;
constexpr int ringGap = 5, unitGap = 3;

int ringRadius(int scale) { return scale; }
int ringWidth(int scale) { return 2 * ringRadius(scale) + 1; }

int temperatureWidth(const char* number, int scale, bool withUnit) {
    int width = Widgets::inkWidth(number, Fonts::glcd, scale) + ringGap + ringWidth(scale);
    if (withUnit) width += unitGap + Widgets::inkWidth("C", Fonts::glcd, scale);
    return width;
}

void drawTemperature(const char* number, int x, int top, int scale, bool withUnit) {
    Widgets::inkText(number, x, top, Fonts::glcd, scale, Theme::text);
    int ringX = x + Widgets::inkWidth(number, Fonts::glcd, scale) + ringGap;
    WeatherUi::degree(ringX + ringRadius(scale), top + ringRadius(scale) + 1, ringRadius(scale), Theme::text);
    if (withUnit) {
        Widgets::inkText("C", ringX + ringWidth(scale) + unitGap, top, Fonts::glcd, scale, Theme::text);
    }
}

int fittingScale(const char* number, const char* humidity, int budget, bool withUnit) {
    int scale = maxValueScale;
    while (scale > 1 && (temperatureWidth(number, scale, withUnit) > budget ||
                         Widgets::inkWidth(humidity, Fonts::glcd, scale) > budget)) {
        scale--;
    }
    return scale;
}

int labelTopFor(int valueTop) {
    return valueTop + (7 * maxValueScale - 7 * labelScale) / 2;
}

void drawTree(int centerX, int centerY) {
    int x = centerX - 11, y = centerY - 14;
    tft.fillTriangle(x + 3, y + 12, x + 11, y, x + 19, y + 12, Theme::plug);
    tft.fillTriangle(x, y + 22, x + 11, y + 8, x + 22, y + 22, Theme::plug);
    tft.fillRect(x + 9, y + 22, 4, 6, Theme::plug);
}
}

WeatherRoomsScreen::WeatherRoomsScreen(const WeatherData& weather, const DeviceStatus& status, const NavState& nav)
    : SectionScreen(nav), _weather(weather), _status(status) {}

void WeatherRoomsScreen::resetKeys() {
    _indoorKey = -1;
    _outdoorKey = -1;
}

void WeatherRoomsScreen::drawFrame() {
    tft.fillRoundRect(panelX, panelY, panelW, panelH, 8, Theme::surface);
    tft.drawFastVLine(dividerX, panelY + 14, 156, Theme::outline);

    Icons::draw(Icon::Main, panelX + 29 - 32, headerCenterY - 32, Theme::plug, Theme::surface);
    int titleY = headerCenterY - Fonts::capCentre(Fonts::title, 2, 'I');
    Widgets::text("Indoor", panelX + 52, titleY, Fonts::title, 2, Theme::text);
    drawTree((dividerX + panelRight) / 2, headerCenterY);

    Widgets::text("Temp:", labelX, labelTopFor(tempTop), Fonts::glcd, labelScale, Theme::textMuted);
    Widgets::text("Humidity:", labelX, labelTopFor(humidityTop), Fonts::glcd, labelScale, Theme::textMuted);
}

void WeatherRoomsScreen::drawRegions() {
    drawIndoor();
    drawOutdoor();
}

void WeatherRoomsScreen::drawIndoor() {
    int32_t key = _status.hasClimate ? lroundf(_status.temperatureC) * 1000 + lroundf(_status.humidity) : -2;
    if (key == _indoorKey) return;
    _indoorKey = key;

    tft.fillRect(indoorX - 6, valuesTop, dividerX - indoorX + 2, valuesHeight, Theme::surface);

    if (!_status.hasClimate) {
        Widgets::inkText("--", indoorX, tempTop, Fonts::glcd, maxValueScale, Theme::textDim);
        Widgets::inkText("--", indoorX, humidityTop, Fonts::glcd, maxValueScale, Theme::textDim);
        return;
    }

    char temp[8], humidity[8];
    snprintf(temp, sizeof(temp), "%ld", lroundf(_status.temperatureC));
    snprintf(humidity, sizeof(humidity), "%ld%%", lroundf(_status.humidity));

    int scale = fittingScale(temp, humidity, dividerX - columnMargin - indoorX, true);
    int top = tempTop + (7 * maxValueScale - 7 * scale) / 2;
    int humidityY = humidityTop + (7 * maxValueScale - 7 * scale) / 2;
    drawTemperature(temp, indoorX, top, scale, true);
    Widgets::inkText(humidity, indoorX, humidityY, Fonts::glcd, scale, Theme::text);
}

void WeatherRoomsScreen::drawOutdoor() {
    int64_t key = _weather.revision;
    if (key == _outdoorKey) return;
    _outdoorKey = key;

    tft.fillRect(dividerX + 4, valuesTop, panelRight - dividerX - 8, valuesHeight, Theme::surface);

    if (!_weather.valid) {
        int dashesX = (dividerX + panelRight) / 2;
        Widgets::inkTextCentered("--", dashesX, tempTop, Fonts::glcd, maxValueScale, Theme::textDim);
        Widgets::inkTextCentered("--", dashesX, humidityTop, Fonts::glcd, maxValueScale, Theme::textDim);
        return;
    }

    char temp[8], humidity[8];
    snprintf(temp, sizeof(temp), "%d", _weather.outdoorTemperatureC);
    snprintf(humidity, sizeof(humidity), "%u%%", _weather.outdoorHumidity);

    int centerX = (dividerX + panelRight) / 2;
    int scale = fittingScale(temp, humidity, panelRight - dividerX - 2 * columnMargin, false);
    int top = tempTop + (7 * maxValueScale - 7 * scale) / 2;
    int humidityY = humidityTop + (7 * maxValueScale - 7 * scale) / 2;

    drawTemperature(temp, centerX - temperatureWidth(temp, scale, false) / 2, top, scale, false);
    Widgets::inkTextCentered(humidity, centerX, humidityY, Fonts::glcd, scale, Theme::text);
}
