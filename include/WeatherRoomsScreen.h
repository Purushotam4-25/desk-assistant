#pragma once
#include "SectionScreen.h"
#include "WeatherData.h"
#include "DeviceStatus.h"

class WeatherRoomsScreen : public SectionScreen {
public:
    WeatherRoomsScreen(const WeatherData& weather, const DeviceStatus& status, const NavState& nav);

protected:
    void resetKeys() override;
    void drawFrame() override;
    void drawRegions() override;

private:
    const WeatherData& _weather;
    const DeviceStatus& _status;
    int32_t _indoorKey = -1;
    int64_t _outdoorKey = -1;

    void drawIndoor();
    void drawOutdoor();
};
