#pragma once
#include <Arduino.h>

constexpr uint8_t musicMaxTracks = 24;
constexpr uint8_t musicMaxLyrics = 120;
constexpr uint8_t musicLyricChars = 24;
constexpr uint32_t lyricLeadMs = 300;
constexpr uint8_t musicIdChars = 24;
constexpr int musicCoverSide = 88;
constexpr size_t musicCoverBytes = musicCoverSide * musicCoverSide * 2;

struct MusicTrack {
    char id[musicIdChars];
    char title[28];
    uint16_t seconds;
};

struct MusicLyric {
    uint32_t startMs;
    char line1[musicLyricChars];
    char line2[musicLyricChars];
};

struct MusicAlbumInfo {
    char id[musicIdChars];
    char title[40];
    char artist[40];
    char year[8];
    uint8_t count;
    MusicTrack tracks[musicMaxTracks];
};

struct MusicNowInfo {
    bool playing;
    char id[musicIdChars];
    char title[40];
    char artist[40];
    char albumId[musicIdChars];
    char album[40];
    uint32_t positionMs;
    uint32_t durationMs;
};

class MusicPlayer {
public:
    static constexpr uint8_t maxTracks = musicMaxTracks;

    enum class State : uint8_t {
        Connecting, NoBridge, NoWifi, NotLinked, NoDevice, Idle, PremiumRequired, Error, Ready
    };
    enum class LyricsState : uint8_t { Pending, Ready, Failed };

    struct Command {
        enum Type : uint8_t { None, Play, Pause, Next, Previous, PlayTrack };
        Type type = None;
        uint8_t index = 0;
        char albumId[musicIdChars] = "";
    };

    State state() const { return _state; }
    bool ready() const { return _state == State::Ready; }
    const char* stateText() const;
    const char* stateHint() const;

    bool playing() const { return _now.playing; }
    const char* title() const { return _now.title; }
    const char* artist() const { return _now.artist; }
    uint32_t durationSeconds() const { return _now.durationMs / 1000; }
    uint32_t elapsedSeconds() const;
    uint32_t elapsedMs() const;
    bool trackEnded() const;

    uint32_t revision() const { return _revision; }
    uint32_t trackRevision() const { return _trackRevision; }
    uint32_t albumRevision() const { return _albumRevision; }
    uint32_t lyricsRevision() const { return _lyricsRevision; }

    const MusicAlbumInfo& album() const { return _album; }
    bool albumLoaded() const;
    bool albumFailed() const { return _albumFailed; }
    uint8_t trackCount() const { return albumLoaded() ? _album.count : 0; }
    const MusicTrack& track(uint8_t index) const { return _album.tracks[index]; }
    int current() const { return _current; }
    uint32_t albumSeconds() const;

    bool coverLoaded() const { return _coverPixels && _now.albumId[0] && strcmp(_coverAlbum, _now.albumId) == 0; }
    bool coverVisible() const { return ready() && coverLoaded(); }
    const uint16_t* coverPixels() const { return _coverPixels; }
    uint32_t coverRevision() const { return _coverRevision; }

    LyricsState lyricsState() const { return _lyricsState; }
    bool lyricsLoaded() const { return strcmp(_lyricsId, _now.id) == 0 && _now.id[0]; }
    uint8_t lyricCount() const { return _lyricCount; }
    const MusicLyric& lyric(uint8_t index) const { return _lyrics[index]; }
    int lyricIndex() const;

    void togglePlay();
    void next();
    void previous();
    void playTrack(uint8_t index);

    void setState(State state);
    void setNow(const MusicNowInfo& info);
    void setAlbum(const MusicAlbumInfo& album);
    void setAlbumFailed();
    void setCover(const char* albumId, const uint16_t* pixels);
    void dropCover() { clearCover(); }
    void setLyrics(const char* trackId, const MusicLyric* blocks, uint8_t count);
    void setLyricsFailed();
    bool takeCommand(Command& command);
    bool hasCommand() const { return _queued > 0; }
    const char* nowTrackId() const { return _now.id; }
    const char* nowAlbumId() const { return _now.albumId; }
    const char* lyricsTrackId() const { return _lyricsId; }

private:
    static constexpr uint8_t queueSize = 4;

    State _state = State::Connecting;
    MusicNowInfo _now = {};
    uint32_t _positionMs = 0;
    uint32_t _positionAt = 0;
    MusicAlbumInfo _album = {};
    bool _albumFailed = false;
    const uint16_t* _coverPixels = nullptr;
    char _coverAlbum[musicIdChars] = "";
    uint32_t _coverRevision = 0;
    int _current = -1;
    MusicLyric _lyrics[musicMaxLyrics] = {};
    uint8_t _lyricCount = 0;
    char _lyricsId[musicIdChars] = "";
    LyricsState _lyricsState = LyricsState::Pending;
    Command _queue[queueSize];
    uint8_t _queueHead = 0;
    uint8_t _queued = 0;
    uint32_t _revision = 0;
    uint32_t _trackRevision = 0;
    uint32_t _albumRevision = 0;
    uint32_t _lyricsRevision = 0;

    uint32_t rawElapsedMs() const;
    void rebase();
    void findCurrent();
    void clearLyrics();
    void clearCover();
    void push(Command::Type type, uint8_t index = 0);
};
