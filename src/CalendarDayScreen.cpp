#include "CalendarDayScreen.h"
#include <TFT_eSPI.h>
#include <stdio.h>
#include <string.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "DateMath.h"
#include "ClockFormat.h"

extern TFT_eSPI tft;

struct CalendarSurface {
    TFT_eSPI* g;
    int dy;
};

namespace {
constexpr int headerRuleY = 26, headerCentre = 13, sideMargin = 10, screenHeight = 240;
constexpr int labelColumn = 44, labelCentre = 22;
constexpr int gridTop = 38, hourStep = 38;
constexpr int lookBackMinutes = 60;
constexpr int areaLeft = 48, areaRight = 310;
constexpr int columnGap = 2, maxColumns = 4;
constexpr int accentBar = 4, textInset = 10, textPadRight = 6;
constexpr int fullMinCard = 34, textMinCard = 17;
constexpr int fullTitleTop = 4, fullTimeTop = 22;
constexpr int nowDotRadius = 3, labelHideRange = 13;
constexpr int bandHeight = 24;

const char* const monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char* const weekdayNames[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

TFT_eSprite* band = nullptr;
bool bandFailed = false;

TFT_eSprite* bandSprite() {
    if (band || bandFailed) return band;
    band = new TFT_eSprite(&tft);
    if (!band->createSprite(320, bandHeight)) {
        delete band;
        band = nullptr;
        bandFailed = true;
    }
    return band;
}

void fillOn(const CalendarSurface& s, int x, int y, int w, int h, uint16_t color) {
    s.g->fillRect(x, y - s.dy, w, h, color);
}

void hLineOn(const CalendarSurface& s, int x, int y, int w, uint16_t color) {
    s.g->drawFastHLine(x, y - s.dy, w, color);
}

void vLineOn(const CalendarSurface& s, int x, int y, int h, uint16_t color) {
    s.g->drawFastVLine(x, y - s.dy, h, color);
}

void circleOn(const CalendarSurface& s, int x, int y, int r, uint16_t color) {
    s.g->fillCircle(x, y - s.dy, r, color);
}

void inkTextOn(const CalendarSurface& s, const char* text, int inkX, int y, const GFXfont* font, uint16_t color) {
    s.g->setFreeFont(font);
    s.g->setTextSize(1);
    s.g->setTextColor(color);
    s.g->drawString(text, inkX - Widgets::ink(text, font, 1).left, y - s.dy);
}

void inkTextCenteredOn(const CalendarSurface& s, const char* text, int centerX, int y, const GFXfont* font,
                       uint16_t color) {
    inkTextOn(s, text, centerX - Widgets::inkWidth(text, font, 1) / 2, y, font, color);
}

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
      _presetStart(startHour * 60),
      _windowStart(startHour * 60),
      _windowEnd((startHour + CalendarDay::windowHours) * 60),
      _targetStart(startHour * 60) {}

uint8_t CalendarDayScreen::windowFor(uint16_t minuteOfDay) {
    return minuteOfDay >= CalendarDay::afternoonStartHour * 60 &&
                   minuteOfDay < (CalendarDay::afternoonStartHour + CalendarDay::windowHours) * 60
               ? 1
               : 0;
}

void CalendarDayScreen::init() {
    if (_followNow) {
        int start = (int)_status.minuteOfDay() - lookBackMinutes;
        start -= start % CalendarDay::scrollStepMinutes;
        if (start < 0) start = 0;
        if (start > CalendarDay::maxStartMinute) start = CalendarDay::maxStartMinute;
        _targetStart = start;
        _followNow = false;
    } else {
        _targetStart = _presetStart;
    }
    _full = true;
}

void CalendarDayScreen::update() {}

void CalendarDayScreen::handleInput(Input input) {
    int step = 0;
    if (input == Input::FocusNext) step = CalendarDay::scrollStepMinutes;
    if (input == Input::FocusPrev) step = -(int)CalendarDay::scrollStepMinutes;
    if (step == 0) return;

    int next = (int)_targetStart + step;
    if (next < 0) next = 0;
    if (next > CalendarDay::maxStartMinute) next = CalendarDay::maxStartMinute;
    _targetStart = next;
}

void CalendarDayScreen::applyWindow(uint16_t start) {
    _windowStart = start;
    _windowEnd = start + CalendarDay::windowHours * 60;
}

int CalendarDayScreen::yOfMinute(uint16_t minute) const {
    return gridTop + ((int)(minute - _windowStart) * hourStep + 30) / 60;
}

bool CalendarDayScreen::inWindow(uint16_t minute) const {
    return minute >= _windowStart && minute < _windowEnd;
}

int CalendarDayScreen::firstLineMinute() const {
    return (_windowStart + 59) / 60 * 60;
}

int CalendarDayScreen::lineCount() const {
    int first = firstLineMinute();
    return first > _windowEnd ? 0 : (_windowEnd - first) / 60 + 1;
}

int CalendarDayScreen::lineY(int index) const {
    return yOfMinute(firstLineMinute() + index * 60);
}

bool CalendarDayScreen::overlapsPaint(int x0, int y0, int x1, int y1) const {
    return x0 < _paintX + _paintW && x1 > _paintX && y0 < _paintY + _paintH && y1 > _paintY;
}

uint8_t CalendarDayScreen::hiddenMaskFor(int nowY) const {
    uint8_t mask = 0;
    if (nowY < 0) return mask;
    int lines = lineCount();
    for (int i = 0; i < lines; i++) {
        int d = lineY(i) - nowY;
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
    if (_targetStart != _windowStart) {
        scrollRedraw(now);
        return;
    }
    updateLive(now);
}

void CalendarDayScreen::fullRedraw(uint16_t now) {
    applyWindow(_targetStart);
    _headerKey = -1;
    buildLayout(now);
    setNow(now);
    drawHeader();
    repaintRows(headerRuleY + 1, screenHeight);
    _full = false;
}

void CalendarDayScreen::scrollRedraw(uint16_t now) {
    applyWindow(_targetStart);
    buildLayout(now);
    setNow(now);
    repaintRows(headerRuleY + 1, screenHeight);
}

void CalendarDayScreen::updateLive(uint16_t now) {
    int32_t nowKey = inWindow(now) ? now : -1;
    if (nowKey != _nowKey) {
        int oldY = _nowY;
        uint8_t oldHidden = _hiddenLabels;
        setNow(now);

        int low = screenHeight, high = 0;
        if (oldY >= 0) {
            low = oldY - 7;
            high = oldY + 8;
        }
        if (_nowY >= 0) {
            if (_nowY - 7 < low) low = _nowY - 7;
            if (_nowY + 8 > high) high = _nowY + 8;
        }
        if (high > low) repaintRows(low, high);

        uint8_t changed = oldHidden ^ _hiddenLabels;
        for (int i = 0; i < lineCount(); i++) {
            if (changed & (1 << i)) repaintRows(lineY(i) - 7, lineY(i) + 8);
        }
    }

    for (uint8_t i = 0; i < _blockCount; i++) {
        Block& b = _blocks[i];
        EventState state = _agenda.stateOf(b.event, now);
        if (state == b.state) continue;
        b.state = state;
        repaintRows(b.top, b.bottom);
    }
}

void CalendarDayScreen::repaintRows(int y0, int y1) {
    if (y0 < headerRuleY + 1) y0 = headerRuleY + 1;
    if (y1 > screenHeight) y1 = screenHeight;
    for (int top = y0; top < y1; top += bandHeight) {
        int rows = y1 - top < bandHeight ? y1 - top : bandHeight;
        paintBand(top, rows);
    }
}

void CalendarDayScreen::paintBand(int top, int rows) {
    _paintX = 0;
    _paintY = top;
    _paintW = 320;
    _paintH = rows;

    TFT_eSprite* sprite = bandSprite();
    if (sprite) {
        sprite->fillSprite(Theme::background);
        CalendarSurface surface = {sprite, top};
        compose(surface);
        sprite->pushSprite(0, top, 0, 0, 320, rows);
        return;
    }

    tft.setViewport(0, top, 320, rows, false);
    tft.fillRect(0, top, 320, rows, Theme::background);
    CalendarSurface surface = {&tft, 0};
    compose(surface);
    tft.resetViewport();
}

void CalendarDayScreen::compose(const CalendarSurface& surface) {
    drawGrid(surface);
    for (uint8_t i = 0; i < _blockCount; i++) {
        const Block& b = _blocks[i];
        if (overlapsPaint(b.x, b.top, b.x + b.width, b.bottom)) drawBlock(surface, b);
    }
    drawNowLine(surface);
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

void CalendarDayScreen::drawGrid(const CalendarSurface& surface) {
    vLineOn(surface, labelColumn, headerRuleY + 1, screenHeight - (headerRuleY + 1), Theme::gridLine);
    int labelOffset = Fonts::capCentre(Fonts::title, 1, 'N');
    int firstHour = firstLineMinute() / 60;
    int lines = lineCount();
    for (int i = 0; i < lines; i++) {
        int y = lineY(i);
        if (overlapsPaint(labelColumn + 1, y, 320, y + 1)) {
            hLineOn(surface, labelColumn + 1, y, 320 - labelColumn - 1, Theme::gridLine);
        }
        if (!(_hiddenLabels & (1 << i)) && overlapsPaint(0, y - 7, labelColumn, y + 8)) {
            char label[6];
            ClockFormat::hourLabel(label, sizeof(label), (firstHour + i) % 24, _status.use24Hour);
            inkTextCenteredOn(surface, label, labelCentre, y - labelOffset, Fonts::title, Theme::textMuted);
        }
    }
}

void CalendarDayScreen::drawBlock(const CalendarSurface& surface, const Block& b) {
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
    fillOn(surface, b.x, cardTop, b.width, cardH, fill);
    fillOn(surface, b.x, cardTop, accentBar, cardH, bar);
    if (cardH < textMinCard) return;

    const AgendaEvent& event = _agenda.at(b.event);
    int textX = b.x + textInset;
    int maxInk = b.width - textInset - textPadRight;
    char title[40];
    fitInk(event.title, maxInk, Fonts::body, title, sizeof(title));

    if (cardH >= fullMinCard) {
        int titleY = cardTop + fullTitleTop - Fonts::rows(Fonts::body, 'H').top;
        inkTextOn(surface, title, textX, titleY, Fonts::body, titleColor);

        char range[20], fitted[20];
        ClockFormat::range(range, sizeof(range), event.startMinute, event.endMinute, _status.use24Hour);
        fitInk(range, maxInk, Fonts::title, fitted, sizeof(fitted));
        int timeY = cardTop + fullTimeTop - Fonts::rows(Fonts::title, '0').top;
        inkTextOn(surface, fitted, textX, timeY, Fonts::title, timeColor);
    } else {
        int inkTop = Fonts::rows(Fonts::body, 'H').top;
        int inkBox = Fonts::rows(Fonts::body, 'g').bottom - inkTop;
        int titleY = cardTop + (cardH - inkBox) / 2 - inkTop;
        inkTextOn(surface, title, textX, titleY, Fonts::body, titleColor);
    }
}

void CalendarDayScreen::drawNowLine(const CalendarSurface& surface) {
    if (_nowY < 0) return;
    if (overlapsPaint(0, _nowY - 4, 320, _nowY + 5)) {
        fillOn(surface, labelColumn + 1, _nowY - 1, 320 - labelColumn - 1, 3, Theme::alert);
        circleOn(surface, labelColumn, _nowY, nowDotRadius, Theme::alert);
    }
    if (overlapsPaint(0, _nowY - 7, labelColumn, _nowY + 8)) {
        char label[8];
        ClockFormat::time(label, sizeof(label), _nowKey, _status.use24Hour);
        inkTextCenteredOn(surface, label, labelCentre, _nowY - Fonts::capCentre(Fonts::title, 1, 'N'), Fonts::title,
                          Theme::alert);
    }
}
