#pragma once
#include "screens.h"
#include "DeviceStatus.h"
#include "FocusSession.h"
#include "NavState.h"

class FocusScreen : public Screen {
public:
    FocusScreen(const DeviceStatus& status, FocusSession& session, const NavState& nav, Screen& statsScreen);

    void init() override;
    void update() override;
    void draw() override;
    void handleInput(Input input) override;
    Screen* takeDetail() override;

private:
    enum class Item : uint8_t { Action, End, Stats };

    const DeviceStatus& _status;
    FocusSession& _session;
    const NavState& _nav;
    Screen& _statsScreen;
    bool _full = true;
    bool _openStats = false;
    Item _focused = Item::Action;

    int _timeScale = 4;
    int _groupWidth = 0, _colonGap = 0, _timeX = 0;
    int _digitTop = 0, _digitHeight = 0, _textY = 0;

    int32_t _statusKey, _headerKey, _goalKey, _colonKey, _layoutKey, _actionKey, _endKey, _statsKey;
    int32_t _timeKeys[3];

    void resetKeys();
    void computeTimeLayout();
    bool available(Item item) const;
    void moveFocus(int step);
    void activate();

    void drawStatus();
    void drawHeader();
    void drawGoalBar();
    void drawTime();
    void drawColons(FocusState state, uint32_t seconds);
    void drawButtons();
    void drawButton(int x, const char* label, bool focused);
    void drawStatsButton();
};
