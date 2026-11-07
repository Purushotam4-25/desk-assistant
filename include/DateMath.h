#pragma once
#include <stdint.h>

namespace Date {
inline int32_t daysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yearOfEra = year - era * 400;
    const unsigned dayOfYear = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
    const unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return era * 146097 + (int32_t)dayOfEra - 719468;
}

inline void civilFromDays(int32_t days, int& year, unsigned& month, unsigned& day) {
    days += 719468;
    const int era = (days >= 0 ? days : days - 146096) / 146097;
    const unsigned dayOfEra = days - era * 146097;
    const unsigned yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
    const unsigned dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
    const unsigned shiftedMonth = (5 * dayOfYear + 2) / 153;
    day = dayOfYear - (153 * shiftedMonth + 2) / 5 + 1;
    month = shiftedMonth < 10 ? shiftedMonth + 3 : shiftedMonth - 9;
    year = yearOfEra + era * 400 + (month <= 2);
}

inline int mondayIndex(int32_t days) {
    return (int)(((days + 3) % 7 + 7) % 7);
}

inline int isoWeek(int year, unsigned month, unsigned day) {
    int32_t days = daysFromCivil(year, month, day);
    int32_t thursday = days - mondayIndex(days) + 3;
    int thursdayYear;
    unsigned thursdayMonth, thursdayDay;
    civilFromDays(thursday, thursdayYear, thursdayMonth, thursdayDay);
    return (int)((thursday - daysFromCivil(thursdayYear, 1, 1)) / 7 + 1);
}
}
