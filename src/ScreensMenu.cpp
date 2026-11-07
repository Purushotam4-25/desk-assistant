#include "ScreensMenu.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"

extern TFT_eSPI tft;

namespace {
constexpr int rowX = 10, rowY = 16, rowW = 284, rowH = 64, rowStep = 72, labelMaxWidth = 200;
constexpr int barX = 306, barY = 16, barH = 208;
}

ScreensMenu::ScreensMenu(const Page* pages, uint8_t pageCount) : _pages(pages) {
    _focus.count = pageCount;
    _focus.visible = visibleRows;
    resetKeys();
}

void ScreensMenu::resetKeys() {
    for (int i = 0; i < visibleRows; i++) _rowKeys[i] = -1;
    _barKey = -1;
}

void ScreensMenu::open(uint8_t currentPage) {
    _focus.set(currentPage);
    init();
}

uint8_t ScreensMenu::focusedPage() const {
    return _focus.index;
}

void ScreensMenu::init() {
    _full = true;
}

void ScreensMenu::update() {}

void ScreensMenu::handleInput(Input input) {
    if (input == Input::FocusNext) _focus.move(1);
    if (input == Input::FocusPrev) _focus.move(-1);
}

void ScreensMenu::draw() {
    if (_full) {
        tft.fillScreen(Theme::background);
        resetKeys();
        _full = false;
    }
    drawRows();
    drawScrollBar();
}

void ScreensMenu::drawRows() {
    for (int slot = 0; slot < visibleRows; slot++) {
        int page = _focus.first + slot;
        bool exists = page < _focus.count;
        bool selected = exists && page == _focus.index;
        int16_t key = exists ? page * 2 + selected : -2;
        if (key == _rowKeys[slot]) continue;
        _rowKeys[slot] = key;
        drawRow(slot, page, selected);
    }
}

void ScreensMenu::drawRow(int slot, int page, bool selected) {
    int y = rowY + slot * rowStep;
    tft.fillRect(rowX, y, rowW, rowH, Theme::background);
    if (page >= _focus.count) return;

    const Page& entry = _pages[page];
    if (selected) {
        tft.fillRoundRect(rowX, y, rowW, rowH, 8, Theme::highlight);
        tft.drawRoundRect(rowX, y, rowW, rowH, 8, Theme::accent);
        tft.drawRoundRect(rowX + 1, y + 1, rowW - 2, rowH - 2, 7, Theme::accent);
        Icons::draw(entry.icon, rowX, y, Theme::accent, Theme::highlight);
        bool fitsLarge = Widgets::textWidth(entry.name, Fonts::body, 2) <= labelMaxWidth;
        Widgets::fitText(entry.name, rowX + 64, y + (fitsLarge ? 15 : 23), labelMaxWidth, Fonts::body, fitsLarge ? 2 : 1,
                         Theme::text);
    } else {
        Icons::draw(entry.icon, rowX, y, Theme::textMuted, Theme::background);
        Widgets::fitText(entry.name, rowX + 64, y + 23, labelMaxWidth, Fonts::body, 1, Theme::textMuted);
    }
}

void ScreensMenu::drawScrollBar() {
    if (_focus.count <= visibleRows) return;
    if (_focus.index == _barKey) return;
    _barKey = _focus.index;
    Widgets::scrollBar(barX, barY, barH, _focus.index, _focus.count);
}
