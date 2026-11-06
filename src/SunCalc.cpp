#include "SunCalc.h"
#include <math.h>

namespace {
constexpr double degToRad = M_PI / 180.0;
constexpr double radToDeg = 180.0 / M_PI;

double wrap360(double degrees) {
    degrees = fmod(degrees, 360.0);
    return degrees < 0 ? degrees + 360.0 : degrees;
}

double wrap180(double degrees) {
    degrees = wrap360(degrees);
    return degrees > 180.0 ? degrees - 360.0 : degrees;
}

int wrapDay(double minutes) {
    int rounded = (int)lround(minutes) % 1440;
    return rounded < 0 ? rounded + 1440 : rounded;
}
}

namespace SunCalc {
Position at(int64_t utcMinutesSinceEpoch) {
    double n = (double)utcMinutesSinceEpoch / 1440.0 + 2440587.5 - 2451545.0;
    double meanLongitude = wrap360(280.460 + 0.9856474 * n);
    double anomaly = wrap360(357.528 + 0.9856003 * n) * degToRad;
    double eclipticLongitude = (meanLongitude + 1.915 * sin(anomaly) + 0.020 * sin(2 * anomaly)) * degToRad;
    double obliquity = (23.439 - 0.0000004 * n) * degToRad;

    double declination = asin(sin(obliquity) * sin(eclipticLongitude)) * radToDeg;
    double rightAscension = atan2(cos(obliquity) * sin(eclipticLongitude), cos(eclipticLongitude)) * radToDeg;
    double siderealTime = wrap360(280.46061837 + 360.98564736629 * n);

    Position position;
    position.declination = declination;
    position.subsolarLongitude = wrap180(rightAscension - siderealTime);
    position.equationOfTime = 4.0 * wrap180(meanLongitude - 0.0057183 - rightAscension);
    return position;
}

bool riseSet(double latitude, double longitude, int32_t localDays, int offsetMinutes, int& riseMinute, int& setMinute) {
    Position noon = at((int64_t)localDays * 1440 + 720 - (int64_t)lround(4.0 * longitude));
    double phi = latitude * degToRad;
    double delta = noon.declination * degToRad;

    double cosHourAngle = (sin(-0.833 * degToRad) - sin(phi) * sin(delta)) / (cos(phi) * cos(delta));
    if (cosHourAngle > 1.0 || cosHourAngle < -1.0) return false;

    double hourAngle = acos(cosHourAngle) * radToDeg;
    double solarNoon = 720.0 + offsetMinutes - 4.0 * longitude - noon.equationOfTime;
    riseMinute = wrapDay(solarNoon - 4.0 * hourAngle);
    setMinute = wrapDay(solarNoon + 4.0 * hourAngle);
    return true;
}
}
