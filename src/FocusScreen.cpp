#include "FocusScreen.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "TimeFormat.h"

extern TFT_eSPI tft;

namespace {
constexpr int titleX = 20, titleY = 38, rightEdge = 300;
constexpr int goalX = 20, goalY = 70, goalW = 280, goalH = 4;
constexpr int timeCenterY = 112, maxTimeWidth = 304, minColonGap = 16;
constexpr int buttonY = 152, buttonW = 112, buttonH = 34, buttonGap = 12;
constexpr int statsX = 266, statsY = 200, statsW = 44, statsH = 32;
constexpr int itemCount = 3;
}

FocusScreen::FocusScreen(const DeviceStatus& status, FocusSession& session, const NavState& nav, Screen& statsScreen)
    : _status(status), _session(session), _nav(nav), _statsScreen(statsScreen) {
    resetKeys();
}

void FocusScreen::resetKeys() {
    _statusKey = _headerKey = _goalKey = _colonKey = _layoutKey = _actionKey = _endKey = _statsKey = -1;
    _timeKeys[0] = _timeKeys[1] = _timeKeys[2] = -1;
}

void FocusScreen::init() {
    _full = true;
    _focused = Item::Action;
    computeTimeLayout();
}

void FocusScreen::update() {}

Screen* FocusScreen::takeDetail() {
    if (!_openStats) return nullptr;
    _openStats = false;
    return &_statsScreen;
}

bool FocusScreen::available(Item item) const {
    if (item == Item::End) return _session.state() != FocusState::Idle;
    return true;
}

void FocusScreen::moveFocus(int step) {
    int index = (int)_focused;
    for (int tries = 0; tries < itemCount; tries++) {
        index = (index + step + itemCount) % itemCount;
        if (available((Item)index)) {
            _focused = (Item)index;
            return;
        }
    }
}

void FocusScreen::activate() {
    switch (_focused) {
        case Item::Action:
            if (_session.state() == FocusState::Idle) _session.start();
            else if (_session.state() == FocusState::Running) _session.pause();
            else _session.resume();
            break;
        case Item::End:
            _session.end();
            _focused = Item::Action;
            break;
        case Item::Stats:
            _openStats = true;
            break;
    }
}

void FocusScreen::handleInput(Input input) {
    if (input == Input::FocusNext) moveFocus(1);
    if (input == Input::FocusPrev) moveFocus(-1);
    if (input == Input::Select) activate();
}

void FocusScreen::computeTimeLayout() {
    for (_timeScale = 4;; _timeScale--) {
        _groupWidth = 0;
        for (char digit = '0'; digit <= '9'; digit++) {
            char pair[3] = {digit, digit, 0};
            int width = Widgets::textWidth(pair, Fonts::body, _timeScale);
            if (width > _groupWidth) _groupWidth = width;
        }
        _colonGap = _groupWidth / 3 > minColonGap ? _groupWidth / 3 : minColonGap;
        if (3 * _groupWidth + 2 * _colonGap <= maxTimeWidth || _timeScale == 1) break;
    }
    _timeX = (320 - (3 * _groupWidth + 2 * _colonGap)) / 2;

    Fonts::GlyphBox zero = Fonts::glyph(Fonts::body, '0');
    _digitHeight = zero.height * _timeScale;
    _digitTop = timeCenterY - _digitHeight / 2;
    _textY = _digitTop - (Fonts::ascent(Fonts::body) + zero.yOffset) * _timeScale;
}

void FocusScreen::draw() {
    if (_full) {
        tft.fillScreen(Theme::background);
        resetKeys();
        _full = false;
    }
    if (!available(_focused)) _focused = Item::Action;

    drawStatus();
    drawHeader();
    drawGoalBar();
    drawTime();
    drawButtons();
    drawStatsButton();
}

void FocusScreen::drawStatus() {
    int32_t key = _status.wifiConnected | (_status.sdOk << 1);
    if (key == _statusKey) return;
    _statusKey = key;
    Widgets::statusBar(_status, _nav.subPage, _nav.subPageCount);
}

void FocusScreen::drawHeader() {
    FocusState state = _session.state();
    uint32_t minutes = _session.todaySeconds() / 60;
    int32_t key = (int32_t)minutes * 3 + (int)state;
    if (key == _headerKey) return;
    _headerKey = key;

    tft.fillRect(0, 28, 320, 40, Theme::background);
    Widgets::text("Focus", titleX, titleY, Fonts::title, 2, Theme::text);

    int statusX = titleX + Widgets::textWidth("Focus", Fonts::title, 2) + 12;
    if (state == FocusState::Running) Widgets::text("Running", statusX, 52, Fonts::title, 1, Theme::good);
    if (state == FocusState::Paused) Widgets::text("Paused", statusX, 52, Fonts::title, 1, Theme::accent);

    char total[12];
    formatMinutes(total, sizeof(total), minutes);
    int labelWidth = Widgets::textWidth("Today", Fonts::title, 1);
    int totalWidth = Widgets::textWidth(total, Fonts::body, 1);
    Widgets::text("Today", rightEdge - labelWidth, 33, Fonts::title, 1, Theme::textMuted);
    Widgets::text(total, rightEdge - totalWidth, 46, Fonts::body, 1, Theme::text);
}

