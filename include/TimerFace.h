#pragma once
#include "SectionScreen.h"
#include "TimerService.h"
#include "TimerWidgets.h"

enum class FaceMode : uint8_t { Set, Running, Paused, Done };

class TimerFace : public SectionScreen {
public:
    TimerFace(const NavState& nav, TimerService& service);

    void init() override;
    void update() override;
    void handleInput(Input input) override;

protected:
    TimerService& _service;

    virtual FaceMode mode() const = 0;
    virtual uint32_t seconds() const = 0;
    virtual uint32_t setSeconds() const = 0;
    virtual void applySetSeconds(uint32_t total) = 0;
    virtual bool secondsEditable() const = 0;
    virtual uint32_t minSetSeconds() const = 0;
    virtual uint32_t maxSetSeconds() const = 0;
    virtual int buttonTop() const = 0;
    virtual void primary() = 0;
    virtual void end() = 0;
    virtual void resetExtraKeys() = 0;
    virtual void drawTop() = 0;
    virtual void drawFooter() {}

    void resetKeys() override;
    void drawRegions() override;

private:
    TimerUi::Readout _readout;
    int _focus = 0;
    bool _editing = false;
    int _editField = 0;
    uint32_t _editOriginal = 0;
    uint32_t _lastEditMs = 0;
    int _seenMode = -1;

    int32_t _groupKeys[2] = {-1, -1};
    int32_t _colonKey = -1, _underlineKey = -1, _layoutKey = -1, _primaryKey = -1, _endKey = -1;

    int fieldCount() const;
    int itemCount() const;
    int primaryIndex() const;
    void syncMode();
    void activate();
    void adjust(int direction);
    uint16_t digitColor(FaceMode m, int field) const;
    uint16_t colonColor(FaceMode m) const;
    void drawReadout();
    void drawButtons();
};
