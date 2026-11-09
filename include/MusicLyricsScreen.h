#pragma once
#include "screens.h"
#include "NavState.h"
#include "MusicPlayer.h"

class MusicLyricsScreen : public Screen {
public:
    MusicLyricsScreen(const MusicPlayer& player, const NavState& nav);

    void init() override;
    void update() override;
    void draw() override;
    void handleInput(Input input) override;

private:
    static constexpr int blockCount = 3;

    const MusicPlayer& _player;
    const NavState& _nav;
    bool _full = true;

    int32_t _dotsKey, _headerKey;
    int32_t _blockKeys[blockCount];

    void resetKeys();
    void drawDots();
    void drawHeader();
    void drawBlocks();
    void drawBlock(int slot, int lyric, const char* message);
};
