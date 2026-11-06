#include "Widgets.h"
#include "Theme.h"
#include "Fonts.h"

extern TFT_eSPI tft;

namespace {
struct Rect {
    int8_t dx, dy, w, h;
};

const Rect wifiArcs[] = {
    {6, 0, 9, 1}, {4, 1, 13, 1}, {2, 2, 17, 1}, {1, 3, 5, 1}, {15, 3, 5, 1}, {2, 4, 3, 1},
    {16, 4, 3, 1}, {7, 5, 7, 1}, {5, 6, 11, 2}, {6, 8, 2, 1}, {13, 8, 2, 1},
};

void drawWifi(int x, int y, bool connected) {
    uint16_t color = connected ? Theme::text : Theme::textDim;
    for (const Rect& r : wifiArcs) {
        tft.fillRect(x + r.dx, y + r.dy, r.w, r.h, color);
    }
    tft.fillCircle(x + 10, y + 12, 2, color);
}

void drawSdCard(int x, int y, bool ok) {
    uint16_t color = ok ? Theme::text : Theme::textDim;
    tft.fillRect(x + 6, y, 13, 22, color);
    tft.fillRect(x, y + 6, 6, 16, color);
    tft.fillTriangle(x, y + 6, x + 6, y, x + 6, y + 6, color);
    tft.fillRect(x + 7, y + 2, 10, 18, Theme::background);
    tft.fillRect(x + 2, y + 7, 5, 13, Theme::background);
    tft.fillTriangle(x + 2, y + 7, x + 7, y + 2, x + 7, y + 7, Theme::background);
    tft.fillRect(x + 8, y + 3, 2, 5, color);
    tft.fillRect(x + 11, y + 3, 2, 5, color);
    tft.fillRect(x + 14, y + 3, 2, 5, color);
    Widgets::text("SD", x + 4, y + 11, Fonts::tiny, 1, color);
}

void drawPageDots(int centerX, int y, int count, int active) {
    int spacing = 10;
    int x0 = centerX - ((count - 1) * spacing) / 2;
    for (int i = 0; i < count; i++) {
        tft.fillCircle(x0 + i * spacing, y, 2, i == active ? Theme::accent : Theme::outline);
    }
}

void drawUsbPower(int x, int y) {
    tft.drawRect(x + 14, y + 1, 27, 13, Theme::text);
    tft.fillRect(x + 41, y + 5, 2, 5, Theme::text);
    for (int i = 0; i < 4; i++) {
        tft.fillRect(x + 16 + i * 6, y + 3, 5, 9, Theme::good);
    }
    tft.fillRect(x + 2, y, 2, 5, Theme::plug);
    tft.fillRect(x + 6, y, 2, 5, Theme::plug);
    tft.fillRoundRect(x, y + 5, 10, 6, 1, Theme::plug);
    tft.fillRect(x + 4, y + 11, 2, 3, Theme::plug);
}
}

namespace Widgets {
void text(const char* s, int x, int y, const GFXfont* font, uint8_t size, uint16_t color) {
    tft.setFreeFont(font);
    tft.setTextSize(size);
    tft.setTextColor(color);
    tft.drawString(s, x, y);
}

int textWidth(const char* s, const GFXfont* font, uint8_t size) {
    tft.setFreeFont(font);
    tft.setTextSize(size);
    return tft.textWidth(s);
}

Ink ink(const char* s, const GFXfont* font, uint8_t size) {
    int pen = 0, left = 0, right = 0;
    bool seen = false;
    for (; *s; s++) {
        unsigned char c = *s;
        if (font ? (c < font->first || c > font->last) : (c < 0x20 || c > 0x7E)) continue;
        Fonts::GlyphRun glyph = font ? Fonts::run(font, c) : Fonts::glcdRun(c);
        if (glyph.width > 0) {
            if (!seen) left = pen + glyph.xOffset;
            seen = true;
            right = pen + glyph.xOffset + glyph.width;
        }
        pen += glyph.advance;
    }
    return {left * size, (right - left) * size};
}

Rows inkRows(const char* s, const GFXfont* font, uint8_t size) {
    if (!font) return {0, 8 * size};
    Fonts::GlyphRows cap = Fonts::rows(font, 'H');
    int top = cap.top, bottom = cap.bottom;
    for (; *s; s++) {
        unsigned char c = *s;
        if (c < font->first || c > font->last) continue;
        Fonts::GlyphRows glyph = Fonts::rows(font, c);
        if (glyph.bottom == glyph.top) continue;
        if (glyph.top < top) top = glyph.top;
        if (glyph.bottom > bottom) bottom = glyph.bottom;
    }
    return {top * size, (bottom - top) * size};
}

int inkWidth(const char* s, const GFXfont* font, uint8_t size) {
    return ink(s, font, size).width;
}

void inkText(const char* s, int inkX, int y, const GFXfont* font, uint8_t size, uint16_t color) {
    text(s, inkX - ink(s, font, size).left, y, font, size, color);
}

void inkTextCentered(const char* s, int centerX, int y, const GFXfont* font, uint8_t size, uint16_t color) {
    inkText(s, centerX - inkWidth(s, font, size) / 2, y, font, size, color);
}

void fitText(const char* s, int x, int y, int maxWidth, const GFXfont* font, uint8_t size, uint16_t color) {
    if (textWidth(s, font, size) <= maxWidth) {
        text(s, x, y, font, size, color);
        return;
    }
    char shortened[40];
    for (int len = strlen(s); len > 0; len--) {
        snprintf(shortened, sizeof(shortened), "%.*s..", len, s);
        if (textWidth(shortened, font, size) <= maxWidth) break;
    }
    text(shortened, x, y, font, size, color);
}

void card(int x, int y, int w, int h) {
    tft.fillRoundRect(x, y, w, h, 6, Theme::surface);
}

void button(int x, int y, int w, int h, const char* label, bool focused) {
    tft.fillRect(x, y, w, h, Theme::background);
    if (focused) {
        tft.fillRoundRect(x, y, w, h, 8, Theme::accent);
    } else {
        tft.drawRoundRect(x, y, w, h, 8, Theme::outline);
        tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 7, Theme::outline);
    }
    Rows rows = inkRows(label, Fonts::title, 2);
    int top = y + (h - rows.height) / 2 - rows.top;
    inkTextCentered(label, x + w / 2, top, Fonts::title, 2, focused ? Theme::background : Theme::text);
}

void statusBar(const DeviceStatus& status, int activePage, int pageCount) {
    tft.fillRect(0, 0, 320, 26, Theme::background);
    tft.drawFastHLine(0, 26, 320, Theme::outline);
    drawWifi(10, 5, status.wifiConnected);
    drawSdCard(44, 2, status.sdOk);
    drawPageDots(160, 13, pageCount, activePage);
    drawUsbPower(267, 6);
}
}
