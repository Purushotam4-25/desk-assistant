#pragma once
#include <stddef.h>
#include <stdint.h>
#include "RTCManager.h"

namespace ClockFormat {
void time(char* out, size_t size, uint16_t minuteOfDay, bool use24Hour);
void timeCompact(char* out, size_t size, uint16_t minuteOfDay, bool use24Hour);
void range(char* out, size_t size, uint16_t start, uint16_t end, bool use24Hour);
void hourLabel(char* out, size_t size, uint8_t hour, bool use24Hour);
void date(char* out, size_t size, const DateTime& now, uint8_t format);
const char* meridiem(uint16_t minuteOfDay);
}
