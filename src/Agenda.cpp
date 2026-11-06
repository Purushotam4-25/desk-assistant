#include "Agenda.h"

EventState Agenda::stateOf(size_t i, uint16_t nowMinute) const {
    const AgendaEvent& e = _events[i];
    if (nowMinute >= e.endMinute) return EventState::Done;
    if (nowMinute >= e.startMinute) return EventState::Now;
    return EventState::Upcoming;
}

int Agenda::nextUpcoming(uint16_t nowMinute) const {
    for (size_t i = 0; i < _count; i++) {
        if (_events[i].startMinute > nowMinute) return i;
    }
    return -1;
}

int Agenda::remaining(uint16_t nowMinute) const {
    int n = 0;
    for (size_t i = 0; i < _count; i++) {
        if (stateOf(i, nowMinute) != EventState::Done) n++;
    }
    return n;
}

int Agenda::firstActiveOrUpcoming(uint16_t nowMinute) const {
    for (size_t i = 0; i < _count; i++) {
        if (stateOf(i, nowMinute) != EventState::Done) return i;
    }
    return -1;
}
