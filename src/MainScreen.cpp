#include "MainScreen.h"
#include <TFT_eSPI.h>
#include <math.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "ClockFormat.h"

extern TFT_eSPI tft;

namespace {
constexpr int soonWithinMinutes = 30;
const char* const weekdayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

constexpr int clockCenterX = 160, clockY = 31, clockScale = 4, clockHalfGap = 32;
constexpr int dateY = 105, barY = 133;

constexpr int roomX = 10, roomY = 150, roomW = 104, cardH = 80;
constexpr int nextX = 120, nextY = 150, nextW = 190;
constexpr int nextPadLeft = 14, nextPadRight = 10;
constexpr int nextLabelY = 6, nextTitleCentre = 35, nextWhenCentre = 62;
constexpr int nextBarTop = 12, nextBarHeight = 56;

int drawYForCentre(const GFXfont* font, uint8_t scale, int centre) {
    return centre - Fonts::capCentre(font, scale, 'N');
}

void boldText(const char* s, int x, int y, const GFXfont* font, uint8_t size, uint16_t color) {
    Widgets::text(s, x, y, font, size, color);
    Widgets::text(s, x + 1, y, font, size, color);
}
}

MainScreen::MainScreen(const DeviceStatus& status, const Agenda& agenda, const NavState& nav)
    : _status(status), _agenda(agenda), _nav(nav) {
    resetKeys();
}

void MainScreen::resetKeys() {
    _statusKey = _clockKey = _colonKey = _dateKey = _barKey = _roomKey = _nextKey = -1;
}

void MainScreen::init() {
    _full = true;
}

void MainScreen::update() {}

void MainScreen::handleInput(Input input) {}

void MainScreen::draw() {
    if (_full) {
        tft.fillScreen(Theme::background);
        resetKeys();
        _full = false;
    }
    drawStatus();
    drawClock();
    drawColon();
    drawDate();
    drawDayBar();
    drawRoomCard();
    drawNextUpCard();
}

void MainScreen::drawStatus() {
    int32_t key = _status.wifiConnected | (_status.sdOk << 1);
    if (key == _statusKey) return;
    _statusKey = key;
    Widgets::statusBar(_status, _nav.subPage, _nav.subPageCount);
}

void MainScreen::drawClock() {
    int32_t key = _status.minuteOfDay();
    if (key == _clockKey) return;
    _clockKey = key;

    char hours[3], minutes[3];
    unsigned shownHour = _status.use24Hour ? _status.now.hour : (_status.now.hour % 12 == 0 ? 12 : _status.now.hour % 12);
    snprintf(hours, sizeof(hours), "%02u", shownHour);
    snprintf(minutes, sizeof(minutes), "%02u", _status.now.minute);

    int hoursInk = Widgets::inkWidth(hours, Fonts::body, clockScale);
    tft.fillRect(0, 28, 320, 72, Theme::background);
    Widgets::inkText(hours, clockCenterX - clockHalfGap - hoursInk, clockY, Fonts::body, clockScale, Theme::text);
    Widgets::inkText(minutes, clockCenterX + clockHalfGap, clockY, Fonts::body, clockScale, Theme::text);
    _colonKey = -1;
}

void MainScreen::drawColon() {
    int32_t key = _status.now.second % 2;
    if (key == _colonKey) return;
    _colonKey = key;

    Fonts::GlyphBox digit = Fonts::glyph(Fonts::body, '0');
    int digitTop = clockY + (Fonts::ascent(Fonts::body) + digit.yOffset) * clockScale;
    int digitHeight = digit.height * clockScale;
    int dot = digitHeight / 5;
    int middle = digitTop + digitHeight / 2;
    int spread = digitHeight / 4;
    uint16_t color = key == 0 ? Theme::accent : Theme::background;

    tft.fillRect(clockCenterX - dot / 2, middle - spread - dot / 2, dot, dot, color);
    tft.fillRect(clockCenterX - dot / 2, middle + spread - dot / 2, dot, dot, color);
}

void MainScreen::drawDate() {
    int32_t key = _status.now.year * 10000 + _status.now.month * 100 + _status.now.day;
    if (key == _dateKey) return;
    _dateKey = key;

    char date[12];
    ClockFormat::date(date, sizeof(date), _status.now, _status.dateFormat);
    const char* day = weekdayNames[_status.now.weekday % 7];

    constexpr int gap = 16;
    int dayWidth = Widgets::textWidth(day, Fonts::body, 1);
    int dateWidth = Widgets::textWidth(date, Fonts::body, 1);
    int x = 160 - (dayWidth + gap + dateWidth) / 2;

    tft.fillRect(0, dateY - 3, 320, 22, Theme::background);
    Widgets::text(day, x, dateY, Fonts::body, 1, Theme::textMuted);
    Widgets::text(date, x + dayWidth + gap, dateY, Fonts::body, 1, Theme::textMuted);
}

