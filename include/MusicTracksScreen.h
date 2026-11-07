#pragma once
#include "screens.h"
#include "NavState.h"
#include "MusicPlayer.h"
#include "ListFocus.h"

class MusicTracksScreen : public Screen {
public:
    MusicTracksScreen(MusicPlayer& player, const NavState& nav);

    void init() override;
    void update() override;
    void draw() override;
    void handleInput(Input input) override;

private:
    static constexpr int visibleRows = 7;

    MusicPlayer& _player;
    const NavState& _nav;
    ListFocus _focus;
    bool _full = true;

    int32_t _dotsKey, _barKey;
    int32_t _rowKeys[visibleRows];

    void resetKeys();
    void drawDots();
    void drawAlbumInfo();
    void drawRows();
    void drawRow(int slot, int track, bool selected, bool playing);
    void drawScrollBar();
};
