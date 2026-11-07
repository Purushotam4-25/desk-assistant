#pragma once
#include <stdint.h>

namespace SunCalc {
struct Position {
    double declination;
    double subsolarLongitude;
    double equationOfTime;
};

Position at(int64_t utcMinutesSinceEpoch);
bool riseSet(double latitude, double longitude, int32_t localDays, int offsetMinutes, int& riseMinute, int& setMinute);
bool isDaylight(double latitude, double longitude, int32_t localDays, int offsetMinutes, int minuteOfDay);
}
