#include "WeatherWidgets.h"
#include <TFT_eSPI.h>
#include "Theme.h"

extern TFT_eSPI tft;

namespace {
constexpr uint8_t allRays = 0xFF;
constexpr uint8_t partlyRays = 0xF9;
constexpr uint8_t showersRays = 0xF8;

struct Shape {
    int x, y, size;
    uint16_t background;

    int at(int hundredths) const { return (hundredths * size + 5000) / 10000; }
    int px(int v) const { return at(v * 100); }

    void disc(int cx, int cy, int r, uint16_t color) const {
        tft.fillCircle(x + cx, y + cy, r < 1 ? 1 : r, color);
    }

    void circle(int cx, int cy, int r, uint16_t color) const { disc(px(cx), px(cy), px(r), color); }

    void rect(int rx, int ry, int w, int h, uint16_t color) const {
        tft.fillRect(x + px(rx), y + px(ry), px(rx + w) - px(rx), px(ry + h) - px(ry), color);
    }

    void triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) const {
        tft.fillTriangle(x + px(x0), y + px(y0), x + px(x1), y + px(y1), x + px(x2), y + px(y2), color);
    }

    void sun(int cx, int cy, int r, int rayDistance, int rayRadius, uint8_t rays) const {
        circle(cx, cy, r, Theme::sun);
        int diagonal = (rayDistance * 707 + 500) / 1000;
        const int8_t steps[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
        for (int i = 0; i < 8; i++) {
            if (!(rays & (1 << i))) continue;
            int distance = (steps[i][0] != 0 && steps[i][1] != 0) ? diagonal : rayDistance;
            circle(cx + steps[i][0] * distance, cy + steps[i][1] * distance, rayRadius, Theme::sun);
        }
    }

    // Scale is in percent. The halo and crescent cutouts are drawn in the background colour.
    void cloud(int ox, int oy, int scale, uint16_t color, int grow = 0) const {
        int g = grow * 100;
        int x0 = ox * 100 + 20 * scale;
        int y0 = oy * 100 + 58 * scale;
        int endY = at(y0);
        int endR = at(18 * scale + g);
        disc(at(x0), endY, endR, color);
        disc(at(ox * 100 + 45 * scale), at(oy * 100 + 45 * scale), at(25 * scale + g), color);
        disc(at(ox * 100 + 78 * scale), endY, endR, color);

        int left = at(x0 - g);
        int top = at(y0 - g);
        int right = at(ox * 100 + 78 * scale + g);
        tft.fillRect(x + left, y + top, right - left, endY + (endR < 1 ? 1 : endR) + 1 - top, color);
    }

    void drop(int cx, int cy) const {
        triangle(cx, cy - 16, cx - 6, cy - 1, cx + 6, cy - 1, Theme::rain);
        circle(cx, cy, 6, Theme::rain);
    }

    void sparkle(int cx, int cy, int length, int widthTenths) const {
        int sx = at(cx * 100), sy = at(cy * 100);
        int l = at(length * 100);
        int w = at(widthTenths * 10);
        if (w < 1) w = 1;
        uint16_t c = Theme::cloud;
        tft.fillTriangle(x + sx, y + sy - l, x + sx - w, y + sy, x + sx + w, y + sy, c);
        tft.fillTriangle(x + sx, y + sy + l, x + sx - w, y + sy, x + sx + w, y + sy, c);
        tft.fillTriangle(x + sx - l, y + sy, x + sx, y + sy - w, x + sx, y + sy + w, c);
        tft.fillTriangle(x + sx + l, y + sy, x + sx, y + sy - w, x + sx, y + sy + w, c);
    }

    void crescent(int cx, int cy, int r, int cutX, int cutY, int cutR) const {
        circle(cx, cy, r, Theme::moon);
        circle(cutX, cutY, cutR, background);
    }

    void bolt() const {
        const uint16_t c = Theme::sun;
        triangle(33, 74, 47, 48, 65, 48, c);
        triangle(33, 74, 65, 48, 55, 64, c);
        triangle(33, 74, 55, 64, 67, 64, c);
        triangle(67, 64, 39, 96, 47, 74, c);
        triangle(67, 64, 47, 74, 33, 74, c);
    }
};
}

