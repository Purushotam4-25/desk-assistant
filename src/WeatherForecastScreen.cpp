#include "WeatherForecastScreen.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "WeatherWidgets.h"

extern TFT_eSPI tft;

namespace {
constexpr int cardsY = 52, cardW = 96, cardH = 118, firstX = 8, stepX = 104;
constexpr int iconOffsetX = 20, iconOffsetY = 34, iconSize = 56;
constexpr int rangeOffsetY = 96, rangeGap = 16;
}

WeatherForecastScreen::WeatherForecastScreen(const WeatherData& weather, const DeviceStatus& status, const NavState& nav)
    : SectionScreen(nav), _weather(weather), _status(status) {}

void WeatherForecastScreen::resetKeys() {
    _dataKey = -1;
}

void WeatherForecastScreen::drawRegions() {
    int64_t key = (int64_t)_weather.revision * 8 + _status.now.weekday % 7;
    if (key == _dataKey) return;
    _dataKey = key;

    for (int slot = 0; slot < 3; slot++) drawDay(slot);
}

void WeatherForecastScreen::drawDay(int slot) {
    const ForecastDay& day = _weather.forecast[slot];
    int x = firstX + slot * stepX;
    const char* name = WeatherUi::weekdayShort[(_status.now.weekday + 1 + slot) % 7];

    tft.fillRoundRect(x, cardsY, cardW, cardH, 6, Theme::surface);

    int nameWidth = Widgets::textWidth(name, Fonts::title, 2);
    Widgets::text(name, x + (cardW - nameWidth) / 2, cardsY + 2, Fonts::title, 2, Theme::textMuted);

    WeatherUi::icon(day.condition, x + iconOffsetX, cardsY + iconOffsetY, iconSize, Theme::surface);

    char lo[8], hi[8];
    snprintf(lo, sizeof(lo), "%d", day.lo);
    snprintf(hi, sizeof(hi), "%d", day.hi);
    constexpr int ringSpace = 8;
    int loWidth = Widgets::textWidth(lo, Fonts::body, 1) + ringSpace;
    int hiWidth = Widgets::textWidth(hi, Fonts::body, 1) + ringSpace;
    int start = x + (cardW - (loWidth + rangeGap + hiWidth)) / 2;
    int y = cardsY + rangeOffsetY;

    Widgets::text(lo, start, y, Fonts::body, 1, Theme::textMuted);
    WeatherUi::degree(start + loWidth - 4, y + 5, 2, Theme::textMuted);
    int dividerX = start + loWidth + rangeGap / 2;
    tft.drawFastVLine(dividerX, y + 1, 14, Theme::outline);
    int hiStart = start + loWidth + rangeGap;
    Widgets::text(hi, hiStart, y, Fonts::body, 1, Theme::text);
    WeatherUi::degree(hiStart + hiWidth - 4, y + 5, 2, Theme::text);
}
