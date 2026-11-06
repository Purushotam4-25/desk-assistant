#pragma once
#include <stdint.h>

struct City {
    const char* name;
    float latitude;
    float longitude;
    int16_t offsetMinutes;
};

constexpr City worldCities[] = {
    {"Mumbai", 19.08f, 72.88f, 330},
    {"London", 51.5f, -0.1f, 0},
    {"New York", 40.7f, -74.0f, -300},
    {"Tokyo", 35.7f, 139.7f, 540},
    {"Brisbane", -27.47f, 153.03f, 600},
};

constexpr int worldCityCount = sizeof(worldCities) / sizeof(worldCities[0]);
constexpr int localCity = 0;
constexpr int localOffsetMinutes = worldCities[localCity].offsetMinutes;
