#pragma once
#include <stdint.h>
#include "screens.h"
#include "Icons.h"

struct Page {
    const char* name;
    Icon icon;
    Screen* const* subPages;
    uint8_t count;
};
