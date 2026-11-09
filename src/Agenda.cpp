#include "Agenda.h"
#include <string.h>

void Agenda::replace(const AgendaEvent* events, size_t count, uint32_t dateKey) {
    if (count > agendaCapacity) count = agendaCapacity;
    bool same = _synced && _dateKey == dateKey && _count == count && memcmp(_events, events, count * sizeof(AgendaEvent)) == 0;
    if (same) return;
    memcpy(_events, events, count * sizeof(AgendaEvent));
    _count = count;
    _synced = true;
    _dateKey = dateKey;
    _revision++;
}

void Agenda::clear() {
    if (!_synced && _count == 0) return;
    _count = 0;
    _synced = false;
    _revision++;
}

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
