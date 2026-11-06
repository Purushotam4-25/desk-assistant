#include "TimerWidgets.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"

extern TFT_eSPI tft;

namespace {
constexpr int readoutScale = 4;
constexpr int underlineGap = 8;

int groupX(const TimerUi::Readout& r, int index) {
    return r.x + index * (r.groupWidth + r.colonGap);
}
}

namespace TimerUi {
Readout makeReadout() {
    Readout r;
    r.scale = readoutScale;
    int inkRight = 0;
    r.inkLeft = 1000;
    for (char digit = '0'; digit <= '9'; digit++) {
        char pair[3] = {digit, digit, 0};
        int width = Widgets::textWidth(pair, Fonts::body, readoutScale);
        if (width > r.groupWidth) r.groupWidth = width;
        Widgets::Ink ink = Widgets::ink(pair, Fonts::body, readoutScale);
        if (ink.left < r.inkLeft) r.inkLeft = ink.left;
        if (ink.left + ink.width > inkRight) inkRight = ink.left + ink.width;
    }
    r.inkWidth = inkRight - r.inkLeft;
    r.colonGap = r.groupWidth / 2;
    r.width = 2 * r.groupWidth + r.colonGap;
    r.x = (320 - r.width) / 2;

    Fonts::GlyphRows rows = Fonts::rows(Fonts::body, '0');
    r.digitHeight = (rows.bottom - rows.top) * readoutScale;
    r.digitTop = readoutCenterY - r.digitHeight / 2;
    r.textY = r.digitTop - rows.top * readoutScale;
    return r;
}

void drawGroup(const Readout& r, int index, unsigned value, uint16_t color) {
    char digits[3];
    snprintf(digits, sizeof(digits), "%02u", value);
    int x = groupX(r, index);
    tft.fillRect(x - 2, r.digitTop - 2, r.groupWidth + 4, r.digitHeight + 4, Theme::background);
    Widgets::text(digits, x, r.textY, Fonts::body, r.scale, color);
}

void drawColon(const Readout& r, uint16_t color) {
    int dot = r.digitHeight / 5;
    int spread = r.digitHeight / 4;
    int middle = r.digitTop + r.digitHeight / 2;
    int centerX = r.x + r.groupWidth + r.colonGap / 2;
    tft.fillRect(centerX - dot / 2, middle - spread - dot / 2, dot, dot, color);
    tft.fillRect(centerX - dot / 2, middle + spread - dot / 2, dot, dot, color);
}

void drawUnderline(const Readout& r, int field, int level) {
    int y = r.digitTop + r.digitHeight + underlineGap;
    tft.fillRect(r.x - 2, y - 2, r.width + 4, 8, Theme::background);
    if (level == 0) return;
    int x = groupX(r, field) + r.inkLeft;
    if (level == 1) tft.fillRect(x, y, r.inkWidth, 2, Theme::textMuted);
    if (level == 2) tft.fillRect(x, y, r.inkWidth, 3, Theme::accent);
}

int buttonX(int count, int index) {
    int total = count * buttonW + (count - 1) * buttonGap;
    return (320 - total) / 2 + index * (buttonW + buttonGap);
}
}
