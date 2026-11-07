#pragma once
#include "screens.h"
#include "DeviceStatus.h"
#include "ConfigService.h"
#include "WorldCities.h"
#include "SunCalc.h"

class WorldClockScreen : public Screen {
public:
    WorldClockScreen(const DeviceStatus& status, ConfigService& config);

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
    ConfigService& _config;
    SunCalc::Position _sun = {0, 0, 0};
    int _selected;
    bool _full = true;
    bool _markersDirty = true;
    bool _infoDirty = true;
    int32_t _stamp = -1;

    City cityAt(int index) const;
    int homeOffset() const { return _config.config().utcOffsetMinutes; }
    Moment momentIn(int offsetMinutes) const;
    void redrawMap();
    void refreshMarkers();
    void drawMarkers();
    void drawInfo();
};
