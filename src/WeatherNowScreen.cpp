#include "WeatherNowScreen.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "WeatherWidgets.h"

extern TFT_eSPI tft;

namespace {
constexpr int contentHeight = 216;
constexpr int iconX = 28, iconY = 44, iconSize = 100;
constexpr int tempX = 160, tempDigitTop = 52, tempScale = 4;
constexpr int summaryY = 120, summaryMaxWidth = 150;
constexpr int limitDigitTop = 162, limitScale = 2, limitCenterX = 160;
constexpr int labelGap = 5, ringGap = 3, ringRadius = 3, pairGap = 28;

struct Limit {
    const char* label;
    char number[8];
    int labelWidth, numberWidth, width;

    Limit(const char* labelText, int value) : label(labelText) {
        snprintf(number, sizeof(number), "%d", value);
        labelWidth = Widgets::inkWidth(label, Fonts::body, limitScale);
        numberWidth = Widgets::inkWidth(number, Fonts::body, limitScale);
        width = labelWidth + labelGap + numberWidth + ringGap + 2 * ringRadius + 1;
    }

    void draw(int x, int y) const {
        Widgets::inkText(label, x, y, Fonts::body, limitScale, Theme::textMuted);
        int numberX = x + labelWidth + labelGap;
        Widgets::inkText(number, numberX, y, Fonts::body, limitScale, Theme::text);
        int ringX = numberX + numberWidth + ringGap + ringRadius;
        WeatherUi::degree(ringX, limitDigitTop + ringRadius + 1, ringRadius, Theme::text);
    }
};
}

WeatherNowScreen::WeatherNowScreen(const WeatherData& weather, const NavState& nav)
    : SectionScreen(nav), _weather(weather) {}

void WeatherNowScreen::resetKeys() {
    _dataKey = -1;
}

void WeatherNowScreen::drawRegions() {
    if (_weather.revision == _dataKey) return;
    _dataKey = _weather.revision;

    tft.fillRect(0, 0, 320, contentHeight, Theme::background);
    WeatherUi::icon(_weather.condition, iconX, iconY, iconSize, Theme::background);

    char temp[8];
    snprintf(temp, sizeof(temp), "%d", _weather.temperatureC);
    int tempY = WeatherUi::yForDigitTop(Fonts::body, tempScale, tempDigitTop);
    Widgets::text(temp, tempX, tempY, Fonts::body, tempScale, Theme::text);
    Widgets::Ink tempInk = Widgets::ink(temp, Fonts::body, tempScale);
    WeatherUi::degree(tempX + tempInk.left + tempInk.width + 10, tempDigitTop + 8, 6, Theme::text);

    Widgets::fitText(_weather.summary, tempX, summaryY, summaryMaxWidth, Fonts::body, 1, Theme::textMuted);

    drawLimits();
}

void WeatherNowScreen::drawLimits() {
    Limit high("H:", _weather.hi);
    Limit low("L:", _weather.lo);
    int y = WeatherUi::yForDigitTop(Fonts::body, limitScale, limitDigitTop);
    int x = limitCenterX - (high.width + pairGap + low.width) / 2;
    high.draw(x, y);
    low.draw(x + high.width + pairGap, y);
}
