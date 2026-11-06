#pragma once
#include "screens.h"
#include "DeviceStatus.h"
#include "Agenda.h"
#include "NavState.h"

class TodayScreen : public Screen {
public:
    TodayScreen(const DeviceStatus& status, const Agenda& agenda, const NavState& nav);

    void init() override;
    void update() override;
    void draw() override;
    void handleInput(Input input) override;

private:
    const DeviceStatus& _status;
    const Agenda& _agenda;
    const NavState& _nav;
    bool _full = true;

    int32_t _statusKey, _headerKey, _rowsKey;

    int firstVisibleRow() const;
    void drawStatus();
    void drawHeader();
    void drawRows();
    void drawRow(int slot, const AgendaEvent& event, EventState state);
};
