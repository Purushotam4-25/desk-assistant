#pragma once
#include "SunCalc.h"

namespace WorldMap {
constexpr int top = 8;
constexpr int bottom = 135;

void project(float latitude, float longitude, int& x, int& y);
void computeNight(const SunCalc::Position& sun);
void drawLand(int x0, int y0, int x1, int y1);
void drawTerminator();
}
