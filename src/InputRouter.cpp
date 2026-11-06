#include "InputRouter.h"

namespace {
constexpr unsigned int clickWindowMs = 170;
constexpr int encoderDebounceMs = 20;
constexpr int maxStepsPerPass = 4;
}

InputRouter* InputRouter::_instance = nullptr;

InputRouter::InputRouter(RotaryEncoderManager& encoder, TapButton& left, TapButton& right)
    : _encoder(encoder), _left(left), _right(right) {
    _instance = this;
}

// Must run after encoder.begin(), which resets the button handlers and timing.
void InputRouter::begin() {
    _encoder.setClickHandler(onClick);
    _encoder.setDoubleClickHandler(onDoubleClick);
    _encoder.setLongPressHandler(onLongPress);
    _encoder.setTiming(clickWindowMs, encoderDebounceMs);
    _left.begin();
    _right.begin();
}

void InputRouter::update() {
    _encoder.update();

    int32_t delta = _encoder.getDelta();
    int steps = delta < 0 ? -delta : delta;
    if (steps > maxStepsPerPass) steps = maxStepsPerPass;
    for (int i = 0; i < steps; i++) {
        push(delta > 0 ? Input::FocusNext : Input::FocusPrev);
    }

    if (_left.wasPressed()) push(Input::SubPagePrev);
    if (_right.wasPressed()) push(Input::SubPageNext);
}

bool InputRouter::next(Input& out) {
    if (_count == 0) return false;
    out = _queue[_head];
    _head = (_head + 1) % queueSize;
    _count--;
    return true;
}

void InputRouter::push(Input input) {
    if (_count == queueSize) return;
    _queue[(_head + _count) % queueSize] = input;
    _count++;
}

void InputRouter::onClick() { _instance->push(Input::Select); }
void InputRouter::onDoubleClick() { _instance->push(Input::Menu); }
void InputRouter::onLongPress() { _instance->push(Input::Home); }
