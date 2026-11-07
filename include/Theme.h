#pragma once
#include <stdint.h>

constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

namespace Theme {
constexpr uint16_t background = rgb565(0, 0, 0);
constexpr uint16_t surface    = rgb565(22, 24, 32);
constexpr uint16_t outline    = rgb565(70, 74, 88);

constexpr uint16_t text       = rgb565(255, 255, 255);
constexpr uint16_t textMuted  = rgb565(140, 144, 156);
constexpr uint16_t textDim    = rgb565(80, 84, 96);

constexpr uint16_t accent     = 0xF9A0;
constexpr uint16_t highlight  = rgb565(38, 10, 0);
constexpr uint16_t eventNow   = rgb565(104, 22, 0);
constexpr uint16_t gridLine   = rgb565(44, 46, 56);
constexpr uint16_t barPast    = rgb565(96, 100, 112);
constexpr uint16_t good       = rgb565(40, 235, 90);
constexpr uint16_t alert      = rgb565(255, 36, 36);
constexpr uint16_t plug       = rgb565(180, 184, 196);
constexpr uint16_t warn       = rgb565(255, 232, 0);

constexpr uint16_t sun        = rgb565(255, 232, 0);
constexpr uint16_t cloud      = rgb565(215, 225, 240);
constexpr uint16_t cloudDark  = rgb565(150, 165, 190);
constexpr uint16_t rain       = rgb565(40, 160, 255);

constexpr uint16_t uvLow      = rgb565(0, 230, 80);
constexpr uint16_t uvModerate = rgb565(255, 232, 0);
constexpr uint16_t uvHigh     = accent;
constexpr uint16_t uvExtreme  = alert;

constexpr uint16_t mapDot     = rgb565(136, 138, 142);
constexpr uint16_t mapNight   = rgb565(48, 52, 64);
constexpr uint16_t terminator = rgb565(205, 214, 230);
}
