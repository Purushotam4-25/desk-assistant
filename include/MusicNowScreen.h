#pragma once
#include "screens.h"
#include "NavState.h"
#include "MusicPlayer.h"

class MusicNowScreen : public Screen {
public:
    MusicNowScreen(MusicPlayer& player, const NavState& nav, Screen& lyricsScreen);

    void init() override;
    void update() override;
    void draw() override;
    void handleInput(Input input) override;
    Screen* takeDetail() override;

private:
    enum Item : uint8_t { Prev, Play, Next, Lyrics, itemCount };

    MusicPlayer& _player;
    const NavState& _nav;
    Screen& _lyricsScreen;
    Item _focused = Play;
    bool _full = true;
    bool _openLyrics = false;
    bool _returning = false;

    int32_t _dotsKey, _infoKey, _barKey, _controlsKey, _timesKey, _pillKey, _coverKey;

    void resetKeys();
    void activate();
    void drawDots();
    void drawCover();
    void drawInfo();
    void drawProgress();
    void drawControls();
    void drawTimes();
    void drawPill();
};
