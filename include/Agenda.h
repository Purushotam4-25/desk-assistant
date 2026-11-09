#pragma once
#include <stdint.h>
#include <stddef.h>

constexpr size_t agendaCapacity = 16;
constexpr size_t agendaTitleChars = 28;

struct AgendaEvent {
    uint16_t startMinute;
    uint16_t endMinute;
    char title[agendaTitleChars];
};

enum class EventState { Done, Now, Upcoming };

class Agenda {
public:
    size_t count() const { return _count; }
    const AgendaEvent& at(size_t i) const { return _events[i]; }
    bool synced() const { return _synced; }
    uint32_t dateKey() const { return _dateKey; }
    uint32_t revision() const { return _revision; }

    void replace(const AgendaEvent* events, size_t count, uint32_t dateKey);
    void clear();

    EventState stateOf(size_t i, uint16_t nowMinute) const;
    int nextUpcoming(uint16_t nowMinute) const;
    int remaining(uint16_t nowMinute) const;
    int firstActiveOrUpcoming(uint16_t nowMinute) const;

private:
    AgendaEvent _events[agendaCapacity] = {};
    size_t _count = 0;
    bool _synced = false;
    uint32_t _dateKey = 0;
    uint32_t _revision = 0;
};
