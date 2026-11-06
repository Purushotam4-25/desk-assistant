#pragma once
#include "screens.h"
#include "Page.h"
#include "ListFocus.h"

class ScreensMenu : public Screen {
public:
    ScreensMenu(const Page* pages, uint8_t pageCount);

    void open(uint8_t currentPage);
    uint8_t focusedPage() const;

    void init() override;
    void update() override;
    void draw() override;
    void handleInput(Input input) override;

private:
    static constexpr int visibleRows = 3;

    const Page* _pages;
    ListFocus _focus;
    bool _full = true;

    int16_t _rowKeys[visibleRows];
    int16_t _barKey = -1;

    void resetKeys();
    void drawRows();
    void drawRow(int slot, int page, bool selected);
    void drawScrollBar();
};