namespace {
// Painted area of each icon on the 100 grid. Every icon is scaled so this area is about the same
// height, and centred in the icon box, so the set looks even next to the text.
struct Ink {
    int8_t left, top, right, bottom;
};

constexpr int targetHeight = 92, maxWidth = 112, maxGrowPercent = 125;

Ink inkOf(WeatherCondition condition, bool isDay) {
    switch (condition) {
        case WeatherCondition::Clear: return isDay ? Ink{7, 7, 93, 93} : Ink{14, 19, 90, 84};
        case WeatherCondition::MostlyClear: return isDay ? Ink{5, 5, 99, 90} : Ink{12, 11, 99, 90};
        case WeatherCondition::Cloudy: return {4, 14, 97, 83};
        case WeatherCondition::Fog: return {10, 12, 89, 95};
        case WeatherCondition::Rain:
        case WeatherCondition::Storm: return {10, 12, 89, 96};
        case WeatherCondition::Showers: return {5, 3, 95, 97};
    }
    return {0, 0, 99, 99};
}

int fitPercent(const Ink& ink) {
    int byHeight = targetHeight * 100 / (ink.bottom - ink.top + 1);
    int byWidth = maxWidth * 100 / (ink.right - ink.left + 1);
    int grow = byHeight < byWidth ? byHeight : byWidth;
    return grow < maxGrowPercent ? grow : maxGrowPercent;
}
}

namespace WeatherUi {
const char* const weekdayShort[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void icon(WeatherCondition condition, bool isDay, int x, int y, int size, uint16_t background) {
    Ink ink = inkOf(condition, isDay);
    int grow = fitPercent(ink);
    int fitted = size * grow / 100;
    int originX = x + size / 2 - ((ink.left + ink.right + 1) * 50 * fitted + 5000) / 10000;
    int originY = y + size / 2 - ((ink.top + ink.bottom + 1) * 50 * fitted + 5000) / 10000;
    Shape i = {originX, originY, fitted, background};
    constexpr int medium = 85, small = 72;
    switch (condition) {
        case WeatherCondition::Clear:
            if (isDay) {
                i.sun(50, 50, 22, 38, 5, allRays);
            } else {
                i.crescent(44, 54, 30, 57, 45, 26);
                i.sparkle(71, 30, 9, 30);
                i.circle(87, 56, 3, Theme::cloud);
            }
            break;
        case WeatherCondition::MostlyClear:
            if (isDay) {
                i.sun(36, 36, 16, 28, 3, partlyRays);
            } else {
                i.crescent(34, 36, 22, 43, 30, 19);
                i.sparkle(76, 22, 7, 25);
            }
            i.cloud(27, 32, small, background, 3);
            i.cloud(27, 32, small, Theme::cloud);
            break;
        case WeatherCondition::Cloudy:
            i.cloud(28, 0, small, Theme::cloudDark);
            i.cloud(2, 19, medium, Theme::cloud);
            break;
        case WeatherCondition::Fog:
            i.cloud(8, -5, medium, Theme::cloud);
            i.rect(12, 66, 64, 6, Theme::cloud);
            i.rect(28, 78, 62, 6, Theme::cloud);
            i.rect(16, 90, 52, 6, Theme::cloud);
            break;
        case WeatherCondition::Rain:
            i.cloud(8, -5, medium, Theme::cloudDark);
            i.drop(27, 79);
            i.drop(50, 90);
            i.drop(73, 79);
            break;
        case WeatherCondition::Showers:
            i.sun(32, 30, 14, 24, 3, showersRays);
            i.cloud(23, 16, small, background, 3);
            i.cloud(23, 16, small, Theme::cloudDark);
            i.drop(47, 91);
            i.drop(70, 91);
            break;
        case WeatherCondition::Storm:
            i.cloud(8, -5, medium, Theme::cloudDark);
            i.bolt();
            break;
    }
}

int iconWidth(WeatherCondition condition, bool isDay, int size) {
    Ink ink = inkOf(condition, isDay);
    return (ink.right - ink.left + 1) * (size * fitPercent(ink) / 100) / 100;
}

void degree(int x, int y, int radius, uint16_t color) {
    int rings = radius >= 5 ? 3 : 2;
    for (int k = 0; k < rings; k++) {
        tft.drawCircle(x, y, radius - k, color);
    }
}

int yForDigitTop(const GFXfont* font, uint8_t scale, int digitTop) {
    return digitTop - (Fonts::ascent(font) + Fonts::glyph(font, '0').yOffset) * scale;
}

int digitHeight(const GFXfont* font, uint8_t scale) {
    return Fonts::glyph(font, '0').height * scale;
}

uint16_t uvColor(UvLevel level) {
    switch (level) {
        case UvLevel::Low: return Theme::uvLow;
        case UvLevel::Moderate: return Theme::uvModerate;
        case UvLevel::High: return Theme::uvHigh;
        default: return Theme::uvExtreme;
    }
}
}
