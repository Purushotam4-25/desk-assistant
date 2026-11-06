#include "SectionScreen.h"
#include <TFT_eSPI.h>
#include "Theme.h"

extern TFT_eSPI tft;

namespace {
constexpr int dotsCenterX = 160, dotsY = 224, dotSpacing = 14, dotRadius = 3;
}

SectionScreen::SectionScreen(const NavState& nav) : _nav(nav) {}

void SectionScreen::init() {
    _full = true;
}

void SectionScreen::update() {}

void SectionScreen::handleInput(Input input) {}

void SectionScreen::draw() {
    if (_full) {
        tft.fillScreen(Theme::background);
        resetKeys();
        _dotsKey = -1;
        drawFrame();
        _full = false;
    }
    drawDots();
    drawRegions();
}

void SectionScreen::drawDots() {
    int16_t key = _nav.subPage * 16 + _nav.subPageCount;
    if (key == _dotsKey) return;
    _dotsKey = key;

    tft.fillRect(dotsCenterX - 60, dotsY - 8, 120, 16, Theme::background);
    int x0 = dotsCenterX - ((_nav.subPageCount - 1) * dotSpacing) / 2;
    for (int i = 0; i < _nav.subPageCount; i++) {
        tft.fillCircle(x0 + i * dotSpacing, dotsY, dotRadius, i == _nav.subPage ? Theme::accent : Theme::outline);
    }
}
