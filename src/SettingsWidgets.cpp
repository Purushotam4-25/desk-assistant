#include "SettingsWidgets.h"
#include <TFT_eSPI.h>
#include <stdio.h>
#include <string.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"

extern TFT_eSPI tft;

namespace {
constexpr int labelX = 10, valueX = 148, infoValueX = 100;
constexpr int labelMaxInk = 130, infoLabelMaxInk = 86;
constexpr int chevronX = 298, normalDotX = 289, infoDotX = 295;
constexpr int editBoxX = 140, editBoxW = 146;
constexpr int barX = 310, barHeight = 192;

int bodyY(int centre) {
    return centre - Fonts::capCentre(Fonts::body, 1, 'N');
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

void bodyText(const char* s, int inkX, int centre, int maxInk, uint16_t color) {
    char shown[40];
    fit(s, maxInk, shown, sizeof(shown));
    Widgets::inkText(shown, inkX, bodyY(centre), Fonts::body, 1, color);
}

void dot(int cx, int cy, Health health) {
    if (health == Health::None) return;
    tft.fillCircle(cx, cy, 8, Theme::background);
    switch (health) {
        case Health::Ok: tft.fillCircle(cx, cy, 5, Theme::good); break;
        case Health::Warn: tft.fillTriangle(cx, cy - 5, cx - 6, cy + 5, cx + 6, cy + 5, Theme::warn); break;
        case Health::Error: tft.fillRect(cx - 4, cy - 4, 9, 9, Theme::alert); break;
        case Health::Off: tft.drawCircle(cx, cy, 5, Theme::textMuted); break;
        default: break;
    }
}

void chevron(int x, int cy, uint16_t color) {
    tft.fillTriangle(x, cy - 6, x, cy + 6, x + 8, cy, color);
}
}

namespace SettingsUi {
void header(const char* title, bool backFocused) {
    tft.fillRect(0, 0, 320, 32, Theme::background);
    tft.drawFastHLine(0, 31, 320, Theme::outline);
    if (backFocused) tft.fillRoundRect(4, 4, 30, 24, 4, Theme::accent);
    tft.fillTriangle(24, 9, 24, 23, 14, 16, backFocused ? Theme::background : Theme::textMuted);

    uint8_t scale = Widgets::inkWidth(title, Fonts::title, 2) <= 260 ? 2 : 1;
    Widgets::inkText(title, 44, 16 - Fonts::capCentre(Fonts::title, scale, 'N'), Fonts::title, scale, Theme::text);
}

void row(int slot, const RowView& view) {
    int y = rowTop + slot * rowHeight;
    int cy = y + rowHeight / 2;
    tft.fillRect(0, y, rowWidth, rowHeight, Theme::background);
    if (view.state == RowState::Empty) return;

    bool bar = view.state == RowState::Selected || view.state == RowState::Editing || view.state == RowState::InfoSelected;
    if (bar) tft.fillRoundRect(0, y, rowWidth, rowHeight, 3, Theme::accent);

    bool info = view.state == RowState::Info || view.state == RowState::InfoSelected;
    uint16_t labelColor = bar ? Theme::background : (info ? Theme::textMuted : Theme::text);
    bodyText(view.label, labelX, cy, info ? infoLabelMaxInk : labelMaxInk, labelColor);

    if (view.state == RowState::Editing) {
        tft.fillRoundRect(editBoxX, y + 2, editBoxW, rowHeight - 4, 3, Theme::background);
        int boxCentre = editBoxX + editBoxW / 2;
        Widgets::inkText("<", editBoxX + 8, bodyY(cy), Fonts::body, 1, Theme::accent);
        int arrowW = Widgets::inkWidth(">", Fonts::body, 1);
        Widgets::inkText(">", editBoxX + editBoxW - 8 - arrowW, bodyY(cy), Fonts::body, 1, Theme::accent);
        char shown[40];
        fit(view.value, editBoxW - 56, shown, sizeof(shown));
        Widgets::inkTextCentered(shown, boxCentre, bodyY(cy), Fonts::body, 1, Theme::text);
        return;
    }

    bool hasDot = view.health != Health::None;
    if (info) {
        uint16_t valueColor = bar ? Theme::background : Theme::text;
        bodyText(view.value, infoValueX, cy, hasDot ? 170 : 190, valueColor);
        dot(infoDotX, cy, view.health);
        return;
    }

    uint16_t valueColor = bar ? Theme::background : Theme::accent;
    bodyText(view.value, valueX, cy, hasDot ? 124 : 144, valueColor);
    if (view.chevron) chevron(chevronX, cy, bar ? Theme::background : Theme::textMuted);
    dot(normalDotX, cy, view.health);
}

void scrollBar(int index, int count) {
    Widgets::scrollBar(barX, rowTop, barHeight, index, count);
}

void footer(const char* hint, const char* page) {
    tft.fillRect(0, footerY, 320, 16, Theme::background);
    int cy = footerY + 8;
    bodyText(hint, labelX, cy, 220, Theme::textMuted);
    int pageW = Widgets::inkWidth(page, Fonts::body, 1);
    Widgets::inkText(page, 310 - pageW, bodyY(cy), Fonts::body, 1, Theme::textMuted);
}

void dialog(const char* title, const char* line1, const char* line2, const char* line3, uint16_t panelColor) {
    tft.fillScreen(Theme::background);
    tft.drawRoundRect(16, 48, 288, 144, 6, panelColor);
    tft.drawRoundRect(17, 49, 286, 142, 5, panelColor);
    Widgets::inkText(title, 32, 70 - Fonts::capCentre(Fonts::title, 2, 'N'), Fonts::title, 2, Theme::text);
    bodyText(line1, 32, 98, 250, Theme::text);
    bodyText(line2, 32, 116, 250, Theme::text);
    bodyText(line3, 32, 134, 250, Theme::textMuted);
}

void dialogButtons(const char* cancelLabel, const char* confirmLabel, bool confirmFocused) {
    const char* labels[2] = {cancelLabel, confirmLabel};
    int xs[2] = {32, 168};
    for (int i = 0; i < 2; i++) {
        bool focused = (i == 1) == confirmFocused;
        tft.fillRect(xs[i], 152, 120, 28, Theme::background);
        if (focused) {
            tft.fillRoundRect(xs[i], 152, 120, 28, 4, Theme::accent);
        } else {
            tft.drawRoundRect(xs[i], 152, 120, 28, 4, Theme::text);
        }
        Widgets::inkTextCentered(labels[i], xs[i] + 60, bodyY(166), Fonts::body, 1,
                                 focused ? Theme::background : Theme::text);
    }
}
}
