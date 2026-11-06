#pragma once
#include "screens.h"
#include "Page.h"
#include "NavState.h"
#include "InputRouter.h"
#include "ScreensMenu.h"

class ScreenManager {
public:
    ScreenManager(InputRouter& input, NavState& nav, const Page* pages, uint8_t pageCount, ScreensMenu& menu);
    void begin();
    void update();
    void draw();

private:
    InputRouter& _input;
    NavState& _nav;
    const Page* _pages;
    uint8_t _pageCount;
    ScreensMenu& _menu;
    uint8_t _page = 0;
    uint8_t _subPage = 0;
    bool _menuOpen = false;
    Screen* _detail = nullptr;

    Screen* active();
    void show(uint8_t page, uint8_t subPage);
    void closeDetail();
    void dispatch(Input input);
    void stepSubPage(int step);
};
