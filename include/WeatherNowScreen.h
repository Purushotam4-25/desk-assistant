#pragma once
#include "SectionScreen.h"
#include "WeatherData.h"

class WeatherNowScreen : public SectionScreen {
public:
    WeatherNowScreen(const WeatherData& weather, const NavState& nav);

protected:
    void resetKeys() override;
    void drawRegions() override;

private:
    const WeatherData& _weather;
    int64_t _dataKey = -1;

    void drawLimits();
};
