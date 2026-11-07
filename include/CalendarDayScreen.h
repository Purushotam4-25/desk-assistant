#pragma once
#include "screens.h"
#include "DeviceStatus.h"
#include "Agenda.h"

namespace CalendarDay {
constexpr uint8_t morningStartHour = 8;
constexpr uint8_t afternoonStartHour = 13;
constexpr uint8_t windowHours = 5;
}

class CalendarDayScreen : public Screen {
public:
    CalendarDayScreen(const DeviceStatus& status, const Agenda& agenda, uint8_t startHour);

    static uint8_t windowFor(uint16_t minuteOfDay);

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
    uint8_t _startHour;
    uint16_t _windowStart;
    uint16_t _windowEnd;
    Block _blocks[maxBlocks];
    uint8_t _blockCount = 0;
    bool _full = true;
    int32_t _headerKey = -1;
    int32_t _nowKey = -1;
    int _nowY = -1;
    uint8_t _hiddenLabels = 0;
    int _paintX = 0, _paintY = 0, _paintW = 0, _paintH = 0;

    int yOfMinute(uint16_t minute) const;
    bool inWindow(uint16_t minute) const;
    bool overlapsPaint(int x0, int y0, int x1, int y1) const;
    uint8_t hiddenMaskFor(int nowY) const;
    void buildLayout(uint16_t now);
    void setNow(uint16_t now);
    void fullRedraw(uint16_t now);
    void updateLive(uint16_t now);
    void drawHeader();
    void paintRect(int x, int y, int w, int h);
    void drawGrid();
    void drawBlock(const Block& block);
    void drawNowLine(uint16_t now);
    void paintLineBand(int y);
    void paintLabelBand(int y);
};
