#include "Fonts.h"
#include <Adafruit_GFX.h>
#include "Org_01.h"
#include "Pixellari_16.h"
#include "VHS_Gothic_16.h"

namespace Fonts {
const GFXfont* const title = &Pixellari_16;
const GFXfont* const body = &VHS_Gothic_16;
const GFXfont* const tiny = &Org_01;

// Same rule TFT_eSPI uses to place the baseline of a free font below the draw y.
int ascent(const GFXfont* font) {
    int best = 0;
    int numChars = font->last - font->first;
    for (int i = 0; i < numChars; i++) {
        int above = -font->glyph[i].yOffset;
        if (above > best) best = above;
    }
    return best;
}

GlyphBox glyph(const GFXfont* font, char c) {
    const GFXglyph& g = font->glyph[c - font->first];
    return {g.yOffset, g.height};
}

// The bitmaps carry blank columns (digits are 12 wide but draw 10; "1" draws 6), so the real ink is found by scanning.
GlyphRun run(const GFXfont* font, char c) {
    const GFXglyph& g = font->glyph[c - font->first];
    const uint8_t* bits = font->bitmap + g.bitmapOffset;
    int first = -1, last = -1;
    for (int x = 0; x < g.width; x++) {
        for (int y = 0; y < g.height; y++) {
            int index = y * g.width + x;
            if (bits[index >> 3] & (0x80 >> (index & 7))) {
                if (first < 0) first = x;
                last = x;
                break;
            }
        }
    }
    if (first < 0) return {g.xOffset, 0, g.xAdvance};
    return {(int8_t)(g.xOffset + first), (uint8_t)(last - first + 1), g.xAdvance};
}

// Packed (first ink column << 4) | ink width for the 5x7 font, ASCII 0x20 to 0x7E.
const uint8_t glcdInk[] = {
    0x00, 0x21, 0x13, 0x05, 0x05, 0x05, 0x05, 0x13, 0x13, 0x13, 0x05, 0x05, 0x13, 0x05, 0x22, 0x05, 0x05, 0x13, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x21, 0x12, 0x14, 0x05, 0x14, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x13, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x14, 0x05, 0x14, 0x05, 0x05, 0x13, 0x05, 0x05, 0x05, 0x05, 0x05, 0x14, 0x05, 0x05, 0x13, 0x04, 0x04,
    0x13, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x13, 0x21, 0x13, 0x05,
};
static_assert(sizeof(glcdInk) == 95, "glcdInk covers 0x20 to 0x7E");

GlyphRun glcdRun(char c) {
    if (c < 0x20 || c > 0x7E) return {0, 0, 6};
    uint8_t packed = glcdInk[c - 0x20];
    return {(int8_t)(packed >> 4), (uint8_t)(packed & 0x0F), 6};
}

GlyphRows rows(const GFXfont* font, char c) {
    const GFXglyph& g = font->glyph[c - font->first];
    const uint8_t* bits = font->bitmap + g.bitmapOffset;
    int first = -1, last = -1;
    for (int y = 0; y < g.height; y++) {
        for (int x = 0; x < g.width; x++) {
            int index = y * g.width + x;
            if (bits[index >> 3] & (0x80 >> (index & 7))) {
                if (first < 0) first = y;
                last = y;
                break;
            }
        }
    }
    if (first < 0) return {0, 0};
    int base = ascent(font) + g.yOffset;
    return {base + first, base + last + 1};
}

int capCentre(const GFXfont* font, uint8_t scale, char c) {
    GlyphBox box = glyph(font, c);
    return (ascent(font) + box.yOffset) * scale + box.height * scale / 2;
}
}
