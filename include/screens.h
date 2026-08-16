#pragma once
#include "rot_encoder.h"

enum class RotaryInput { ROTATE_CW, ROTATE_CCW, CLICK, DOUBLE_CLICK, LONG_PRESS };

class Screen {
public:
    virtual ~Screen() = default;
    virtual void init() = 0;
    virtual void update() = 0;
    virtual void draw() = 0;
    virtual void handleInput(RotaryInput input) = 0;
};