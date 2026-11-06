#pragma once
#include <stdint.h>

namespace TimerUi {
constexpr int readoutCenterY = 95;
constexpr int buttonY = 139, timerButtonY = 155, buttonW = 112, buttonH = 34, buttonGap = 16;

struct Readout {
    int scale = 4;
    int groupWidth = 0, colonGap = 0, x = 0, width = 0;
    int digitTop = 0, digitHeight = 0, textY = 0;
    int inkLeft = 0, inkWidth = 0;
};

Readout makeReadout();
void drawGroup(const Readout& readout, int index, unsigned value, uint16_t color);
void drawColon(const Readout& readout, uint16_t color);
void drawUnderline(const Readout& readout, int field, int level);
int buttonX(int count, int index);
}
