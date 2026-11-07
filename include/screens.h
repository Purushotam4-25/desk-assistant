#pragma once

#include <stdint.h>

enum class Input { FocusNext, FocusPrev, Select, Menu, Home, SubPagePrev, SubPageNext };

struct NavTarget {
    const char* page;
    uint8_t subPage;
};

class Screen {
public:
    virtual ~Screen() = default;
    virtual void init() = 0;
    virtual void update() = 0;
    virtual void draw() = 0;
    virtual void handleInput(Input input) = 0;
    virtual Screen* takeDetail() { return nullptr; }
    virtual bool takeNavigation(NavTarget& target) { return false; }
    virtual bool usesSideButtons() const { return false; }
};
