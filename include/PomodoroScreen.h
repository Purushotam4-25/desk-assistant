#pragma once
#include "TimerFace.h"

class PomodoroScreen : public TimerFace {
public:
    PomodoroScreen(const NavState& nav, TimerService& service);

protected:
    FaceMode mode() const override;
    uint32_t seconds() const override;
    uint32_t setSeconds() const override;
    void applySetSeconds(uint32_t total) override;
    bool secondsEditable() const override { return false; }
    uint32_t minSetSeconds() const override;
    uint32_t maxSetSeconds() const override;
    int buttonTop() const override { return TimerUi::buttonY; }
    void primary() override;
    void end() override;
    void resetExtraKeys() override;
    void drawTop() override;
    void drawFooter() override;

private:
    int32_t _trackKey = -1;
    int32_t _footerKey = -1;

    int trackStep() const;
};
