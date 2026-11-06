#include "FocusStatsScreen.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "TimeFormat.h"
#include "DateMath.h"

extern TFT_eSPI tft;

namespace {
constexpr int rightEdge = 310;
constexpr int cardY = 62, cardW = 146, cardH = 88, todayX = 10, weekX = 164;
constexpr int cardPadLeft = 12, cardPadRight = 8;
constexpr int chartX = 10, chartY = 152, barW = 30, barStep = 42, barMaxHeight = 50;
const char* const monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char* const dayNames[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
const char dayLetters[] = "MTWTFSS";
}

FocusStatsScreen::FocusStatsScreen(const DeviceStatus& status, const FocusSession& session, const NavState& nav)
    : _status(status), _session(session), _nav(nav) {
    resetKeys();
}

void FocusStatsScreen::resetKeys() {
    _statusKey = _headerKey = _todayKey = _weekKey = _chartKey = -1;
}

void FocusStatsScreen::init() {
    _full = true;
}

void FocusStatsScreen::update() {}

void FocusStatsScreen::handleInput(Input input) {}

void FocusStatsScreen::draw() {
    if (_full) {
        tft.fillScreen(Theme::background);
        resetKeys();
        _full = false;
    }
    drawStatus();
    drawHeader();
    drawTodayCard();
    drawWeekCard();
    drawChart();
}

void FocusStatsScreen::drawStatus() {
    int32_t key = _status.wifiConnected | (_status.sdOk << 1);
    if (key == _statusKey) return;
    _statusKey = key;
    Widgets::statusBar(_status, _nav.subPage, _nav.subPageCount);
}

void FocusStatsScreen::drawHeader() {
    int32_t start = _session.weekStartDay();
    if (start == _headerKey) return;
    _headerKey = start;

    int startYear, endYear;
    unsigned startMonth, startDay, endMonth, endDay;
    Date::civilFromDays(start, startYear, startMonth, startDay);
    Date::civilFromDays(start + 6, endYear, endMonth, endDay);

    char range[24];
    if (startMonth == endMonth) {
        snprintf(range, sizeof(range), "%s %u - %u", monthNames[startMonth - 1], startDay, endDay);
    } else {
        snprintf(range, sizeof(range), "%s %u - %s %u", monthNames[startMonth - 1], startDay,
                 monthNames[endMonth - 1], endDay);
    }

    tft.fillRect(0, 28, 320, 33, Theme::background);
    Widgets::text("Focus stats", 9, 33, Fonts::title, 2, Theme::text);
    int width = Widgets::textWidth(range, Fonts::title, 1);
    Widgets::text(range, rightEdge - width, 45, Fonts::title, 1, Theme::textMuted);
}

void FocusStatsScreen::drawCard(int x, const char* label, uint16_t labelColor, const char* value, const char* line1,
                                const char* line2) {
    Widgets::card(x, cardY, cardW, cardH);
    int innerX = x + cardPadLeft;
    int innerWidth = cardW - cardPadLeft - cardPadRight;

    int labelTop = cardY + 5;
    Widgets::inkText(label, innerX, labelTop, Fonts::title, 1, labelColor);

    int scale = 3;
    while (scale > 1 && Widgets::inkWidth(value, Fonts::glcd, scale) > innerWidth) scale--;
    int valueHeight = 7 * scale;

    int labelBottom = labelTop + Fonts::ascent(Fonts::title);
    int linesTop = cardY + 52;
    int linesInkTop = linesTop + Fonts::ascent(Fonts::title) + Fonts::glyph(Fonts::title, 'A').yOffset;
    int areaTop = labelBottom + 2;
    int valueTop = areaTop + (linesInkTop - 2 - areaTop - valueHeight) / 2;
    Widgets::inkText(value, innerX, valueTop, Fonts::glcd, scale, Theme::text);

    Widgets::fitText(line1, innerX, linesTop, innerWidth, Fonts::title, 1, Theme::textMuted);
    Widgets::fitText(line2, innerX, linesTop + 16, innerWidth, Fonts::title, 1, Theme::textMuted);
}

void FocusStatsScreen::drawTodayCard() {
    uint32_t minutes = _session.todaySeconds() / 60;
    uint8_t sessions = _session.sessionsToday();
    int32_t key = (int32_t)minutes * 256 + sessions;
    if (key == _todayKey) return;
    _todayKey = key;

    uint32_t goal = _session.goalSeconds();
    unsigned percent = goal ? _session.todaySeconds() * 100UL / goal : 0;

    char value[12], line1[20], line2[20];
    formatMinutes(value, sizeof(value), minutes);
    snprintf(line1, sizeof(line1), "%u%% of goal", percent);
    snprintf(line2, sizeof(line2), "%u session%s", sessions, sessions == 1 ? "" : "s");
    drawCard(todayX, "Today", Theme::accent, value, line1, line2);
}

void FocusStatsScreen::drawWeekCard() {
    uint32_t minutes = _session.weekSeconds() / 60;
    uint8_t today = _session.todayIndex();

    int best = -1;
    uint32_t bestSeconds = 0;
    for (int i = 0; i < FocusSession::weekDays; i++) {
        uint32_t seconds = _session.daySeconds(i);
        if (seconds > bestSeconds) {
            bestSeconds = seconds;
            best = i;
        }
    }

    int32_t key = ((int32_t)minutes * 8 + today) * 8 + (best + 1);
    if (key == _weekKey) return;
    _weekKey = key;

    char value[12], average[12], line1[24], line2[20];
    formatMinutes(value, sizeof(value), minutes);
    formatMinutes(average, sizeof(average), minutes / (today + 1));
    snprintf(line1, sizeof(line1), "Avg %s/day", average);
    snprintf(line2, sizeof(line2), "Best: %s", best < 0 ? "none" : dayNames[best]);
    drawCard(weekX, "This week", Theme::textMuted, value, line1, line2);
}

void FocusStatsScreen::drawChart() {
    uint8_t today = _session.todayIndex();
    uint32_t values[FocusSession::weekDays];
    uint32_t scale = _session.goalSeconds();
    uint32_t key = today;
    for (int i = 0; i < FocusSession::weekDays; i++) {
        values[i] = _session.daySeconds(i);
        if (values[i] > scale) scale = values[i];
        key = key * 131 + values[i] / 60;
    }
    if ((int32_t)key == _chartKey) return;
    _chartKey = key;
    if (scale == 0) scale = 1;

    tft.fillRect(0, chartY - 2, 320, 240 - (chartY - 2), Theme::background);
    for (int i = 0; i < FocusSession::weekDays; i++) {
        int x = chartX + 9 + i * barStep;
        int height = values[i] * barMaxHeight / scale;
        uint16_t barColor = i == today ? Theme::accent : (i < today ? Theme::barPast : Theme::outline);
        tft.fillRect(x, chartY + 58 - height, barW, 2 + height, barColor);

        char letter[2] = {dayLetters[i], 0};
        int width = Widgets::textWidth(letter, Fonts::body, 1);
        Widgets::text(letter, x + (barW - width) / 2, chartY + 69, Fonts::body, 1,
                      i == today ? Theme::accent : Theme::textMuted);
    }
    tft.fillRect(chartX, chartY + 63, 300, 1, Theme::outline);
}
