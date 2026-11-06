#include "WorldMap.h"
#include <TFT_eSPI.h>
#include <math.h>
#include "Theme.h"

extern TFT_eSPI tft;

namespace {
constexpr int originX = 5, originY = 8, pitch = 5, dotSize = 3;
constexpr int cols = 62, rows = 23;
constexpr double latTop = 84.0, latPerRow = 6.2;
constexpr double degToRad = M_PI / 180.0;
constexpr double radToDeg = 180.0 / M_PI;
constexpr bool dimNightSide = false;
constexpr int terminatorStep = 4;
constexpr int terminatorWrapJump = 150;

constexpr uint64_t bits(int first, int last) {
    return ((1ULL << (last + 1)) - 1) & ~((1ULL << first) - 1);
}

constexpr uint64_t land[rows] = {
    bits(9, 17) | bits(19, 27) | bits(40, 40) | bits(45, 50) | bits(55, 55),
    bits(0, 0) | bits(3, 19) | bits(21, 26) | bits(34, 37) | bits(39, 61),
    bits(2, 19) | bits(22, 23) | bits(27, 27) | bits(32, 61),
    bits(3, 4) | bits(7, 15) | bits(17, 20) | bits(30, 30) | bits(32, 55) | bits(57, 58),
    bits(9, 21) | bits(29, 55) | bits(58, 58),
    bits(9, 20) | bits(30, 55),
    bits(9, 18) | bits(29, 31) | bits(33, 53) | bits(55, 55),
    bits(10, 17) | bits(29, 32) | bits(36, 51) | bits(53, 54),
    bits(11, 16) | bits(29, 51),
    bits(12, 13) | bits(17, 17) | bits(28, 51),
    bits(13, 15) | bits(28, 40) | bits(43, 45) | bits(47, 49),
    bits(16, 16) | bits(18, 19) | bits(28, 39) | bits(44, 44) | bits(48, 49) | bits(52, 52),
    bits(17, 21) | bits(29, 39) | bits(48, 48) | bits(52, 52),
    bits(17, 22) | bits(32, 38) | bits(48, 48) | bits(50, 51),
    bits(17, 24) | bits(33, 37) | bits(48, 50) | bits(54, 56),
    bits(17, 24) | bits(33, 37) | bits(55, 55),
    bits(18, 23) | bits(33, 39) | bits(52, 55),
    bits(19, 23) | bits(33, 36) | bits(38, 39) | bits(50, 56),
    bits(18, 22) | bits(34, 36) | bits(50, 57),
    bits(18, 21) | bits(34, 35) | bits(51, 52) | bits(54, 56),
    bits(18, 20) | bits(56, 56) | bits(61, 61),
    bits(18, 19) | bits(60, 60),
    bits(18, 19),
};

uint64_t nightRows[rows];
SunCalc::Position sun = {0, 0, 0};

double wrap180(double degrees) {
    while (degrees > 180.0) degrees -= 360.0;
    while (degrees < -180.0) degrees += 360.0;
    return degrees;
}

int firstCellAtOrAfter(int pixel, int origin) {
    int value = pixel - origin - dotSize;
    return value <= 0 ? 0 : (value + pitch - 1) / pitch;
}

int lastCellAtOrBefore(int pixel, int origin, int count) {
    int value = pixel - origin - 1;
    int cell = value < 0 ? -1 : value / pitch;
    return cell >= count ? count - 1 : cell;
}
}

namespace WorldMap {
void project(float latitude, float longitude, int& x, int& y) {
    double column = (longitude + 180.0) * cols / 360.0 - 0.5;
    double row = (latTop - latitude) / latPerRow;
    x = (int)lround(originX + 2 + pitch * column);
    y = (int)lround(originY + 2 + pitch * row);
}

void computeNight(const SunCalc::Position& position) {
    sun = position;
    if (!dimNightSide) return;
    double sinDecl = sin(sun.declination * degToRad);
    double cosDecl = cos(sun.declination * degToRad);

    double cosHour[cols];
    for (int k = 0; k < cols; k++) {
        double longitude = -180.0 + (k + 0.5) * 360.0 / cols;
        cosHour[k] = cos((longitude - sun.subsolarLongitude) * degToRad);
    }

    for (int r = 0; r < rows; r++) {
        double latitude = (latTop - r * latPerRow) * degToRad;
        double sinLat = sin(latitude), cosLat = cos(latitude);
        uint64_t mask = 0;
        for (int k = 0; k < cols; k++) {
            if (sinLat * sinDecl + cosLat * cosDecl * cosHour[k] < 0) mask |= 1ULL << k;
        }
        nightRows[r] = mask;
    }
}

void drawLand(int x0, int y0, int x1, int y1) {
    if (x0 < 0) x0 = 0;
    if (x1 > 319) x1 = 319;
    if (y0 < top) y0 = top;
    if (y1 > bottom) y1 = bottom;
    if (x1 < x0 || y1 < y0) return;

    tft.fillRect(x0, y0, x1 - x0 + 1, y1 - y0 + 1, Theme::background);

    int kMin = firstCellAtOrAfter(x0, originX);
    int kMax = lastCellAtOrBefore(x1, originX, cols);
    int rMin = firstCellAtOrAfter(y0, originY);
    int rMax = lastCellAtOrBefore(y1, originY, rows);

    for (int r = rMin; r <= rMax; r++) {
        for (int k = kMin; k <= kMax; k++) {
            if (!(land[r] >> k & 1)) continue;
            uint16_t color = (dimNightSide && (nightRows[r] >> k & 1)) ? Theme::mapNight : Theme::mapDot;
            tft.fillRect(originX + 1 + pitch * k, originY + 1 + pitch * r, dotSize, dotSize, color);
        }
    }
}

void drawTerminator() {
    double sinDecl = sin(sun.declination * degToRad);
    double cosDecl = cos(sun.declination * degToRad);

    int previousX = 0, previousY = 0;
    bool havePrevious = false;
    for (int bearing = 0; bearing <= 360; bearing += terminatorStep) {
        double theta = bearing * degToRad;
        double sinLat = cosDecl * cos(theta);
        double latitude = asin(sinLat) * radToDeg;
        double longitude = wrap180(sun.subsolarLongitude + atan2(sin(theta) * cosDecl, -sinDecl * sinLat) * radToDeg);

        int x, y;
        project((float)latitude, (float)longitude, x, y);
        bool inside = y >= top + 1 && y <= bottom - 2;
        if (havePrevious && inside && abs(x - previousX) < terminatorWrapJump) {
            tft.drawLine(previousX, previousY, x, y, Theme::terminator);
            tft.drawLine(previousX, previousY + 1, x, y + 1, Theme::terminator);
        }
        havePrevious = inside;
        previousX = x;
        previousY = y;
    }
}
}
