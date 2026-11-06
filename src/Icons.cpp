#include "Icons.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

namespace {
void house(int x, int y, uint16_t c, uint16_t cut) {
    tft.fillTriangle(x + 18, y + 31, x + 32, y + 19, x + 46, y + 31, c);
    tft.fillRect(x + 22, y + 31, 20, 14, c);
    tft.fillRect(x + 29, y + 36, 6, 9, cut);
}

void weather(int x, int y, uint16_t c) {
    tft.fillCircle(x + 25, y + 25, 5, c);
    tft.fillCircle(x + 27, y + 39, 6, c);
    tft.fillCircle(x + 34, y + 35, 8, c);
    tft.fillCircle(x + 40, y + 40, 5, c);
    tft.fillRect(x + 27, y + 39, 13, 6, c);
}

void world(int x, int y, uint16_t c) {
    tft.drawCircle(x + 32, y + 32, 13, c);
    tft.drawCircle(x + 32, y + 32, 12, c);
    tft.drawEllipse(x + 32, y + 32, 6, 13, c);
    tft.drawLine(x + 19, y + 32, x + 45, y + 32, c);
}

void timer(int x, int y, uint16_t c) {
    tft.drawCircle(x + 32, y + 34, 11, c);
    tft.drawCircle(x + 32, y + 34, 10, c);
    tft.fillRect(x + 29, y + 18, 6, 3, c);
    tft.fillRect(x + 31, y + 21, 2, 3, c);
    tft.drawLine(x + 32, y + 34, x + 32, y + 26, c);
    tft.drawLine(x + 33, y + 34, x + 33, y + 26, c);
    tft.drawLine(x + 32, y + 34, x + 37, y + 38, c);
    tft.drawLine(x + 33, y + 34, x + 38, y + 38, c);
}

void pet(int x, int y, uint16_t c, uint16_t cut) {
    tft.fillRoundRect(x + 20, y + 20, 24, 24, 9, c);
    tft.fillRect(x + 26, y + 28, 3, 6, cut);
    tft.fillRect(x + 35, y + 28, 3, 6, cut);
    tft.drawLine(x + 29, y + 37, x + 35, y + 37, cut);
    tft.drawLine(x + 29, y + 38, x + 35, y + 38, cut);
}

void calendar(int x, int y, uint16_t c) {
    tft.drawRoundRect(x + 19, y + 22, 26, 23, 3, c);
    tft.drawRoundRect(x + 20, y + 23, 24, 21, 2, c);
    tft.fillRect(x + 19, y + 22, 26, 8, c);
    tft.fillRect(x + 25, y + 18, 3, 7, c);
    tft.fillRect(x + 36, y + 18, 3, 7, c);
    tft.fillRect(x + 24, y + 34, 4, 3, c);
    tft.fillRect(x + 30, y + 34, 4, 3, c);
    tft.fillRect(x + 36, y + 34, 4, 3, c);
    tft.fillRect(x + 24, y + 39, 4, 3, c);
    tft.fillRect(x + 30, y + 39, 4, 3, c);
}

void music(int x, int y, uint16_t c) {
    tft.fillCircle(x + 25, y + 40, 4, c);
    tft.fillCircle(x + 38, y + 37, 4, c);
    tft.fillRect(x + 27, y + 24, 2, 17, c);
    tft.fillRect(x + 40, y + 21, 2, 17, c);
    tft.fillTriangle(x + 27, y + 24, x + 42, y + 21, x + 42, y + 26, c);
    tft.fillTriangle(x + 27, y + 24, x + 42, y + 26, x + 27, y + 29, c);
}

void settings(int x, int y, uint16_t c, uint16_t cut) {
    tft.fillCircle(x + 32, y + 32, 9, c);
    tft.fillCircle(x + 42, y + 32, 3, c);
    tft.fillCircle(x + 39, y + 39, 3, c);
    tft.fillCircle(x + 32, y + 42, 3, c);
    tft.fillCircle(x + 25, y + 39, 3, c);
    tft.fillCircle(x + 22, y + 32, 3, c);
    tft.fillCircle(x + 25, y + 25, 3, c);
    tft.fillCircle(x + 32, y + 22, 3, c);
    tft.fillCircle(x + 39, y + 25, 3, c);
    tft.fillCircle(x + 32, y + 32, 4, cut);
}
}

namespace Icons {
void draw(Icon icon, int x, int y, uint16_t color, uint16_t cutout) {
    switch (icon) {
        case Icon::Main:     house(x, y, color, cutout); break;
        case Icon::Weather:  weather(x, y, color); break;
        case Icon::World:    world(x, y, color); break;
        case Icon::Timer:    timer(x, y, color); break;
        case Icon::Pet:      pet(x, y, color, cutout); break;
        case Icon::Calendar: calendar(x, y, color); break;
        case Icon::Music:    music(x, y, color); break;
        case Icon::Settings: settings(x, y, color, cutout); break;
    }
}
}
