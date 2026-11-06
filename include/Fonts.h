#pragma once
#include <TFT_eSPI.h>

namespace Fonts {
extern const GFXfont* const title;
extern const GFXfont* const body;
extern const GFXfont* const tiny;
constexpr const GFXfont* glcd = nullptr;

struct GlyphBox {
    int8_t yOffset;
    uint8_t height;
};

struct GlyphRun {
    int8_t xOffset;
    uint8_t width;
    uint8_t advance;
};

struct GlyphRows {
    int top;
    int bottom;
};

int ascent(const GFXfont* font);
GlyphBox glyph(const GFXfont* font, char c);
GlyphRun run(const GFXfont* font, char c);
GlyphRun glcdRun(char c);
GlyphRows rows(const GFXfont* font, char c);
int capCentre(const GFXfont* font, uint8_t scale, char c);
}
