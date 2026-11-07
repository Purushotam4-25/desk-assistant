#pragma once
#include <stdint.h>

enum class WeatherCondition : uint8_t { Clear, MostlyClear, Cloudy, Rain, Fog, Showers, Storm };
enum class UvLevel : uint8_t { Low, Moderate, High, Extreme };

struct ForecastDay {
    WeatherCondition condition = WeatherCondition::Clear;
    int8_t lo = 0;
    int8_t hi = 0;
};

struct WeatherData {
    uint32_t revision = 0;
    WeatherCondition condition = WeatherCondition::Clear;
    bool isDay = true;
    int8_t temperatureC = 0;
    int8_t hi = 0;
    int8_t lo = 0;
    char summary[24] = "";
    ForecastDay forecast[3];
    int8_t outdoorTemperatureC = 0;
    uint8_t outdoorHumidity = 0;
    uint8_t humidity = 0;
    uint8_t uvIndex = 0;
    uint8_t rainPercent = 0;

    UvLevel uvLevel() const {
        if (uvIndex < 3) return UvLevel::Low;
        if (uvIndex < 6) return UvLevel::Moderate;
        if (uvIndex < 8) return UvLevel::High;
        return UvLevel::Extreme;
    }
};
