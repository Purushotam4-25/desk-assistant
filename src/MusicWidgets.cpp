#include "MusicWidgets.h"
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "MusicPlayer.h"

static_assert(MusicUi::coverSize == musicCoverSide, "cover size must match the pixel buffer");

extern TFT_eSPI tft;

namespace MusicUi {
void dots(const NavState& nav) {
    constexpr int spacing = 14, radius = 3;
    int halfSpan = ((nav.subPageCount - 1) * spacing) / 2 + radius + 1;
    tft.fillRect(160 - halfSpan, dotsY - 4, 2 * halfSpan, 9, Theme::background);
    int x0 = 160 - ((nav.subPageCount - 1) * spacing) / 2;
    for (int i = 0; i < nav.subPageCount; i++) {
        tft.fillCircle(x0 + i * spacing, dotsY, radius, i == nav.subPage ? Theme::accent : Theme::outline);
    }
}

void cover(int x, int y, const uint16_t* pixels) {
    if (pixels) {
        constexpr int radius = 6;
        constexpr uint8_t cornerInset[radius] = {4, 2, 1, 1, 0, 0};
        uint16_t* data = const_cast<uint16_t*>(pixels);
        tft.fillRect(x, y, radius, radius, Theme::background);
        tft.fillRect(x + coverSize - radius, y, radius, radius, Theme::background);
        tft.fillRect(x, y + coverSize - radius, radius, radius, Theme::background);
        tft.fillRect(x + coverSize - radius, y + coverSize - radius, radius, radius, Theme::background);
        for (int row = 0; row < radius; row++) {
            int inset = cornerInset[row];
            int bottom = coverSize - 1 - row;
            tft.pushImage(x + inset, y + row, coverSize - 2 * inset, 1, data + row * coverSize + inset);
            tft.pushImage(x + inset, y + bottom, coverSize - 2 * inset, 1, data + bottom * coverSize + inset);
        }
        tft.pushImage(x, y + radius, coverSize, coverSize - 2 * radius, data + radius * coverSize);
        return;
    }
    tft.fillRoundRect(x, y, coverSize, coverSize, 6, Theme::surface);
    int nx = x + 24, ny = y + 20;
    uint16_t color = Theme::textDim;
    tft.fillCircle(nx + 6, ny + 40, 6, color);
    tft.fillCircle(nx + 32, ny + 34, 6, color);
    tft.fillRect(nx + 10, ny + 6, 3, 34, color);
    tft.fillRect(nx + 36, ny, 3, 34, color);
    tft.fillTriangle(nx + 10, ny + 6, nx + 39, ny, nx + 39, ny + 7, color);
    tft.fillTriangle(nx + 10, ny + 6, nx + 39, ny + 7, nx + 10, ny + 13, color);
}

void pill(int y, const char* label, bool focused) {
    uint16_t color = focused ? Theme::accent : Theme::textMuted;
    tft.fillRect(pillX, y, pillW, pillH, Theme::background);
    tft.drawRoundRect(pillX, y, pillW, pillH, 6, color);
    Widgets::inkTextCentered(label, pillX + pillW / 2, y + 5, Fonts::title, 1, color);
}

void fit(const char* s, int maxInk, char* out, size_t size) {
    if (Widgets::inkWidth(s, Fonts::body, 1) <= maxInk) {
        snprintf(out, size, "%s", s);
        return;
    }
    for (int len = strlen(s); len > 0; len--) {
        if (s[len - 1] == ' ') continue;
        snprintf(out, size, "%.*s..", len, s);
        if (Widgets::inkWidth(out, Fonts::body, 1) <= maxInk) return;
    }
    snprintf(out, size, "..");
}

int bodyY(int capCentre) {
    return capCentre - Fonts::capCentre(Fonts::body, 1, 'N');
}
}
