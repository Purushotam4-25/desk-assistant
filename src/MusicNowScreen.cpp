#include "MusicNowScreen.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "MusicWidgets.h"
#include "ListFocus.h"

extern TFT_eSPI tft;

namespace {
constexpr int ruleY = 19;
constexpr int coverX = 116, coverY = 24;
constexpr int titleCentre = 128, artistCentre = 148, textMaxInk = 280;
constexpr int barX = 10, barW = 300, barTop = 163, barTrackY = 166, barBlockH = 11, scrubTravel = 290;
constexpr int controlsX = 160, controlsY = 191;
constexpr int timesY = controlsY;
constexpr int pillY = 208;
constexpr int edge = 10, rightEdge = 310;

void formatTime(char* out, size_t size, uint32_t seconds) {
    snprintf(out, size, "%lu:%02lu", (unsigned long)(seconds / 60), (unsigned long)(seconds % 60));
}
}

MusicNowScreen::MusicNowScreen(MusicPlayer& player, const NavState& nav, Screen& lyricsScreen)
    : _player(player), _nav(nav), _lyricsScreen(lyricsScreen) {
    resetKeys();
}

void MusicNowScreen::resetKeys() {
    _dotsKey = _infoKey = _barKey = _controlsKey = _timesKey = _pillKey = -1;
}

void MusicNowScreen::init() {
    _full = true;
    if (!_returning) _focused = Play;
    _returning = false;
}

void MusicNowScreen::update() {}

Screen* MusicNowScreen::takeDetail() {
    if (!_openLyrics) return nullptr;
    _openLyrics = false;
    _returning = true;
    return &_lyricsScreen;
}

void MusicNowScreen::activate() {
    switch (_focused) {
        case Prev: _player.previous(); break;
        case Play: _player.togglePlay(); break;
        case Next: _player.next(); break;
        case Lyrics: _openLyrics = true; break;
        default: break;
    }
}

void MusicNowScreen::handleInput(Input input) {
    if (input == Input::FocusNext) _focused = (Item)stepIndex(_focused, 1, itemCount);
    if (input == Input::FocusPrev) _focused = (Item)stepIndex(_focused, -1, itemCount);
    if (input == Input::Select) activate();
}

void MusicNowScreen::draw() {
    if (_full) {
        tft.fillScreen(Theme::background);
        resetKeys();
        tft.drawFastHLine(60, ruleY, 200, Theme::outline);
        MusicUi::cover(coverX, coverY);
        _full = false;
    }
    drawDots();
    drawInfo();
    drawProgress();
    drawControls();
    drawTimes();
    drawPill();
}

void MusicNowScreen::drawDots() {
    int32_t key = _nav.subPage * 16 + _nav.subPageCount;
    if (key == _dotsKey) return;
    _dotsKey = key;
    MusicUi::dots(_nav);
}

void MusicNowScreen::drawInfo() {
    int32_t key = _player.current();
    if (key == _infoKey) return;
    _infoKey = key;

    tft.fillRect(0, 117, 320, 44, Theme::background);
    if (_player.trackCount() == 0) return;

    char line[40];
    MusicUi::fit(_player.currentTrack().title, textMaxInk, line, sizeof(line));
    Widgets::inkTextCentered(line, 160, MusicUi::bodyY(titleCentre), Fonts::body, 1, Theme::text);
    MusicUi::fit(_player.album().artist, textMaxInk, line, sizeof(line));
    Widgets::inkTextCentered(line, 160, MusicUi::bodyY(artistCentre), Fonts::body, 1, Theme::textMuted);
}

void MusicNowScreen::drawProgress() {
    uint32_t length = _player.trackCount() ? _player.currentTrack().seconds : 0;
    int position = length ? _player.elapsedSeconds() * scrubTravel / length : 0;
    int32_t key = _player.current() * 1000 + position;
    if (key == _barKey) return;
    _barKey = key;

    tft.fillRect(barX, barTop, barW + 1, barBlockH, Theme::background);
    tft.fillRect(barX, barTrackY, barW, 4, Theme::outline);
    tft.fillRect(barX, barTrackY, 5 + position, 4, Theme::accent);
    tft.fillCircle(barX + 5 + position, barTop + 5, 5, Theme::text);
}

void MusicNowScreen::drawControls() {
    int32_t key = (int32_t)(_player.revision() & 0xFFFFF) * 8 + _focused;
    if (key == _controlsKey) return;
    _controlsKey = key;

    int cx = controlsX, cy = controlsY;
    tft.fillRect(cx - 52, cy - 11, 104, 23, Theme::background);

    uint16_t prevColor = _focused == Prev ? Theme::accent : Theme::textMuted;
    int px = cx - 48;
    tft.fillRect(px, cy - 7, 2, 15, prevColor);
    tft.fillTriangle(px + 14, cy - 7, px + 14, cy + 7, px + 3, cy, prevColor);

    uint16_t playColor = _focused == Play ? Theme::accent : Theme::text;
    if (_player.playing()) {
        tft.fillRect(cx - 8, cy - 9, 6, 19, playColor);
        tft.fillRect(cx + 3, cy - 9, 6, 19, playColor);
    } else {
        tft.fillTriangle(cx - 7, cy - 9, cx - 7, cy + 9, cx + 9, cy, playColor);
    }

    uint16_t nextColor = _focused == Next ? Theme::accent : Theme::textMuted;
    int nx = cx + 34;
    tft.fillTriangle(nx, cy - 7, nx, cy + 7, nx + 11, cy, nextColor);
    tft.fillRect(nx + 13, cy - 7, 2, 15, nextColor);
}

void MusicNowScreen::drawTimes() {
    uint32_t elapsed = _player.elapsedSeconds();
    int32_t key = (_player.current() << 16) | (int32_t)elapsed;
    if (key == _timesKey) return;
    _timesKey = key;

    int y = MusicUi::bodyY(timesY);
    char text[12];

    tft.fillRect(edge - 2, timesY - 11, 66, 23, Theme::background);
    formatTime(text, sizeof(text), elapsed);
    Widgets::inkText(text, edge, y, Fonts::body, 1, Theme::textMuted);

    tft.fillRect(rightEdge - 64, timesY - 11, 66, 23, Theme::background);
    formatTime(text, sizeof(text), _player.trackCount() ? _player.currentTrack().seconds : 0);
    Widgets::inkText(text, rightEdge - Widgets::inkWidth(text, Fonts::body, 1), y, Fonts::body, 1, Theme::textMuted);
}

void MusicNowScreen::drawPill() {
    int32_t key = _focused == Lyrics;
    if (key == _pillKey) return;
    _pillKey = key;
    MusicUi::pill(pillY, "Lyrics", key);
}
