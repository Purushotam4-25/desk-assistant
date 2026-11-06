#pragma once
#include <stdint.h>

enum class Icon { Main, Weather, World, Timer, Pet, Calendar, Music, Settings };

namespace Icons {
void draw(Icon icon, int x, int y, uint16_t color, uint16_t cutout);
}
