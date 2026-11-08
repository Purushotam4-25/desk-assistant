#include "TodayScreen.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "ClockFormat.h"

extern TFT_eSPI tft;

namespace {
constexpr int visibleRows = 5;
constexpr int rowX = 10, rowY = 62, rowW = 300, rowH = 30, rowStep = 34;
}

TodayScreen::TodayScreen(const DeviceStatus& status, const Agenda& agenda, const NavState& nav)
    : _status(status), _agenda(agenda), _nav(nav), _statusKey(-1), _headerKey(-1), _rowsKey(-1) {}

void TodayScreen::init() {
    _full = true;
}

void TodayScreen::update() {}

void TodayScreen::handleInput(Input input) {}

void TodayScreen::draw() {
    if (_full) {
        tft.fillScreen(Theme::background);
        _statusKey = _headerKey = _rowsKey = -1;
        _full = false;
    }
    drawStatus();
    drawHeader();
    drawRows();
}

int TodayScreen::firstVisibleRow() const {
    int count = _agenda.count();
    int lastStart = count > visibleRows ? count - visibleRows : 0;
    int active = _agenda.firstActiveOrUpcoming(_status.minuteOfDay());
    int start = active < 0 ? lastStart : active - 1;
    if (start < 0) start = 0;
    if (start > lastStart) start = lastStart;
    return start;
}

void TodayScreen::drawStatus() {
    int32_t key = _status.wifiConnected | (_status.sdOk << 1);
    if (key == _statusKey) return;
    _statusKey = key;
    Widgets::statusBar(_status, _nav.subPage, _nav.subPageCount);
}

void TodayScreen::drawHeader() {
    int32_t key = _agenda.remaining(_status.minuteOfDay());
    if (key == _headerKey) return;
    _headerKey = key;

    char info[16];
    snprintf(info, sizeof(info), "%ld left", (long)key);
    int infoWidth = Widgets::textWidth(info, Fonts::body, 1);

    tft.fillRect(0, 28, 320, 32, Theme::background);
    Widgets::text("Today", 11, 30, Fonts::title, 2, Theme::text);
    Widgets::text(info, 310 - infoWidth, 38, Fonts::body, 1, Theme::textMuted);
}

void TodayScreen::drawRows() {
    uint16_t now = _status.minuteOfDay();
    int start = firstVisibleRow();
    int shown = _agenda.count() - start;
    if (shown > visibleRows) shown = visibleRows;

    int32_t key = start;
    for (int i = 0; i < shown; i++) {
        key = key * 3 + (int)_agenda.stateOf(start + i, now);
    }
    if (key == _rowsKey) return;
    _rowsKey = key;

    tft.fillRect(0, rowY - 2, 320, 240 - (rowY - 2), Theme::background);
    for (int i = 0; i < shown; i++) {
        drawRow(i, _agenda.at(start + i), _agenda.stateOf(start + i, now));
    }
}

void TodayScreen::drawRow(int slot, const AgendaEvent& event, EventState state) {
    int y = rowY + slot * rowStep;
    char time[10];
    ClockFormat::timeCompact(time, sizeof(time), event.startMinute, _status.use24Hour);
    int titleX = rowX + (_status.use24Hour ? 84 : 94);

    uint16_t timeColor, titleColor;
    switch (state) {
        case EventState::Now:
            tft.fillRoundRect(rowX, y, rowW, rowH, 4, Theme::accent);
            timeColor = titleColor = Theme::background;
            break;
        case EventState::Done:
            tft.drawRoundRect(rowX, y, rowW, rowH, 4, Theme::surface);
            timeColor = titleColor = Theme::textDim;
            break;
        default:
            tft.drawRoundRect(rowX, y, rowW, rowH, 4, Theme::outline);
            timeColor = Theme::accent;
            titleColor = Theme::text;
            break;
    }

    Widgets::text(time, rowX + 10, y + 6, Fonts::body, 1, timeColor);
    Widgets::fitText(event.title, titleX, y + 6, 264 - (titleX - rowX), Fonts::body, 1, titleColor);

    if (state == EventState::Done) {
        tft.drawLine(rowX + 282, y + 15, rowX + 286, y + 19, Theme::good);
        tft.drawLine(rowX + 286, y + 19, rowX + 293, y + 10, Theme::good);
    }
}
