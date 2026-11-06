#pragma once
#include <stdint.h>
#include <stddef.h>

struct AgendaEvent {
    uint16_t startMinute;
    uint16_t endMinute;
    const char* title;
};

enum class EventState { Done, Now, Upcoming };

class Agenda {
public:
    Agenda(const AgendaEvent* events, size_t count) : _events(events), _count(count) {}

    size_t count() const { return _count; }
    const AgendaEvent& at(size_t i) const { return _events[i]; }

    EventState stateOf(size_t i, uint16_t nowMinute) const;
    int nextUpcoming(uint16_t nowMinute) const;
    int remaining(uint16_t nowMinute) const;
    int firstActiveOrUpcoming(uint16_t nowMinute) const;

private:
    const AgendaEvent* _events;
    size_t _count;
};
