#include "CalendarDayScreen.h"
#include <TFT_eSPI.h>
#include <stdio.h>
#include <string.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "DateMath.h"

extern TFT_eSPI tft;

namespace {
constexpr int headerRuleY = 26, headerCentre = 13, sideMargin = 10;
constexpr int labelColumn = 44, labelCentre = 22;
constexpr int gridTop = 38, hourStep = 38, lineCount = 6;
constexpr int areaLeft = 48, areaRight = 310;
constexpr int columnGap = 2, maxColumns = 4;
constexpr int accentBar = 4, textInset = 10, textPadRight = 6;
constexpr int fullMinCard = 34, textMinCard = 17;
constexpr int fullTitleTop = 4, fullTimeTop = 22;
constexpr int nowDotRadius = 3, labelHideRange = 13;

const char* const monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char* const weekdayNames[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

void fitInk(const char* s, int maxInk, const GFXfont* font, char* out, size_t size) {
    if (Widgets::inkWidth(s, font, 1) <= maxInk) {
        snprintf(out, size, "%s", s);
        return;
    }
    for (int len = strlen(s); len > 0; len--) {
        if (s[len - 1] == ' ') continue;
        snprintf(out, size, "%.*s..", len, s);
        if (Widgets::inkWidth(out, font, 1) <= maxInk) return;
    }
    snprintf(out, size, "..");
}

int columnX(int column, int columns) {
    return areaLeft + column * (areaRight - areaLeft + columnGap) / columns;
}
}

CalendarDayScreen::CalendarDayScreen(const DeviceStatus& status, const Agenda& agenda, uint8_t startHour)
    : _status(status),
      _agenda(agenda),
      _startHour(startHour),
      _windowStart(startHour * 60),
      _windowEnd((startHour + CalendarDay::windowHours) * 60) {}

uint8_t CalendarDayScreen::windowFor(uint16_t minuteOfDay) {
    return minuteOfDay >= CalendarDay::afternoonStartHour * 60 &&
                   minuteOfDay < (CalendarDay::afternoonStartHour + CalendarDay::windowHours) * 60
               ? 1
               : 0;
}

void CalendarDayScreen::init() {
    _full = true;
}

void CalendarDayScreen::update() {}

void CalendarDayScreen::handleInput(Input input) {}

int CalendarDayScreen::yOfMinute(uint16_t minute) const {
    return gridTop + ((int)(minute - _windowStart) * hourStep + 30) / 60;
}

bool CalendarDayScreen::inWindow(uint16_t minute) const {
    return minute >= _windowStart && minute < _windowEnd;
}

bool CalendarDayScreen::overlapsPaint(int x0, int y0, int x1, int y1) const {
    return x0 < _paintX + _paintW && x1 > _paintX && y0 < _paintY + _paintH && y1 > _paintY;
}

uint8_t CalendarDayScreen::hiddenMaskFor(int nowY) const {
    uint8_t mask = 0;
    if (nowY < 0) return mask;
    for (int i = 0; i < lineCount; i++) {
        int d = gridTop + i * hourStep - nowY;
        if (d <= labelHideRange && d >= -labelHideRange) mask |= 1 << i;
    }
    return mask;
}

void CalendarDayScreen::buildLayout(uint16_t now) {
    _blockCount = 0;
    size_t count = _agenda.count() < maxBlocks ? _agenda.count() : maxBlocks;
    for (size_t i = 0; i < count; i++) {
        const AgendaEvent& event = _agenda.at(i);
        uint16_t start = event.startMinute > _windowStart ? event.startMinute : _windowStart;
        uint16_t end = event.endMinute < _windowEnd ? event.endMinute : _windowEnd;
        if (end <= start) continue;

        Block block = {};
        block.event = i;
        block.start = start;
        block.end = end;
        block.top = yOfMinute(start);
        block.bottom = yOfMinute(end);
        block.state = _agenda.stateOf(i, now);

        int at = _blockCount++;
        while (at > 0 && (_blocks[at - 1].start > start || (_blocks[at - 1].start == start && _blocks[at - 1].end > end))) {
            _blocks[at] = _blocks[at - 1];
            at--;
        }
        _blocks[at] = block;
    }

    uint16_t columnEnd[maxColumns] = {};
    uint8_t used = 0, clusterFirst = 0;
    uint16_t clusterEnd = 0;
    auto closeCluster = [&](uint8_t from, uint8_t to) {
        for (uint8_t k = from; k < to; k++) {
            Block& b = _blocks[k];
            b.columns = used;
            b.x = columnX(b.column, used);
            int right = b.column + 1 < used ? columnX(b.column + 1, used) - columnGap : areaRight;
            b.width = right - b.x;
        }
    };

    for (uint8_t i = 0; i < _blockCount; i++) {
        Block& b = _blocks[i];
        if (i > 0 && b.start >= clusterEnd) {
            closeCluster(clusterFirst, i);
            memset(columnEnd, 0, sizeof(columnEnd));
            used = 0;
            clusterFirst = i;
            clusterEnd = 0;
        }
        uint8_t column = 0;
        while (column < used && columnEnd[column] > b.start) column++;
        if (column >= maxColumns) column = maxColumns - 1;
        if (column == used) used++;
        columnEnd[column] = b.end > columnEnd[column] ? b.end : columnEnd[column];
        b.column = column;
        if (b.end > clusterEnd) clusterEnd = b.end;
    }
    closeCluster(clusterFirst, _blockCount);
}

void CalendarDayScreen::setNow(uint16_t now) {
    if (inWindow(now)) {
        _nowKey = now;
        _nowY = yOfMinute(now);
    } else {
        _nowKey = -1;
        _nowY = -1;
    }
    _hiddenLabels = hiddenMaskFor(_nowY);
}

void CalendarDayScreen::draw() {
    uint16_t now = _status.minuteOfDay();
    if (_full) {
        fullRedraw(now);
        return;
    }
    drawHeader();
    updateLive(now);
}

void CalendarDayScreen::fullRedraw(uint16_t now) {
    tft.fillScreen(Theme::background);
    _headerKey = -1;
    buildLayout(now);
    setNow(now);
    drawHeader();
    paintRect(0, headerRuleY + 1, 320, 240 - headerRuleY - 1);
    _full = false;
}

void CalendarDayScreen::updateLive(uint16_t now) {
    int32_t nowKey = inWindow(now) ? now : -1;
    if (nowKey != _nowKey) {
        int oldY = _nowY;
        uint8_t oldHidden = _hiddenLabels;
        setNow(now);

        if (oldY != _nowY) {
            if (oldY >= 0) paintLineBand(oldY);
            if (_nowY >= 0) paintLineBand(_nowY);
        }
        if (oldY >= 0) paintLabelBand(oldY);
        if (_nowY >= 0) paintLabelBand(_nowY);

        uint8_t changed = oldHidden ^ _hiddenLabels;
        for (int i = 0; i < lineCount; i++) {
            if (changed & (1 << i)) paintLabelBand(gridTop + i * hourStep);
        }
    }

    for (uint8_t i = 0; i < _blockCount; i++) {
        Block& b = _blocks[i];
        EventState state = _agenda.stateOf(b.event, now);
        if (state == b.state) continue;
        b.state = state;
        paintRect(b.x, b.top, b.width, b.bottom - b.top);
    }
}

void CalendarDayScreen::paintLineBand(int y) {
    paintRect(0, y - 4, 320, 9);
}

void CalendarDayScreen::paintLabelBand(int y) {
    paintRect(0, y - 7, labelColumn, 15);
}

void CalendarDayScreen::drawHeader() {
    const DateTime& t = _status.now;
    int32_t key = t.year * 10000 + t.month * 100 + t.day;
    if (key == _headerKey) return;
    _headerKey = key;

    int32_t days = Date::daysFromCivil(t.year, t.month, t.day);
    const char* month = monthNames[(t.month + 11) % 12];
    char week[12], day[12];
    snprintf(week, sizeof(week), "Week %d", Date::isoWeek(t.year, t.month, t.day));
    snprintf(day, sizeof(day), "%s %02u", weekdayNames[Date::mondayIndex(days)], t.day);

    int bodyY = headerCentre - Fonts::capCentre(Fonts::body, 1, 'N');
    int titleY = headerCentre - Fonts::capCentre(Fonts::title, 1, 'N');
    tft.fillRect(0, 0, 320, headerRuleY, Theme::background);
    Widgets::inkText(month, sideMargin, bodyY, Fonts::body, 1, Theme::text);
    Widgets::inkTextCentered(week, 160, titleY, Fonts::title, 1, Theme::textMuted);
    Widgets::inkText(day, 320 - sideMargin - Widgets::inkWidth(day, Fonts::body, 1), bodyY, Fonts::body, 1,
                     Theme::accent);
    tft.drawFastHLine(0, headerRuleY, 320, Theme::outline);
}

void CalendarDayScreen::paintRect(int x, int y, int w, int h) {
    _paintX = x;
    _paintY = y;
    _paintW = w;
    _paintH = h;

    tft.setViewport(x, y, w, h, false);
    tft.fillRect(x, y, w, h, Theme::background);
    drawGrid();
    for (uint8_t i = 0; i < _blockCount; i++) {
        const Block& b = _blocks[i];
        if (overlapsPaint(b.x, b.top, b.x + b.width, b.bottom)) drawBlock(b);
    }
    drawNowLine(_status.minuteOfDay());
    tft.resetViewport();
}

void CalendarDayScreen::drawGrid() {
    tft.drawFastVLine(labelColumn, headerRuleY + 1, 240 - (headerRuleY + 1), Theme::gridLine);
    int labelOffset = Fonts::capCentre(Fonts::title, 1, 'N');
    for (int i = 0; i < lineCount; i++) {
        int y = gridTop + i * hourStep;
        if (overlapsPaint(labelColumn + 1, y, 320, y + 1)) {
            tft.drawFastHLine(labelColumn + 1, y, 320 - labelColumn - 1, Theme::gridLine);
        }
        if (!(_hiddenLabels & (1 << i)) && overlapsPaint(0, y - 7, labelColumn, y + 8)) {
            char label[4];
            snprintf(label, sizeof(label), "%02u", (unsigned)(_startHour + i));
            Widgets::inkTextCentered(label, labelCentre, y - labelOffset, Fonts::title, 1, Theme::textMuted);
        }
    }
}

void CalendarDayScreen::drawBlock(const Block& b) {
    uint16_t fill, bar, titleColor, timeColor;
    switch (b.state) {
        case EventState::Now:
            fill = Theme::eventNow;
            bar = Theme::accent;
            titleColor = Theme::text;
            timeColor = Theme::textMuted;
            break;
        case EventState::Done:
            fill = Theme::surface;
            bar = Theme::outline;
            titleColor = Theme::textMuted;
            timeColor = Theme::textDim;
            break;
        default:
            fill = Theme::highlight;
            bar = Theme::accent;
            titleColor = Theme::text;
            timeColor = Theme::textMuted;
            break;
    }

    int cardTop = b.top + 1;
    int cardH = b.bottom - b.top - 1;
    if (cardH < 1) cardH = 1;
    tft.fillRect(b.x, cardTop, b.width, cardH, fill);
    tft.fillRect(b.x, cardTop, accentBar, cardH, bar);
    if (cardH < textMinCard) return;

    const AgendaEvent& event = _agenda.at(b.event);
    int textX = b.x + textInset;
    int maxInk = b.width - textInset - textPadRight;
    char title[40];
    fitInk(event.title, maxInk, Fonts::body, title, sizeof(title));

    if (cardH >= fullMinCard) {
        int titleY = cardTop + fullTitleTop - Fonts::rows(Fonts::body, 'H').top;
        Widgets::inkText(title, textX, titleY, Fonts::body, 1, titleColor);

        char range[20], fitted[20];
        snprintf(range, sizeof(range), "%02u:%02u - %02u:%02u", event.startMinute / 60, event.startMinute % 60,
                 event.endMinute / 60, event.endMinute % 60);
        fitInk(range, maxInk, Fonts::title, fitted, sizeof(fitted));
        int timeY = cardTop + fullTimeTop - Fonts::rows(Fonts::title, '0').top;
        Widgets::inkText(fitted, textX, timeY, Fonts::title, 1, timeColor);
    } else {
        int inkTop = Fonts::rows(Fonts::body, 'H').top;
        int inkBox = Fonts::rows(Fonts::body, 'g').bottom - inkTop;
        int titleY = cardTop + (cardH - inkBox) / 2 - inkTop;
        Widgets::inkText(title, textX, titleY, Fonts::body, 1, titleColor);
    }
}

void CalendarDayScreen::drawNowLine(uint16_t now) {
    if (_nowY < 0) return;
    if (overlapsPaint(0, _nowY - 4, 320, _nowY + 5)) {
        tft.fillRect(labelColumn + 1, _nowY - 1, 320 - labelColumn - 1, 3, Theme::alert);
        tft.fillCircle(labelColumn, _nowY, nowDotRadius, Theme::alert);
    }
    if (overlapsPaint(0, _nowY - 7, labelColumn, _nowY + 8)) {
        char label[8];
        snprintf(label, sizeof(label), "%02u:%02u", now / 60, now % 60);
        Widgets::inkTextCentered(label, labelCentre, _nowY - Fonts::capCentre(Fonts::title, 1, 'N'), Fonts::title, 1,
                                 Theme::alert);
    }
}
