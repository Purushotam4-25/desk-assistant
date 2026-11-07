#pragma once
#include <TFT_eSPI.h>
#include "NavState.h"

namespace MusicUi {
constexpr int dotsY = 11;
constexpr int coverSize = 88;
constexpr int pillX = 112, pillW = 96, pillH = 22;

void dots(const NavState& nav);
void cover(int x, int y);
void pill(int y, const char* label, bool focused);
void fit(const char* s, int maxInk, char* out, size_t size);
int bodyY(int capCentre);
}
