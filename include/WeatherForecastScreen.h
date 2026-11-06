#pragma once
#include "SectionScreen.h"
#include "WeatherData.h"
#include "DeviceStatus.h"

class WeatherForecastScreen : public SectionScreen {
public:
    WeatherForecastScreen(const WeatherData& weather, const DeviceStatus& status, const NavState& nav);

protected:
    void resetKeys() override;
    void drawRegions() override;

private:
    const WeatherData& _weather;
    const DeviceStatus& _status;
    int64_t _dataKey = -1;

    void drawDay(int slot);
};
