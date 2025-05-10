#ifndef OLED_U8G2_H
#define OLED_U8G2_H

#include <U8g2lib.h>

void setupOLED();
void printLineOLED(const String& text, int x, int y, int size);
void clearOLED();
void updateOLED();

#endif