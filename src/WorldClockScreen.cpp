#include "WorldClockScreen.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "WorldCities.h"
#include "WorldMap.h"
#include "DateMath.h"
#include "ListFocus.h"

extern TFT_eSPI tft;

namespace {
constexpr int panelY = 138;
constexpr int panelLeft = 13;
constexpr int sunMarkX = 240;
constexpr int sunTextX = 254;
constexpr int markerReach = 8;
constexpr int nameTop = 15;
constexpr int timeTop = 25;
constexpr int offsetTop = 81;
constexpr int riseTop = 34;
constexpr int setTop = 56;
constexpr int meridiemGap = 6;
constexpr int timeRightLimit = 228;

int32_t floorDiv(int32_t value, int32_t divisor) {
    int32_t quotient = value / divisor;
    return (value % divisor != 0 && (value < 0) != (divisor < 0)) ? quotient - 1 : quotient;
}

void formatOffset(int city, int dayDifference, char* out, size_t size) {
    if (city == localCity) {
        snprintf(out, size, "Home");
        return;
    }
    int diff = worldCities[city].offsetMinutes - localOffsetMinutes;
    char sign = diff < 0 ? '-' : '+';
    int magnitude = diff < 0 ? -diff : diff;
    const char* day = dayDifference > 0 ? "Tomorrow" : (dayDifference < 0 ? "Yesterday" : "Today");
    snprintf(out, size, "%s, %c%d:%02d hrs", day, sign, magnitude / 60, magnitude % 60);
}

void formatClock(bool known, int minute, bool use24h, char* out, size_t size) {
    if (!known) {
        snprintf(out, size, "--:--");
        return;
    }
    int hour = minute / 60;
    if (use24h) {
        snprintf(out, size, "%02d:%02d", hour, minute % 60);
    } else {
        snprintf(out, size, "%d:%02d", hour % 12 == 0 ? 12 : hour % 12, minute % 60);
    }
}

void drawSunMark(int centerX, int centerY) {
    tft.fillCircle(centerX, centerY, 5, Theme::textMuted);
    tft.fillRect(centerX - 8, centerY + 1, 16, 5, Theme::background);
    tft.fillRect(centerX - 8, centerY + 1, 16, 1, Theme::textMuted);
}
}

WorldClockScreen::WorldClockScreen(const DeviceStatus& status) : _status(status), _selected(localCity) {}

void WorldClockScreen::init() {
    _full = true;
}

void WorldClockScreen::update() {}

void WorldClockScreen::handleInput(Input input) {
    if (input == Input::FocusNext || input == Input::FocusPrev) {
        int next = stepIndex(_selected, input == Input::FocusNext ? 1 : -1, worldCityCount);
        if (next != _selected) {
            _selected = next;
            _markersDirty = true;
            _infoDirty = true;
        }
    }
    if (input == Input::Select) {
        _use24h = !_use24h;
        _infoDirty = true;
    }
}

WorldClockScreen::Moment WorldClockScreen::momentIn(int offsetMinutes) const {
    int32_t localStamp = Date::daysFromCivil(_status.now.year, _status.now.month, _status.now.day) * 1440 + _status.minuteOfDay();
    int32_t cityStamp = localStamp - localOffsetMinutes + offsetMinutes;
    int32_t days = floorDiv(cityStamp, 1440);
    return {days, (int)(cityStamp - days * 1440)};
}

void WorldClockScreen::draw() {
    if (_full) {
        tft.fillScreen(Theme::background);
        _full = false;
        _stamp = -1;
        _infoDirty = true;
    }

    int32_t stamp = Date::daysFromCivil(_status.now.year, _status.now.month, _status.now.day) * 1440 + _status.minuteOfDay();
    if (stamp != _stamp) {
        _stamp = stamp;
        _sun = SunCalc::at((int64_t)stamp - localOffsetMinutes);
        WorldMap::computeNight(_sun);
        redrawMap();
        _markersDirty = false;
        _infoDirty = true;
    } else if (_markersDirty) {
        _markersDirty = false;
        refreshMarkers();
    }

    if (_infoDirty) {
        _infoDirty = false;
        drawInfo();
    }
}

