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

GlyphRun run(const GFXfont* font, char c) {
    const GFXglyph& g = font->glyph[c - font->first];
    return {g.xOffset, g.width, g.xAdvance};
}

int capCentre(const GFXfont* font, uint8_t scale, char c) {
    GlyphBox box = glyph(font, c);
    return (ascent(font) + box.yOffset) * scale + box.height * scale / 2;
}
}
