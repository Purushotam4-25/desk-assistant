#include "WeatherWidgets.h"
#include <TFT_eSPI.h>
#include "Theme.h"

extern TFT_eSPI tft;

namespace {
struct Circle {
    int8_t x, y, r;
};

struct CloudShape {
    Circle left, mid, right;
    int8_t baseX, baseY, baseW, baseH;
};

const CloudShape cloudy = {{20, 58, 18}, {45, 45, 25}, {78, 58, 18}, 20, 58, 58, 17};
const CloudShape partly = {{34, 68, 14}, {54, 58, 20}, {80, 68, 14}, 34, 68, 46, 14};
const CloudShape rainy = {{24, 45, 15}, {46, 34, 22}, {74, 45, 15}, 24, 45, 50, 15};

struct Shape {
    int x, y, size;

    int px(int v) const { return (v * size + 50) / 100; }

    void circle(int cx, int cy, int r, uint16_t color) const {
        tft.fillCircle(x + px(cx), y + px(cy), px(r), color);
    }

    void rect(int rx, int ry, int w, int h, uint16_t color) const {
        tft.fillRect(x + px(rx), y + px(ry), px(rx + w) - px(rx), px(ry + h) - px(ry), color);
    }

    void line(int x0, int y0, int x1, int y1, uint16_t color) const {
        tft.drawLine(x + px(x0), y + px(y0), x + px(x1), y + px(y1), color);
    }

    void sun(int cx, int cy, int r, int rayDistance, int rayRadius) const {
        circle(cx, cy, r, Theme::sun);
        int diagonal = (rayDistance * 707 + 500) / 1000;
        const int8_t steps[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
        for (const auto& s : steps) {
            int distance = (s[0] != 0 && s[1] != 0) ? diagonal : rayDistance;
            circle(cx + s[0] * distance, cy + s[1] * distance, rayRadius, Theme::sun);
        }
    }

    void cloud(const CloudShape& c, uint16_t color, int grow) const {
        circle(c.left.x, c.left.y, c.left.r + grow, color);
        circle(c.mid.x, c.mid.y, c.mid.r + grow, color);
        circle(c.right.x, c.right.y, c.right.r + grow, color);
        rect(c.baseX, c.baseY - grow, c.baseW, c.baseH + 2 * grow, color);
    }

    void drops() const {
        for (int dropX : {32, 50, 68}) {
            line(dropX, 70, dropX - 5, 88, Theme::rain);
            line(dropX + 1, 70, dropX - 4, 88, Theme::rain);
        }
    }
};
}

namespace WeatherUi {
const char* const weekdayShort[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void icon(WeatherCondition condition, int x, int y, int size, uint16_t background) {
    Shape i = {x, y, size};
    switch (condition) {
        case WeatherCondition::Clear:
            i.sun(50, 50, 20, 35, 4);
            break;
        case WeatherCondition::MostlyClear:
            i.sun(40, 40, 17, 29, 3);
            i.cloud(partly, background, 3);
            i.cloud(partly, Theme::cloud, 0);
            break;
        case WeatherCondition::Cloudy:
            i.cloud(cloudy, Theme::cloud, 0);
            break;
        case WeatherCondition::Rain:
            i.cloud(rainy, Theme::cloudDark, 0);
            i.drops();
            break;
    }
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
