#pragma once

enum class Input { FocusNext, FocusPrev, Select, Menu, Home, SubPagePrev, SubPageNext };

class Screen {
public:
    virtual ~Screen() = default;
    virtual void init() = 0;
    virtual void update() = 0;
    virtual void draw() = 0;
    virtual void handleInput(Input input) = 0;
    virtual Screen* takeDetail() { return nullptr; }
};
