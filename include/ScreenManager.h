#pragma once
#include "screens.h"
#include "rot_encoder.h"

class ScreenManager {
public:
    ScreenManager(RotaryEncoderManager* encoder);
    void setScreen(Screen* screen);
    void update();
    void draw();

private:
    static ScreenManager* _instance;
    RotaryEncoderManager* _encoder;
    Screen* _activeScreen = nullptr;
    RotaryInput _pendingInput;
    bool _hasPendingInput = false;

    static void onClick();
    static void onDoubleClick();
    static void onLongPress();
};