void FocusScreen::drawGoalBar() {
    uint32_t goal = _session.goalSeconds();
    int percent = goal ? _session.todaySeconds() * 100UL / goal : 0;
    if (percent > 100) percent = 100;
    if (percent == _goalKey) return;
    _goalKey = percent;

    tft.fillRect(goalX, goalY, goalW, goalH, Theme::outline);
    tft.fillRect(goalX, goalY, percent * goalW / 100, goalH, percent >= 100 ? Theme::good : Theme::accent);
}

void FocusScreen::drawTime() {
    FocusState state = _session.state();
    uint32_t seconds = _session.elapsedSeconds();
    uint32_t parts[3] = {(seconds / 3600) % 100, (seconds / 60) % 60, seconds % 60};
    uint16_t color = state == FocusState::Paused ? Theme::textMuted : Theme::text;

    for (int i = 0; i < 3; i++) {
        int32_t key = parts[i] * 3 + (int)state;
        if (key == _timeKeys[i]) continue;
        _timeKeys[i] = key;

        char digits[3];
        snprintf(digits, sizeof(digits), "%02lu", (unsigned long)parts[i]);
        int groupX = _timeX + i * (_groupWidth + _colonGap);
        int width = Widgets::textWidth(digits, Fonts::body, _timeScale);
        tft.fillRect(groupX - 2, _digitTop - 2, _groupWidth + 4, _digitHeight + 4, Theme::background);
        Widgets::text(digits, groupX + (_groupWidth - width) / 2, _textY, Fonts::body, _timeScale, color);
    }
    drawColons(state, seconds);
}

void FocusScreen::drawColons(FocusState state, uint32_t seconds) {
    int32_t key = state == FocusState::Running ? (int32_t)(seconds % 2) : 2 + (int)state;
    if (key == _colonKey) return;
    _colonKey = key;

    uint16_t color = state == FocusState::Paused ? Theme::textMuted : Theme::accent;
    if (state == FocusState::Running && seconds % 2 == 1) color = Theme::background;

    int dot = _digitHeight / 5;
    int middle = _digitTop + _digitHeight / 2;
    int spread = _digitHeight / 4;
    for (int i = 0; i < 2; i++) {
        int centerX = _timeX + i * (_groupWidth + _colonGap) + _groupWidth + _colonGap / 2;
        tft.fillRect(centerX - dot / 2, middle - spread - dot / 2, dot, dot, color);
        tft.fillRect(centerX - dot / 2, middle + spread - dot / 2, dot, dot, color);
    }
}

void FocusScreen::drawButton(int x, const char* label, bool focused) {
    tft.fillRect(x, buttonY, buttonW, buttonH, Theme::background);
    if (focused) {
        tft.fillRoundRect(x, buttonY, buttonW, buttonH, 8, Theme::accent);
    } else {
        tft.drawRoundRect(x, buttonY, buttonW, buttonH, 8, Theme::outline);
        tft.drawRoundRect(x + 1, buttonY + 1, buttonW - 2, buttonH - 2, 7, Theme::outline);
    }
    int width = Widgets::textWidth(label, Fonts::title, 2);
    Widgets::text(label, x + (buttonW - width) / 2, buttonY + 4, Fonts::title, 2,
                  focused ? Theme::background : Theme::text);
}

void FocusScreen::drawButtons() {
    FocusState state = _session.state();
    bool idle = state == FocusState::Idle;

    int32_t layout = idle;
    if (layout != _layoutKey) {
        _layoutKey = layout;
        tft.fillRect(0, buttonY - 4, 320, buttonH + 8, Theme::background);
        _actionKey = _endKey = -1;
    }

    int actionX = idle ? (320 - buttonW) / 2 : (320 - 2 * buttonW - buttonGap) / 2;
    int endX = actionX + buttonW + buttonGap;

    int32_t actionKey = (int)state * 2 + (_focused == Item::Action);
    if (actionKey != _actionKey) {
        _actionKey = actionKey;
        const char* label = idle ? "Start" : state == FocusState::Running ? "Pause" : "Resume";
        drawButton(actionX, label, _focused == Item::Action);
    }

    if (!idle) {
        int32_t endKey = _focused == Item::End;
        if (endKey != _endKey) {
            _endKey = endKey;
            drawButton(endX, "End", endKey);
        }
    }
}

void FocusScreen::drawStatsButton() {
    int32_t key = _focused == Item::Stats;
    if (key == _statsKey) return;
    _statsKey = key;

    tft.fillRect(statsX, statsY, statsW, statsH, Theme::background);
    uint16_t barColor = key ? Theme::background : Theme::text;
    if (key) {
        tft.fillRoundRect(statsX, statsY, statsW, statsH, 8, Theme::accent);
    } else {
        tft.drawRoundRect(statsX, statsY, statsW, statsH, 8, Theme::outline);
        tft.drawRoundRect(statsX + 1, statsY + 1, statsW - 2, statsH - 2, 7, Theme::outline);
    }
    tft.fillRect(statsX + 10, statsY + 18, 6, 8, barColor);
    tft.fillRect(statsX + 19, statsY + 12, 6, 14, barColor);
    tft.fillRect(statsX + 28, statsY + 6, 6, 20, barColor);
}
