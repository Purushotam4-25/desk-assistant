#pragma once
#include "screens.h"
#include "rot_encoder.h"
#include "TapButton.h"

class InputRouter {
public:
    InputRouter(RotaryEncoderManager& encoder, TapButton& left, TapButton& right);
    void begin();
    void update();
    bool next(Input& out);

private:
    static constexpr uint8_t queueSize = 8;
    static InputRouter* _instance;

    RotaryEncoderManager& _encoder;
    TapButton& _left;
    TapButton& _right;
    Input _queue[queueSize];
    uint8_t _head = 0;
    uint8_t _count = 0;

    void push(Input input);
    static void onClick();
    static void onDoubleClick();
    static void onLongPress();
};
