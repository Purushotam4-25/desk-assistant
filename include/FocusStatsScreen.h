#pragma once
#include "screens.h"
#include "DeviceStatus.h"
#include "FocusSession.h"
#include "NavState.h"

class FocusStatsScreen : public Screen {
public:
    FocusStatsScreen(const DeviceStatus& status, const FocusSession& session, const NavState& nav);

    void init() override;
    void update() override;
    void draw() override;
    void handleInput(Input input) override;

private:
    const DeviceStatus& _status;
    const FocusSession& _session;
    const NavState& _nav;
    bool _full = true;

    int32_t _statusKey, _headerKey, _todayKey, _weekKey, _chartKey;

    void resetKeys();
    void drawStatus();
    void drawHeader();
    void drawTodayCard();
    void drawWeekCard();
    void drawCard(int x, const char* label, uint16_t labelColor, const char* value, const char* line1, const char* line2);
    void drawChart();
};
