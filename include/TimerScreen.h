#pragma once
#include "TimerFace.h"

class TimerScreen : public TimerFace {
public:
    TimerScreen(const NavState& nav, TimerService& service);

protected:
    FaceMode mode() const override;
    uint32_t seconds() const override;
    uint32_t setSeconds() const override;
    void applySetSeconds(uint32_t total) override;
    bool secondsEditable() const override { return true; }
    uint32_t minSetSeconds() const override;
    uint32_t maxSetSeconds() const override;
    int buttonTop() const override { return TimerUi::timerButtonY; }
    void primary() override;
    void end() override;
    void resetExtraKeys() override;
    void drawTop() override;

private:
    int32_t _labelKey = -1;
};
