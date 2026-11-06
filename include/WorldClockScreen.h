#pragma once
#include "screens.h"
#include "DeviceStatus.h"
#include "SunCalc.h"

class WorldClockScreen : public Screen {
public:
    explicit WorldClockScreen(const DeviceStatus& status);

    void init() override;
    void update() override;
    void draw() override;
    void handleInput(Input input) override;

private:
    struct Moment {
        int32_t days;
        int minute;
    };

    const DeviceStatus& _status;
    SunCalc::Position _sun = {0, 0, 0};
    int _selected;
    bool _use24h = false;
    bool _full = true;
    bool _markersDirty = true;
    bool _infoDirty = true;
    int32_t _stamp = -1;

    Moment momentIn(int offsetMinutes) const;
    void redrawMap();
    void refreshMarkers();
    void drawMarkers();
    void drawInfo();
};
