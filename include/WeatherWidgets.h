#pragma once
#include <stdint.h>
#include "WeatherData.h"
#include "Fonts.h"

namespace WeatherUi {
void icon(WeatherCondition condition, bool isDay, int x, int y, int size, uint16_t background);
int iconWidth(WeatherCondition condition, bool isDay, int size);
void degree(int x, int y, int radius, uint16_t color);
int yForDigitTop(const GFXfont* font, uint8_t scale, int digitTop);
int digitHeight(const GFXfont* font, uint8_t scale);
uint16_t uvColor(UvLevel level);
extern const char* const weekdayShort[7];
}