void WorldClockScreen::redrawMap() {
    WorldMap::drawLand(0, WorldMap::top, 319, WorldMap::bottom);
    WorldMap::drawTerminator();
    drawMarkers();
}

void WorldClockScreen::refreshMarkers() {
    for (int i = 0; i < worldCityCount; i++) {
        int x, y;
        WorldMap::project(worldCities[i].latitude, worldCities[i].longitude, x, y);
        WorldMap::drawLand(x - markerReach, y - markerReach, x + markerReach, y + markerReach);
    }
    WorldMap::drawTerminator();
    drawMarkers();
}

void WorldClockScreen::drawMarkers() {
    for (int i = 0; i < worldCityCount; i++) {
        if (i == _selected) continue;
        int x, y;
        WorldMap::project(worldCities[i].latitude, worldCities[i].longitude, x, y);
        tft.fillCircle(x, y, 5, Theme::background);
        tft.fillCircle(x, y, 3, Theme::accent);
    }
    int x, y;
    WorldMap::project(worldCities[_selected].latitude, worldCities[_selected].longitude, x, y);
    tft.fillCircle(x, y, 6, Theme::text);
    tft.fillCircle(x, y, 4, Theme::accent);
}

void WorldClockScreen::drawInfo() {
    const City& city = worldCities[_selected];
    Moment moment = momentIn(city.offsetMinutes);
    Moment home = momentIn(localOffsetMinutes);

    tft.fillRect(0, panelY, 320, 240 - panelY, Theme::background);
    tft.drawFastHLine(10, panelY, 300, Theme::outline);

    Widgets::inkText(city.name, panelLeft, panelY + nameTop, Fonts::title, 1, Theme::accent);

    int hour = moment.minute / 60;
    char time[8];
    formatClock(true, moment.minute, _use24h, time, sizeof(time));
    const char* meridiem = hour < 12 ? "AM" : "PM";
    Widgets::Ink meridiemInk = Widgets::ink(meridiem, Fonts::title, 1);

    int scale = 3;
    Widgets::Ink timeInk = Widgets::ink(time, Fonts::body, scale);
    while (scale > 1) {
        int right = panelLeft + timeInk.width + (_use24h ? 0 : meridiemGap + meridiemInk.width);
        if (right <= timeRightLimit) break;
        scale--;
        timeInk = Widgets::ink(time, Fonts::body, scale);
    }
    int timeY = panelY + timeTop;
    Widgets::inkText(time, panelLeft, timeY, Fonts::body, scale, Theme::text);

    if (!_use24h) {
        Widgets::Rows timeRows = Widgets::inkRows(time, Fonts::body, scale);
        Widgets::Rows meridiemRows = Widgets::inkRows(meridiem, Fonts::title, 1);
        int meridiemY = timeY + timeRows.top + timeRows.height - (meridiemRows.top + meridiemRows.height);
        Widgets::inkText(meridiem, panelLeft + timeInk.width + meridiemGap, meridiemY, Fonts::title, 1, Theme::textMuted);
    }

    char offset[28];
    formatOffset(_selected, (int)(moment.days - home.days), offset, sizeof(offset));
    Widgets::inkText(offset, panelLeft, panelY + offsetTop, Fonts::body, 1, Theme::textMuted);

    int rise = 0, set = 0;
    bool known = SunCalc::riseSet(city.latitude, city.longitude, moment.days, city.offsetMinutes, rise, set);
    char riseText[8], setText[8];
    formatClock(known, rise, _use24h, riseText, sizeof(riseText));
    formatClock(known, set, _use24h, setText, sizeof(setText));

    drawSunMark(sunMarkX, panelY + 46);
    tft.fillTriangle(sunMarkX, panelY + riseTop, sunMarkX - 4, panelY + riseTop + 4, sunMarkX + 4, panelY + riseTop + 4, Theme::textMuted);
    Widgets::inkText(riseText, sunTextX, panelY + riseTop, Fonts::body, 1, Theme::textMuted);

    drawSunMark(sunMarkX, panelY + 68);
    tft.fillTriangle(sunMarkX - 4, panelY + setTop, sunMarkX + 4, panelY + setTop, sunMarkX, panelY + setTop + 4, Theme::textMuted);
    Widgets::inkText(setText, sunTextX, panelY + setTop, Fonts::body, 1, Theme::textMuted);
}
