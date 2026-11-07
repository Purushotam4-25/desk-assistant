#include "QuickMenuScreen.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "ListFocus.h"

extern TFT_eSPI tft;

namespace {
constexpr int gridX = 10, gridY = 45, buttonW = 146, buttonH = 84, gapX = 8, gapY = 8;
constexpr int cornerRadius = 16;
constexpr int axis = 72;
constexpr int slotCentre = 30;
constexpr int captionTop = 58;

const char* const labels[] = {"Wi-Fi", "Night", "Timer", "Brightness"};

int floorSqrt(int value) {
    int root = 0;
    while ((root + 1) * (root + 1) <= value) root++;
    return root;
}

void wifiIcon(int cx, int cy, uint16_t color) {
    constexpr int ringStart[] = {6, 12, 18};
    constexpr int ringWidth = 3;
    for (int dy = 1; dy <= ringStart[2] + ringWidth; dy++) {
        for (int start : ringStart) {
            int outer = start + ringWidth;
            if (dy > outer) continue;
            int reachOut = floorSqrt(outer * outer - dy * dy);
            int innerSq = start * start - dy * dy;
            int reachIn = innerSq > 0 ? floorSqrt(innerSq - 1) + 1 : 0;
            int reachMax = reachOut < dy ? reachOut : dy;
            if (reachIn > reachMax) continue;
            if (reachIn == 0) {
                tft.fillRect(cx - reachMax, cy - dy, 2 * reachMax + 1, 1, color);
            } else {
                tft.fillRect(cx - reachMax, cy - dy, reachMax - reachIn + 1, 1, color);
                tft.fillRect(cx + reachIn, cy - dy, reachMax - reachIn + 1, 1, color);
            }
        }
    }
    tft.fillCircle(cx, cy, 3, color);
}

void moonIcon(int cx, int cy, uint16_t color, uint16_t cut) {
    constexpr int shift = 1;
    tft.fillCircle(cx + shift, cy, 13, color);
    tft.fillCircle(cx + shift + 6, cy - 5, 11, cut);
}

void timerIcon(int cx, int cy, uint16_t color, uint16_t inner) {
    tft.fillRect(cx - 4, cy - 18, 9, 3, color);
    tft.fillRect(cx - 1, cy - 15, 3, 3, color);
    tft.fillCircle(cx, cy, 12, color);
    tft.fillCircle(cx, cy, 10, inner);
    tft.fillRect(cx - 1, cy - 8, 3, 9, color);
    tft.fillCircle(cx, cy, 2, color);
}

void sunIcon(int cx, int cy, uint16_t color, int level) {
    constexpr int start = 9;
    int length = level == 2 ? 5 : 3;
    tft.fillCircle(cx, cy, 5, color);
    tft.fillRect(cx - 1, cy - start - length + 1, 3, length, color);
    tft.fillRect(cx - 1, cy + start, 3, length, color);
    tft.fillRect(cx - start - length + 1, cy - 1, length, 3, color);
    tft.fillRect(cx + start, cy - 1, length, 3, color);
    if (level == 0) return;
    for (int d = 7; d <= (level == 2 ? 9 : 7); d += 2) {
        tft.fillCircle(cx - d, cy - d, 1, color);
        tft.fillCircle(cx + d, cy - d, 1, color);
        tft.fillCircle(cx - d, cy + d, 1, color);
        tft.fillCircle(cx + d, cy + d, 1, color);
    }
}

bool settingOn(const QuickSettings& settings, int item) {
    switch (item) {
        case 0: return settings.wifiOn;
        case 1: return settings.nightOn;
        default: return false;
    }
}

bool iconLit(const QuickSettings& settings, int item) {
    return item == 3 || settingOn(settings, item);
}
}

QuickMenuScreen::QuickMenuScreen(const DeviceStatus& status, const NavState& nav, QuickSettings& settings)
    : _status(status), _nav(nav), _settings(settings) {
    resetKeys();
}

void QuickMenuScreen::resetKeys() {
    _statusKey = -1;
    for (int32_t& key : _buttonKeys) key = -1;
}

void QuickMenuScreen::init() {
    _full = true;
    _focused = Wifi;
}

void QuickMenuScreen::update() {}

bool QuickMenuScreen::takeNavigation(NavTarget& target) {
    if (!_openTimer) return false;
    _openTimer = false;
    target = {"Timer", 0};
    return true;
}

void QuickMenuScreen::activate() {
    switch (_focused) {
        case Wifi: _settings.wifiOn = !_settings.wifiOn; break;
        case Night: _settings.nightOn = !_settings.nightOn; break;
        case Timer: _openTimer = true; break;
        case Brightness: _settings.brightness = (_settings.brightness + 1) % 3; break;
        default: break;
    }
}

void QuickMenuScreen::handleInput(Input input) {
    if (input == Input::FocusNext) _focused = stepIndex(_focused, 1, itemCount);
    if (input == Input::FocusPrev) _focused = stepIndex(_focused, -1, itemCount);
    if (input == Input::Select) activate();
}

int32_t QuickMenuScreen::buttonKey(int item) const {
    int32_t key = item == _focused;
    if (settingOn(_settings, item)) key |= 2;
    if (item == Brightness) key |= _settings.brightness << 2;
    return key;
}

void QuickMenuScreen::draw() {
    if (_full) {
        tft.fillScreen(Theme::background);
        resetKeys();
        _full = false;
    }
    drawStatus();
    for (int item = 0; item < itemCount; item++) drawButton(item);
}

void QuickMenuScreen::drawStatus() {
    int32_t key = _status.wifiConnected | (_status.sdOk << 1);
    if (key == _statusKey) return;
    _statusKey = key;
    Widgets::statusBar(_status, _nav.subPage, _nav.subPageCount);
}

void QuickMenuScreen::drawButton(int item) {
    int32_t key = buttonKey(item);
    if (key == _buttonKeys[item]) return;
    _buttonKeys[item] = key;

    int x = gridX + (item % 2) * (buttonW + gapX);
    int y = gridY + (item / 2) * (buttonH + gapY);
    bool focused = item == _focused;
    bool on = settingOn(_settings, item);
    uint16_t fill = on ? Theme::highlight : Theme::surface;
    uint16_t iconColor = iconLit(_settings, item) ? Theme::accent : Theme::textMuted;

    tft.fillRect(x, y, buttonW, buttonH, Theme::background);
    tft.fillRoundRect(x, y, buttonW, buttonH, cornerRadius, fill);
    if (focused) {
        tft.drawRoundRect(x, y, buttonW, buttonH, cornerRadius, Theme::accent);
        tft.drawRoundRect(x + 1, y + 1, buttonW - 2, buttonH - 2, cornerRadius - 1, Theme::accent);
    } else {
        tft.drawRoundRect(x, y, buttonW, buttonH, cornerRadius, Theme::outline);
    }

    int cx = x + axis;
    int cy = y + slotCentre;
    switch (item) {
        case Wifi: wifiIcon(cx, cy + 9, iconColor); break;
        case Night: moonIcon(cx, cy, iconColor, fill); break;
        case Timer: timerIcon(cx, cy + 3, iconColor, fill); break;
        case Brightness: sunIcon(cx, cy, iconColor, _settings.brightness); break;
        default: break;
    }

    Fonts::GlyphRows cap = Fonts::rows(Fonts::title, 'H');
    Widgets::inkTextCentered(labels[item], cx, y + captionTop - cap.top, Fonts::title, 1,
                             focused ? Theme::text : Theme::textMuted);
}
