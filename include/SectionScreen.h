#pragma once
#include "screens.h"
#include "NavState.h"

class SectionScreen : public Screen {
public:
    explicit SectionScreen(const NavState& nav);

    void init() override;
    void update() override;
    void draw() override;
    void handleInput(Input input) override;

protected:
    virtual void resetKeys() = 0;
    virtual void drawFrame() {}
    virtual void drawRegions() = 0;

private:
    const NavState& _nav;
    bool _full = true;
    int16_t _dotsKey = -1;

    void drawDots();
};
