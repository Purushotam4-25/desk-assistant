#pragma once
#include "SectionScreen.h"
#include "WeatherData.h"

class WeatherAirScreen : public SectionScreen {
public:
    WeatherAirScreen(const WeatherData& weather, const NavState& nav);

protected:
    void resetKeys() override;
    void drawFrame() override;
    void drawRegions() override;

private:
    const WeatherData& _weather;
    int64_t _dataKey = -1;
};
