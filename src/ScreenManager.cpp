#include "ScreenManager.h"

ScreenManager* ScreenManager::_instance = nullptr;

ScreenManager::ScreenManager(RotaryEncoderManager* encoder) : _encoder(encoder) {
    _instance = this;
    _encoder->setClickHandler(onClick);
    _encoder->setDoubleClickHandler(onDoubleClick);
    _encoder->setLongPressHandler(onLongPress);
}

void ScreenManager::setScreen(Screen* screen) {
    _activeScreen = screen;
    _activeScreen->init();
}

void ScreenManager::update() {
    if (!_activeScreen) return;

    int32_t delta = _encoder->getDelta();
    if (delta > 0) {
        _activeScreen->handleInput(RotaryInput::ROTATE_CW);
    } else if (delta < 0) {
        _activeScreen->handleInput(RotaryInput::ROTATE_CCW);
    }

    if (_hasPendingInput) {
        _activeScreen->handleInput(_pendingInput);
        _hasPendingInput = false;
    }

    _activeScreen->update();
}

void ScreenManager::draw() {
    if (_activeScreen) _activeScreen->draw();
}

void ScreenManager::onClick() {
    _instance->_pendingInput = RotaryInput::CLICK;
    _instance->_hasPendingInput = true;
}

void ScreenManager::onDoubleClick() {
    _instance->_pendingInput = RotaryInput::DOUBLE_CLICK;
    _instance->_hasPendingInput = true;
}

void ScreenManager::onLongPress() {
    _instance->_pendingInput = RotaryInput::LONG_PRESS;
    _instance->_hasPendingInput = true;
}
