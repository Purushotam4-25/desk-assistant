#pragma once
#include <stdio.h>
#include <stdint.h>

inline void formatMinutes(char* out, size_t size, uint32_t minutes) {
    if (minutes >= 60) {
        snprintf(out, size, "%luh %02lum", (unsigned long)(minutes / 60), (unsigned long)(minutes % 60));
    } else {
        snprintf(out, size, "%lum", (unsigned long)minutes);
    }
}
