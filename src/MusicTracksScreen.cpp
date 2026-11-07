#include "MusicTracksScreen.h"
#include <TFT_eSPI.h>
#include "Theme.h"
#include "Fonts.h"
#include "Widgets.h"
#include "MusicWidgets.h"
#include "TimeFormat.h"

extern TFT_eSPI tft;

namespace {
constexpr int coverX = 10, coverY = 24;
constexpr int infoX = 10, infoMaxInk = 90, infoTopInk = 124, infoPitch = 20;
constexpr int rowX = 108, rowW = 190, rowH = 26, rowTop = 24, rowPitch = 30;
constexpr int numX = rowX + 10, titleX = rowX + 38, titleMaxInk = rowW - 38 - 28;
constexpr int barX = 306, barY = rowTop, barH = 6 * rowPitch + rowH;
}

MusicTracksScreen::MusicTracksScreen(MusicPlayer& player, const NavState& nav) : _player(player), _nav(nav) {
    _focus.visible = visibleRows;
    resetKeys();
}

void MusicTracksScreen::resetKeys() {
    _dotsKey = _barKey = -1;
    for (int i = 0; i < visibleRows; i++) _rowKeys[i] = -1;
}

void MusicTracksScreen::init() {
    _full = true;
    _focus.count = _player.trackCount();
    _focus.set(_player.current());
}

void MusicTracksScreen::update() {}

void MusicTracksScreen::handleInput(Input input) {
    if (input == Input::FocusNext) _focus.move(1);
    if (input == Input::FocusPrev) _focus.move(-1);
    if (input == Input::Select && _focus.count > 0) _player.select(_focus.index);
}

void MusicTracksScreen::draw() {
    if (_full) {
        tft.fillScreen(Theme::background);
        resetKeys();
        MusicUi::cover(coverX, coverY);
        drawAlbumInfo();
        _full = false;
    }
    drawDots();
    drawRows();
    drawScrollBar();
}

void MusicTracksScreen::drawDots() {
    int32_t key = _nav.subPage * 16 + _nav.subPageCount;
    if (key == _dotsKey) return;
    _dotsKey = key;
    MusicUi::dots(_nav);
}

void MusicTracksScreen::drawAlbumInfo() {
    const MusicAlbum& album = _player.album();
    if (!album.title) return;

    char length[12];
    formatMinutes(length, sizeof(length), _player.albumSeconds() / 60);
    const char* lines[4] = {album.title, album.artist, album.year, length};
    uint16_t colors[4] = {Theme::text, Theme::textMuted, Theme::textMuted, Theme::textMuted};

    char fitted[40];
    for (int i = 0; i < 4; i++) {
        MusicUi::fit(lines[i], infoMaxInk, fitted, sizeof(fitted));
        Widgets::inkText(fitted, infoX, MusicUi::bodyY(infoTopInk + 7 + i * infoPitch), Fonts::body, 1, colors[i]);
    }
}

void MusicTracksScreen::drawRows() {
    for (int slot = 0; slot < visibleRows; slot++) {
        int track = _focus.first + slot;
        bool exists = track < _focus.count;
        bool selected = exists && track == _focus.index;
        bool playing = exists && track == _player.current() && _player.playing();
        int32_t key = exists ? (track + 1) * 4 + selected * 2 + playing : -2;
        if (key == _rowKeys[slot]) continue;
        _rowKeys[slot] = key;
        drawRow(slot, track, selected, playing);
    }
}

void MusicTracksScreen::drawRow(int slot, int track, bool selected, bool playing) {
    int y = rowTop + slot * rowPitch;
    tft.fillRect(rowX, y, rowW, rowH, Theme::background);
    if (track >= _focus.count) return;

    uint16_t numColor = selected || playing ? Theme::accent : Theme::textDim;
    uint16_t titleColor = playing ? Theme::accent : (selected ? Theme::text : Theme::textMuted);
    if (selected) {
        tft.drawRoundRect(rowX, y, rowW, rowH, 6, Theme::accent);
        tft.fillTriangle(rowX + rowW - 20, y + 7, rowX + rowW - 20, y + 19, rowX + rowW - 10, y + 13, Theme::accent);
    }

    char number[8];
    snprintf(number, sizeof(number), "%02d", track + 1);
    Widgets::inkText(number, numX, MusicUi::bodyY(y + 13), Fonts::body, 1, numColor);

    char title[40];
    MusicUi::fit(_player.track(track).title, titleMaxInk, title, sizeof(title));
    Widgets::inkText(title, titleX, MusicUi::bodyY(y + 13), Fonts::body, 1, titleColor);
}

void MusicTracksScreen::drawScrollBar() {
    if (_focus.count <= visibleRows) return;
    if (_focus.index == _barKey) return;
    _barKey = _focus.index;
    Widgets::scrollBar(barX, barY, barH, _focus.index, _focus.count);
}