void MainScreen::drawDayBar() {
    int32_t key = _status.minuteOfDay() * 100 / 1440;
    if (key == _barKey) return;
    _barKey = key;

    tft.fillRect(20, barY, 280, 4, Theme::outline);
    tft.fillRect(20, barY, key * 280 / 100, 4, Theme::accent);
}

void MainScreen::drawRoomCard() {
    int32_t key = _status.hasClimate ? lroundf(_status.temperatureC) * 1000 + lroundf(_status.humidity) : -2;
    if (key == _roomKey) return;
    _roomKey = key;

    Widgets::card(roomX, roomY, roomW, cardH);
    Widgets::text("Room", roomX + 10, roomY + 6, Fonts::title, 1, Theme::textMuted);

    if (!_status.hasClimate) {
        Widgets::text("--", roomX + 10, roomY + 20, Fonts::body, 2, Theme::textDim);
        Widgets::text("No sensor", roomX + 10, roomY + 60, Fonts::title, 1, Theme::textMuted);
        return;
    }

    char temp[8], humidity[16];
    snprintf(temp, sizeof(temp), "%ld", lroundf(_status.temperatureC));
    snprintf(humidity, sizeof(humidity), "Hum %ld%%", lroundf(_status.humidity));

    Widgets::text(temp, roomX + 10, roomY + 20, Fonts::body, 2, Theme::text);
    int unitX = roomX + 10 + Widgets::textWidth(temp, Fonts::body, 2);
    tft.drawCircle(unitX + 7, roomY + 24, 3, Theme::text);
    tft.drawCircle(unitX + 7, roomY + 24, 2, Theme::text);
    Widgets::text("C", unitX + 13, roomY + 28, Fonts::title, 2, Theme::text);
    Widgets::text(humidity, roomX + 10, roomY + 60, Fonts::title, 1, Theme::textMuted);
}

void MainScreen::drawNextUpCard() {
    uint16_t now = _status.minuteOfDay();
    bool synced = _agenda.synced();
    int next = synced ? _agenda.nextUpcoming(now) : -1;
    int until = next >= 0 ? _agenda.at(next).startMinute - now : 0;
    bool soon = next >= 0 && until <= soonWithinMinutes;

    int32_t state = !synced ? -3 : next < 0 ? -2 : next * 1000 + (soon ? until : 999);
    int32_t key = (int32_t)(_agenda.revision() % 1000) * 100000 + state;
    if (key == _nextKey) return;
    _nextKey = key;

    Widgets::card(nextX, nextY, nextW, cardH);
    int textX = nextX + nextPadLeft;
    int textMaxWidth = nextW - nextPadLeft - nextPadRight;
    Widgets::text("Next up", textX, nextY + nextLabelY, Fonts::title, 1, Theme::textMuted);

    if (!synced) {
        tft.fillRect(nextX, nextY + nextBarTop, 4, nextBarHeight, Theme::textDim);
        Widgets::text("No calendar", textX, nextY + drawYForCentre(Fonts::title, 2, nextTitleCentre), Fonts::title, 2,
                      Theme::textMuted);
        Widgets::text("Not synced yet", textX, nextY + drawYForCentre(Fonts::title, 1, nextWhenCentre), Fonts::title, 1,
                      Theme::textMuted);
        return;
    }

    if (next < 0) {
        tft.fillRect(nextX, nextY + nextBarTop, 4, nextBarHeight, Theme::good);
        Widgets::text("All clear", textX, nextY + drawYForCentre(Fonts::title, 2, nextTitleCentre), Fonts::title, 2,
                      Theme::good);
        Widgets::text(_agenda.count() == 0 ? "No events today" : "Nothing else today", textX,
                      nextY + drawYForCentre(Fonts::title, 1, nextWhenCentre), Fonts::title, 1, Theme::textMuted);
        return;
    }

    const AgendaEvent& event = _agenda.at(next);
    uint16_t color = soon ? Theme::alert : Theme::accent;
    tft.fillRect(nextX, nextY + nextBarTop, 4, nextBarHeight, color);

    uint8_t titleScale = Widgets::textWidth(event.title, Fonts::title, 2) <= textMaxWidth ? 2 : 1;
    Widgets::fitText(event.title, textX, nextY + drawYForCentre(Fonts::title, titleScale, nextTitleCentre), textMaxWidth,
                     Fonts::title, titleScale, Theme::text);

    char when[24];
    if (soon) {
        snprintf(when, sizeof(when), "in %d min", until);
    } else {
        ClockFormat::range(when, sizeof(when), event.startMinute, event.endMinute, _status.use24Hour);
    }
    boldText(when, textX, nextY + drawYForCentre(Fonts::body, 1, nextWhenCentre), Fonts::body, 1, color);
}
