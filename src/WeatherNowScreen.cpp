#include "WeatherNowScreen.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "WeatherWidgets.h"

extern TFT_eSPI tft;

namespace {
constexpr int contentHeight = 216;
constexpr int centerX = 160;
constexpr int iconSize = 100, iconGap = 32;
constexpr int tempScale = 4, tempRingGap = 10, tempRingRadius = 6;
constexpr int summaryMaxWidth = 150;
constexpr int limitScale = 2, limitCenterX = centerX;
constexpr int labelGap = 5, ringGap = 3, ringRadius = 3, pairGap = 28;

// The stack runs from the icon top (48) to the bottom of the limits row (190); shiftY centres it vertically.
constexpr int stackTop = 48, stackBottom = 190;
constexpr int shiftY = (contentHeight - (stackBottom - stackTop)) / 2 - stackTop;
constexpr int iconY = 44 + shiftY, tempDigitTop = 52 + shiftY, summaryY = 120 + shiftY, limitDigitTop = 162 + shiftY;

struct Limit {
    const char* label;
    char number[8];
    bool known;
    int labelWidth, numberWidth, width;

    Limit(const char* labelText, int value, bool isKnown) : label(labelText), known(isKnown) {
        if (known) {
            snprintf(number, sizeof(number), "%d", value);
        } else {
            strlcpy(number, "--", sizeof(number));
        }
        labelWidth = Widgets::inkWidth(label, Fonts::body, limitScale);
        numberWidth = Widgets::inkWidth(number, Fonts::body, limitScale);
        width = labelWidth + labelGap + numberWidth + (known ? ringGap + 2 * ringRadius + 1 : 0);
    }

    void draw(int x, int y) const {
        Widgets::inkText(label, x, y, Fonts::body, limitScale, Theme::textMuted);
        int numberX = x + labelWidth + labelGap;
        Widgets::inkText(number, numberX, y, Fonts::body, limitScale, known ? Theme::text : Theme::textDim);
        if (!known) return;
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
    if (!_weather.valid) {
        Widgets::Ink dashes = Widgets::ink("--", Fonts::body, tempScale);
        int dashesY = WeatherUi::yForDigitTop(Fonts::body, tempScale, tempDigitTop);
        Widgets::text("--", centerX - dashes.width / 2 - dashes.left, dashesY, Fonts::body, tempScale, Theme::textDim);
        drawLimits();
        return;
    }
    char temp[8];
    snprintf(temp, sizeof(temp), "%d", _weather.temperatureC);
    Widgets::Ink tempInk = Widgets::ink(temp, Fonts::body, tempScale);
    Widgets::Ink summaryInk = Widgets::ink(_weather.summary, Fonts::body, 1);
    int tempWidth = tempInk.width + tempRingGap + tempRingRadius + 1;
    int summaryWidth = summaryInk.width < summaryMaxWidth ? summaryInk.width : summaryMaxWidth;
    int textWidth = tempWidth > summaryWidth ? tempWidth : summaryWidth;

    int iconWidth = WeatherUi::iconWidth(_weather.condition, _weather.isDay, iconSize);
    int groupLeft = centerX - (iconWidth + iconGap + textWidth) / 2;
    int textLeft = groupLeft + iconWidth + iconGap;

    WeatherUi::icon(_weather.condition, _weather.isDay, groupLeft + iconWidth / 2 - iconSize / 2, iconY, iconSize,
                    Theme::background);

    int tempY = WeatherUi::yForDigitTop(Fonts::body, tempScale, tempDigitTop);
    Widgets::text(temp, textLeft - tempInk.left, tempY, Fonts::body, tempScale, Theme::text);
    WeatherUi::degree(textLeft + tempInk.width + tempRingGap, tempDigitTop + 8, tempRingRadius, Theme::text);

    Widgets::fitText(_weather.summary, textLeft - summaryInk.left, summaryY, summaryMaxWidth, Fonts::body, 1,
                     Theme::textMuted);

    drawLimits();
}

void WeatherNowScreen::drawLimits() {
    Limit high("H:", _weather.hi, _weather.valid);
    Limit low("L:", _weather.lo, _weather.valid);
    int y = WeatherUi::yForDigitTop(Fonts::body, limitScale, limitDigitTop);
    int x = limitCenterX - (high.width + pairGap + low.width) / 2;
    high.draw(x, y);
    low.draw(x + high.width + pairGap, y);
}
