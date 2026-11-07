#pragma once
#include <Arduino.h>

struct MusicLyric {
    uint16_t startSecond;
    const char* line1;
    const char* line2;
};

struct MusicTrack {
    const char* title;
    uint16_t seconds;
    const MusicLyric* lyrics;
    uint8_t lyricCount;
};

struct MusicAlbum {
    const char* title;
    const char* artist;
    const char* year;
};

class MusicPlayer {
public:
    static constexpr uint8_t maxTracks = 24;

    void setAlbum(const MusicAlbum& album, const MusicTrack* tracks, uint8_t count);
    void update();

    void togglePlay();
    void next();
    void previous();
    void select(uint8_t index);

    bool playing() const { return _playing; }
    uint32_t revision() const { return _revision; }
    const MusicAlbum& album() const { return _album; }
    uint8_t trackCount() const { return _count; }
    uint8_t current() const { return _current; }
    const MusicTrack& track(uint8_t index) const { return _tracks[index]; }
    const MusicTrack& currentTrack() const { return _tracks[_current]; }
    uint32_t elapsedSeconds() const;
    uint32_t albumSeconds() const;
    int lyricIndex() const;

private:
    static constexpr uint32_t restartAfterSeconds = 3;

    MusicAlbum _album = {};
    MusicTrack _tracks[maxTracks] = {};
    uint8_t _count = 0;
    uint8_t _current = 0;
    bool _playing = false;
    uint32_t _accumulatedMs = 0;
    uint32_t _startedAt = 0;
    uint32_t _revision = 0;

    uint32_t elapsedMs() const;
    uint32_t lengthMs() const;
    void restartTrack();
};
