#pragma once
#include "screens.h"
#include "DeviceStatus.h"
#include "Agenda.h"

struct CalendarSurface;

namespace CalendarDay {
constexpr uint8_t morningStartHour = 8;
constexpr uint8_t afternoonStartHour = 13;
constexpr uint8_t windowHours = 5;
constexpr uint16_t scrollStepMinutes = 30;
constexpr uint16_t maxStartMinute = (24 - windowHours) * 60;
}

class CalendarDayScreen : public Screen {
public:
    CalendarDayScreen(const DeviceStatus& status, const Agenda& agenda, uint8_t startHour);

    static uint8_t windowFor(uint16_t minuteOfDay);

    void followNow() { _followNow = true; }
    void init() override;
    void update() override;
    void draw() override;
    void handleInput(Input input) override;

private:
    static constexpr uint8_t maxBlocks = 16;

    struct Block {
        uint8_t event;
        uint8_t column;
        uint8_t columns;
        uint16_t start;
        uint16_t end;
        int16_t top;
        int16_t bottom;
        int16_t x;
        int16_t width;
        EventState state;
    };

    const DeviceStatus& _status;
    const Agenda& _agenda;
    uint16_t _presetStart;
    uint16_t _windowStart;
    uint16_t _windowEnd;
    uint16_t _targetStart;
    Block _blocks[maxBlocks];
    uint8_t _blockCount = 0;
    uint32_t _layoutRevision = 0;
    bool _full = true;
    bool _followNow = false;
    int32_t _headerKey = -1;
    int32_t _nowKey = -1;
    int _nowY = -1;
    uint8_t _hiddenLabels = 0;
    int _paintX = 0, _paintY = 0, _paintW = 0, _paintH = 0;

    void applyWindow(uint16_t start);
    int yOfMinute(uint16_t minute) const;
    bool inWindow(uint16_t minute) const;
    int firstLineMinute() const;
    int lineCount() const;
    int lineY(int index) const;
    bool overlapsPaint(int x0, int y0, int x1, int y1) const;
    uint8_t hiddenMaskFor(int nowY) const;
    void buildLayout(uint16_t now);
    void setNow(uint16_t now);
    void fullRedraw(uint16_t now);
    void scrollRedraw(uint16_t now);
    void updateLive(uint16_t now);
    void drawHeader();
    void repaintRows(int y0, int y1);
    void paintBand(int top, int rows);
    void compose(const CalendarSurface& surface);
    void drawGrid(const CalendarSurface& surface);
    void drawBlock(const CalendarSurface& surface, const Block& block);
    void drawNowLine(const CalendarSurface& surface);
};
