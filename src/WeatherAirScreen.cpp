#include "WeatherAirScreen.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "WeatherWidgets.h"

extern TFT_eSPI tft;

namespace {
constexpr int panelX = 10, panelY = 20, panelW = 300, panelH = 184;
constexpr int humidityLabelX = panelX + 25, uvLabelX = panelX + 181, centerX = panelX + panelW / 2;
constexpr int labelDigitTop = panelY + 23, valueDigitTop = panelY + 51;
constexpr int dividerY = panelY + 101;
constexpr int bottomLabelDigitTop = panelY + 110, bottomValueDigitTop = panelY + 139;
constexpr int labelScale = 1, valueScale = 2;
constexpr const char* humidityLabel = "Humidity";
constexpr const char* uvLabel = "UV Index";
constexpr const char* rainLabel = "Precipitation";

int labelCenter(const char* label, int x) {
    return x + Widgets::inkWidth(label, Fonts::body, labelScale) / 2;
}

int yFor(int digitTop, int scale) {
    return WeatherUi::yForDigitTop(Fonts::body, scale, digitTop);
}
}

WeatherAirScreen::WeatherAirScreen(const WeatherData& weather, const NavState& nav)
    : SectionScreen(nav), _weather(weather) {}

void WeatherAirScreen::resetKeys() {
    _dataKey = -1;
}

void WeatherAirScreen::drawFrame() {
    tft.fillRoundRect(panelX, panelY, panelW, panelH, 8, Theme::surface);
    tft.drawFastHLine(panelX + 16, dividerY, panelW - 32, Theme::outline);

    int labelY = yFor(labelDigitTop, labelScale);
    Widgets::inkText(humidityLabel, humidityLabelX, labelY, Fonts::body, labelScale, Theme::textMuted);
    Widgets::inkText(uvLabel, uvLabelX, labelY, Fonts::body, labelScale, Theme::textMuted);
    Widgets::inkTextCentered(rainLabel, centerX, yFor(bottomLabelDigitTop, labelScale), Fonts::body, labelScale,
                             Theme::textMuted);
}

void WeatherAirScreen::drawRegions() {
    if (_weather.revision == _dataKey) return;
    _dataKey = _weather.revision;

    int digitHeight = WeatherUi::digitHeight(Fonts::body, valueScale);
    int bandHeight = digitHeight + 8;
    tft.fillRect(panelX + 8, valueDigitTop - 4, panelW - 16, bandHeight, Theme::surface);
    tft.fillRect(panelX + 8, bottomValueDigitTop - 4, panelW - 16, bandHeight, Theme::surface);

    char humidity[8], uv[8], rain[8];
    bool known = _weather.valid;
    bool showUv = known && _weather.isDay;
    if (known) {
        snprintf(humidity, sizeof(humidity), "%u%%", _weather.humidity);
        snprintf(rain, sizeof(rain), "%u%%", _weather.rainPercent);
    } else {
        strlcpy(humidity, "--", sizeof(humidity));
        strlcpy(rain, "--", sizeof(rain));
    }
    if (showUv) {
        snprintf(uv, sizeof(uv), "%u", _weather.uvIndex);
    } else {
        strlcpy(uv, "--", sizeof(uv));
    }
    uint16_t valueColor = known ? Theme::text : Theme::textDim;

    int valueY = yFor(valueDigitTop, valueScale);
    Widgets::inkTextCentered(humidity, labelCenter(humidityLabel, humidityLabelX), valueY, Fonts::body, valueScale,
                             valueColor);
    Widgets::inkTextCentered(uv, labelCenter(uvLabel, uvLabelX), valueY, Fonts::body, valueScale,
                             showUv ? WeatherUi::uvColor(_weather.uvLevel()) : (known ? Theme::textMuted : Theme::textDim));
    Widgets::inkTextCentered(rain, centerX, yFor(bottomValueDigitTop, valueScale), Fonts::body, valueScale,
                             valueColor);
}
