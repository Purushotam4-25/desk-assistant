#include "MainScreen.h"
#include <TFT_eSPI.h>
#include <math.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"

extern TFT_eSPI tft;

namespace {
constexpr int soonWithinMinutes = 30;
const char* const weekdayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

constexpr int clockCenterX = 160, clockY = 34, clockScale = 4, clockHalfGap = 24;

constexpr int roomX = 10, roomY = 152, roomW = 104, cardH = 80;
constexpr int nextX = 120, nextY = 152, nextW = 190;
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
    snprintf(hours, sizeof(hours), "%02u", _status.now.hour);
    snprintf(minutes, sizeof(minutes), "%02u", _status.now.minute);

    int hoursWidth = Widgets::textWidth(hours, Fonts::body, clockScale);
    tft.fillRect(0, 28, 320, 72, Theme::background);
    Widgets::text(hours, clockCenterX - clockHalfGap - hoursWidth, clockY, Fonts::body, clockScale, Theme::text);
    Widgets::text(minutes, clockCenterX + clockHalfGap, clockY, Fonts::body, clockScale, Theme::text);
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
    snprintf(date, sizeof(date), "%02u/%02u/%02u", _status.now.day, _status.now.month, _status.now.year % 100);
    const char* day = weekdayNames[_status.now.weekday % 7];

    constexpr int gap = 16;
    int dayWidth = Widgets::textWidth(day, Fonts::body, 1);
    int dateWidth = Widgets::textWidth(date, Fonts::body, 1);
    int x = 160 - (dayWidth + gap + dateWidth) / 2;

    tft.fillRect(0, 102, 320, 22, Theme::background);
    Widgets::text(day, x, 104, Fonts::body, 1, Theme::textMuted);
    Widgets::text(date, x + dayWidth + gap, 104, Fonts::body, 1, Theme::textMuted);
}

void MainScreen::drawDayBar() {
    int32_t key = _status.minuteOfDay() * 100 / 1440;
    if (key == _barKey) return;
    _barKey = key;

    tft.fillRect(20, 128, 280, 4, Theme::outline);
    tft.fillRect(20, 128, key * 280 / 100, 4, Theme::accent);
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
    int next = _agenda.nextUpcoming(now);
    int until = next >= 0 ? _agenda.at(next).startMinute - now : 0;
    bool soon = next >= 0 && until <= soonWithinMinutes;

    int32_t key = next < 0 ? -2 : next * 1000 + (soon ? until : 999);
    if (key == _nextKey) return;
    _nextKey = key;

    Widgets::card(nextX, nextY, nextW, cardH);
    int textX = nextX + 14;
    int textMaxWidth = nextW - 14 - 10;
    Widgets::text("Next up", textX, nextY + 6, Fonts::title, 1, Theme::textMuted);

    if (next < 0) {
        tft.fillRect(nextX, nextY + 12, 4, 56, Theme::good);
        Widgets::text("All clear", textX, nextY + 28, Fonts::title, 2, Theme::good);
        Widgets::text("Nothing else today", textX, nextY + 54, Fonts::title, 1, Theme::textMuted);
        return;
    }

    const AgendaEvent& event = _agenda.at(next);
    uint16_t color = soon ? Theme::alert : Theme::accent;
    tft.fillRect(nextX, nextY + 12, 4, 56, color);
    Widgets::fitText(event.title, textX, nextY + 23, textMaxWidth, Fonts::title, 2, Theme::text);

    char when[24];
    if (soon) {
        snprintf(when, sizeof(when), "in %d min", until);
        Widgets::text(when, textX, nextY + 46, Fonts::body, 2, color);
    } else {
        snprintf(when, sizeof(when), "%02u:%02u - %02u:%02u", event.startMinute / 60, event.startMinute % 60,
                 event.endMinute / 60, event.endMinute % 60);
        Widgets::text(when, textX, nextY + 52, Fonts::body, 1, color);
    }
}
