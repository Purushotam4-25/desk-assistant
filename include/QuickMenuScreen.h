#pragma once
#include "screens.h"
#include "DeviceStatus.h"
#include "NavState.h"
#include "QuickSettings.h"

class QuickMenuScreen : public Screen {
public:
    QuickMenuScreen(const DeviceStatus& status, const NavState& nav, QuickSettings& settings);

    void init() override;
    void update() override;
    void draw() override;
    void handleInput(Input input) override;
    bool takeNavigation(NavTarget& target) override;

private:
    enum Item : uint8_t { Wifi, Night, Timer, Brightness, itemCount };

    const DeviceStatus& _status;
    const NavState& _nav;
    QuickSettings& _settings;
    int _focused = Wifi;
    bool _full = true;
    bool _openTimer = false;

    int32_t _statusKey;
    int32_t _buttonKeys[itemCount];

    void resetKeys();
    void activate();
    int32_t buttonKey(int item) const;
    void drawStatus();
    void drawButton(int item);
};
