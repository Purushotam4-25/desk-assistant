#include "MusicLyricsScreen.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "MusicWidgets.h"

extern TFT_eSPI tft;

namespace {
constexpr int edge = 10, rightEdge = 310;
constexpr int headerMaxInk = 130;
constexpr int blockX = 10, blockW = 300, blockH = 52, blockTop = 28, blockPitch = 60;
constexpr int textInset = 14, textMaxInk = blockW - textInset - 10;
constexpr int line1Centre = 16, line2Centre = 36;
constexpr int pillY = 208;
}

MusicLyricsScreen::MusicLyricsScreen(const MusicPlayer& player, const NavState& nav) : _player(player), _nav(nav) {
    resetKeys();
}

void MusicLyricsScreen::resetKeys() {
    _dotsKey = _headerKey = -1;
    for (int i = 0; i < blockCount; i++) _blockKeys[i] = -1;
}

void MusicLyricsScreen::init() {
    _full = true;
}

void MusicLyricsScreen::update() {}

void MusicLyricsScreen::handleInput(Input input) {}

void MusicLyricsScreen::draw() {
    if (_full) {
        tft.fillScreen(Theme::background);
        resetKeys();
        MusicUi::pill(pillY, "Back", true);
        _full = false;
    }
    drawDots();
    drawHeader();
    drawBlocks();
}

void MusicLyricsScreen::drawDots() {
    int32_t key = _nav.subPage * 16 + _nav.subPageCount;
    if (key == _dotsKey) return;
    _dotsKey = key;
    MusicUi::dots(_nav);
}

void MusicLyricsScreen::drawHeader() {
    int32_t key = _player.current();
    if (key == _headerKey) return;
    _headerKey = key;

    tft.fillRect(0, 2, 146, 20, Theme::background);
    tft.fillRect(174, 2, 146, 20, Theme::background);
    if (_player.trackCount() == 0) return;

    int y = MusicUi::bodyY(MusicUi::dotsY);
    char text[40];
    MusicUi::fit(_player.currentTrack().title, headerMaxInk, text, sizeof(text));
    Widgets::inkText(text, edge, y, Fonts::body, 1, Theme::text);
    MusicUi::fit(_player.album().artist, headerMaxInk, text, sizeof(text));
    Widgets::inkText(text, rightEdge - Widgets::inkWidth(text, Fonts::body, 1), y, Fonts::body, 1, Theme::textMuted);
}

void MusicLyricsScreen::drawBlocks() {
    bool noLyrics = _player.trackCount() == 0 || _player.currentTrack().lyricCount == 0;
    int now = noLyrics ? -1 : _player.lyricIndex();
    int count = noLyrics ? 0 : _player.currentTrack().lyricCount;

    for (int slot = 0; slot < blockCount; slot++) {
        int lyric = now - 1 + slot;
        bool exists = lyric >= 0 && lyric < count;
        int32_t key = (_player.current() * 100 + (noLyrics ? 99 : lyric + 2)) * 4 + slot;
        if (!exists && slot != 1) key = -2 - slot;
        if (key == _blockKeys[slot]) continue;
        _blockKeys[slot] = key;
        drawBlock(slot, exists ? lyric : -1, noLyrics);
    }
}

void MusicLyricsScreen::drawBlock(int slot, int lyric, bool noLyrics) {
    int y = blockTop + slot * blockPitch;
    tft.fillRect(blockX, y, blockW, blockH, Theme::background);

    const char* line1 = nullptr;
    const char* line2 = "";
    uint16_t color = slot == 0 ? Theme::textDim : Theme::textMuted;
    if (lyric >= 0) {
        const MusicLyric& block = _player.currentTrack().lyrics[lyric];
        line1 = block.line1;
        line2 = block.line2;
    } else if (slot == 1) {
        line1 = noLyrics ? "No lyrics" : "...";
    } else {
        return;
    }

    if (slot == 1) {
        tft.fillRoundRect(blockX, y, blockW, blockH, 6, Theme::surface);
        tft.fillRect(blockX, y + 10, 4, 32, Theme::accent);
        if (lyric >= 0) color = Theme::text;
    }

    char text[40];
    MusicUi::fit(line1, textMaxInk, text, sizeof(text));
    Widgets::inkText(text, blockX + textInset, MusicUi::bodyY(y + line1Centre), Fonts::body, 1, color);
    MusicUi::fit(line2, textMaxInk, text, sizeof(text));
    Widgets::inkText(text, blockX + textInset, MusicUi::bodyY(y + line2Centre), Fonts::body, 1, color);
}
