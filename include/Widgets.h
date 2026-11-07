#pragma once
#include <TFT_eSPI.h>
#include "DeviceStatus.h"

namespace Widgets {
struct Ink {
    int left;
    int width;
};

struct Rows {
    int top;
    int height;
};

void text(const char* s, int x, int y, const GFXfont* font, uint8_t size, uint16_t color);
void fitText(const char* s, int x, int y, int maxWidth, const GFXfont* font, uint8_t size, uint16_t color);
int textWidth(const char* s, const GFXfont* font, uint8_t size);
Ink ink(const char* s, const GFXfont* font, uint8_t size);
int inkWidth(const char* s, const GFXfont* font, uint8_t size);
Rows inkRows(const char* s, const GFXfont* font, uint8_t size);
void inkText(const char* s, int inkX, int y, const GFXfont* font, uint8_t size, uint16_t color);
void inkTextCentered(const char* s, int centerX, int y, const GFXfont* font, uint8_t size, uint16_t color);
void scrollBar(int x, int y, int height, int index, int count);
void card(int x, int y, int w, int h);
void button(int x, int y, int w, int h, const char* label, bool focused);
void statusBar(const DeviceStatus& status, int activePage, int pageCount);
}
