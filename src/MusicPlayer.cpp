#include "MusicPlayer.h"

void MusicPlayer::setAlbum(const MusicAlbum& album, const MusicTrack* tracks, uint8_t count) {
    _album = album;
    _count = count < maxTracks ? count : maxTracks;
    for (uint8_t i = 0; i < _count; i++) _tracks[i] = tracks[i];
    _current = 0;
    _playing = false;
    restartTrack();
    _revision++;
}

uint32_t MusicPlayer::elapsedMs() const {
    uint32_t ms = _accumulatedMs;
    if (_playing) ms += millis() - _startedAt;
    return ms;
}

uint32_t MusicPlayer::lengthMs() const {
    uint32_t seconds = _tracks[_current].seconds;
    return (seconds ? seconds : 1) * 1000UL;
}

void MusicPlayer::restartTrack() {
    _accumulatedMs = 0;
    _startedAt = millis();
}

void MusicPlayer::update() {
    if (!_playing || _count == 0) return;
    uint32_t ms = elapsedMs();
    uint32_t length = lengthMs();
    if (ms < length) return;

    if (_current + 1 < _count) {
        _current++;
        _accumulatedMs = ms - length;
        _startedAt = millis();
    } else {
        _current = 0;
        _playing = false;
        _accumulatedMs = 0;
    }
    _revision++;
}

void MusicPlayer::togglePlay() {
    if (_count == 0) return;
    if (_playing) {
        _accumulatedMs = elapsedMs();
        _playing = false;
    } else {
        _startedAt = millis();
        _playing = true;
    }
    _revision++;
}

void MusicPlayer::next() {
    if (_count == 0 || _current + 1 >= _count) return;
    _current++;
    restartTrack();
    _revision++;
}

void MusicPlayer::previous() {
    if (_count == 0) return;
    if (elapsedSeconds() <= restartAfterSeconds && _current > 0) _current--;
    restartTrack();
    _revision++;
}

void MusicPlayer::select(uint8_t index) {
    if (index >= _count) return;
    _current = index;
    _playing = true;
    restartTrack();
    _revision++;
}

uint32_t MusicPlayer::elapsedSeconds() const {
    if (_count == 0) return 0;
    uint32_t seconds = elapsedMs() / 1000;
    uint32_t length = _tracks[_current].seconds;
    return seconds < length ? seconds : length;
}

uint32_t MusicPlayer::albumSeconds() const {
    uint32_t total = 0;
    for (uint8_t i = 0; i < _count; i++) total += _tracks[i].seconds;
    return total;
}

int MusicPlayer::lyricIndex() const {
    if (_count == 0) return -1;
    const MusicTrack& track = _tracks[_current];
    uint32_t elapsed = elapsedSeconds();
    int index = -1;
    for (uint8_t i = 0; i < track.lyricCount; i++) {
        if (track.lyrics[i].startSecond > elapsed) break;
        index = i;
    }
    return index;
}
