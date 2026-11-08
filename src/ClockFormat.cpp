#include "ClockFormat.h"
#include <stdio.h>

namespace {
bool isMorning(uint16_t minuteOfDay) { return (minuteOfDay / 60) % 24 < 12; }
int twelveHour(uint16_t minuteOfDay) {
    int hour = (minuteOfDay / 60) % 12;
    return hour == 0 ? 12 : hour;
}
char meridiemLetter(uint16_t minuteOfDay) { return isMorning(minuteOfDay) ? 'a' : 'p'; }
}

namespace ClockFormat {
const char* meridiem(uint16_t minuteOfDay) {
    return isMorning(minuteOfDay) ? "AM" : "PM";
}

void time(char* out, size_t size, uint16_t minuteOfDay, bool use24Hour) {
    if (use24Hour) {
        snprintf(out, size, "%02d:%02d", minuteOfDay / 60, minuteOfDay % 60);
    } else {
        snprintf(out, size, "%d:%02d", twelveHour(minuteOfDay), minuteOfDay % 60);
    }
}

void timeCompact(char* out, size_t size, uint16_t minuteOfDay, bool use24Hour) {
    if (use24Hour) {
        time(out, size, minuteOfDay, true);
    } else {
        snprintf(out, size, "%d:%02d%c", twelveHour(minuteOfDay), minuteOfDay % 60, meridiemLetter(minuteOfDay));
    }
}

void range(char* out, size_t size, uint16_t start, uint16_t end, bool use24Hour) {
    if (use24Hour) {
        snprintf(out, size, "%02d:%02d - %02d:%02d", start / 60, start % 60, end / 60, end % 60);
    } else if (isMorning(start) == isMorning(end)) {
        snprintf(out, size, "%d:%02d - %d:%02d %s", twelveHour(start), start % 60, twelveHour(end), end % 60,
                 meridiem(end));
    } else {
        snprintf(out, size, "%d:%02d%c - %d:%02d%c", twelveHour(start), start % 60, meridiemLetter(start),
                 twelveHour(end), end % 60, meridiemLetter(end));
    }
}

void hourLabel(char* out, size_t size, uint8_t hour, bool use24Hour) {
    if (use24Hour) {
        snprintf(out, size, "%02u", (unsigned)(hour % 24));
    } else {
        snprintf(out, size, "%d%c", twelveHour(hour * 60), meridiemLetter(hour * 60));
    }
}

void date(char* out, size_t size, const DateTime& now, uint8_t format) {
    unsigned shortYear = now.year % 100;
    switch (format) {
        case 1: snprintf(out, size, "%02u/%02u/%02u", now.month, now.day, shortYear); break;
        case 2: snprintf(out, size, "%04u-%02u-%02u", now.year, now.month, now.day); break;
        default: snprintf(out, size, "%02u/%02u/%02u", now.day, now.month, shortYear); break;
    }
}
}
