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
    int32_t key = _player.trackRevision() & 0xFFFFF;
    if (key == _headerKey) return;
    _headerKey = key;

    tft.fillRect(0, 2, 146, 20, Theme::background);
    tft.fillRect(174, 2, 146, 20, Theme::background);
    if (!_player.ready()) return;

    int y = MusicUi::bodyY(MusicUi::dotsY);
    char text[40];
    MusicUi::fit(_player.title(), headerMaxInk, text, sizeof(text));
    Widgets::inkText(text, edge, y, Fonts::body, 1, Theme::text);
    MusicUi::fit(_player.artist(), headerMaxInk, text, sizeof(text));
    Widgets::inkText(text, rightEdge - Widgets::inkWidth(text, Fonts::body, 1), y, Fonts::body, 1, Theme::textMuted);
}

void MusicLyricsScreen::drawBlocks() {
    const char* message = nullptr;
    if (!_player.ready()) {
        message = _player.stateText();
    } else if (_player.lyricsState() == MusicPlayer::LyricsState::Failed) {
        message = "Can't load lyrics";
    } else if (_player.lyricsState() == MusicPlayer::LyricsState::Pending || !_player.lyricsLoaded()) {
        message = "...";
    } else if (_player.lyricCount() == 0) {
        message = "No lyrics";
    }
    int count = message ? 0 : _player.lyricCount();
    int now = message ? -1 : _player.lyricIndex();
    int32_t base = (int32_t)(_player.lyricsRevision() & 0xFFFF) * 256;

    for (int slot = 0; slot < blockCount; slot++) {
        int lyric = now - 1 + slot;
        bool exists = lyric >= 0 && lyric < count;
        int32_t key = (base + (message ? 255 : lyric + 2)) * 4 + slot;
        if (!exists && slot != 1) key = -2 - slot;
        if (key == _blockKeys[slot]) continue;
        _blockKeys[slot] = key;
        drawBlock(slot, exists ? lyric : -1, message);
    }
}

void MusicLyricsScreen::drawBlock(int slot, int lyric, const char* message) {
    int y = blockTop + slot * blockPitch;
    tft.fillRect(blockX, y, blockW, blockH, Theme::background);

    const char* line1 = nullptr;
    const char* line2 = "";
    uint16_t color = slot == 0 ? Theme::textDim : Theme::textMuted;
    if (lyric >= 0) {
        const MusicLyric& block = _player.lyric(lyric);
        line1 = block.line1;
        line2 = block.line2;
    } else if (slot == 1) {
        line1 = message ? message : "...";
        color = Theme::textDim;
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
