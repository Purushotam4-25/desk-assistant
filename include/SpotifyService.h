#pragma once
#include <Arduino.h>
#include <atomic>
#include <ArduinoJson.h>
#include "ConfigService.h"
#include "WifiService.h"
#include "MusicPlayer.h"

class HTTPClient;

class SpotifyService {
public:
    enum class Link : uint8_t { NoBridge, WaitingForWifi, Connecting, Ok, NotLinked, PremiumRequired, Error };

    SpotifyService(const ConfigService& config, const WifiService& wifi);
    void begin();
    void update(MusicPlayer& player);
    void setActive(bool active);
    void refreshNow();

    Link link() const { return _link; }
    bool everOk() const { return _lastOkMs != 0; }
    uint32_t lastOkMs() const { return _lastOkMs; }
    const char* lastError() const { return _error; }

private:
    enum class Job : uint8_t { None, Now, Album, Lyrics, Cover, Control };
    enum class Outcome : uint8_t { Ok, Idle, NotLinked, NoDevice, PremiumRequired, Busy, Failed };

    static constexpr uint8_t failuresBeforeError = 3;

    const ConfigService& _config;
    const WifiService& _wifi;
    Link _link = Link::NoBridge;
    uint32_t _lastOkMs = 0;
    char _error[24] = "";

    bool _active = false;
    bool _forcePoll = false;
    bool _dropCover = false;
    bool _inFlight = false;
    bool _wasOffline = true;
    uint8_t _failures = 0;
    uint32_t _nextPollMs = 0;
    uint32_t _albumRetryMs = 0;
    uint32_t _lyricsRetryMs = 0;
    char _albumRetryId[musicIdChars] = "";
    char _lyricsRetryId[musicIdChars] = "";
    uint16_t* _coverBuffer = nullptr;
    char _coverTryId[musicIdChars] = "";
    uint8_t _coverTries = 0;
    uint32_t _coverRetryMs = 0;
    int32_t _tuneRev = 0;
    int32_t _nowTuneRev = 0;
    bool _tuneKnown = false;

    Job _job = Job::None;
    char _jobId[musicIdChars] = "";
    char _url[128] = "";
    char _body[96] = "";
    bool _post = false;
    uint16_t _timeoutMs = 6000;

    MusicNowInfo _nowResult = {};
    MusicAlbumInfo _albumResult = {};
    MusicLyric _lyricsResult[musicMaxLyrics] = {};
    uint8_t _lyricsCount = 0;
    Outcome _outcome = Outcome::Failed;
    char _taskError[24] = "";
    std::atomic<bool> _requested{false};
    std::atomic<bool> _resultReady{false};

    uint32_t intervalMs() const;
    bool coverAttemptAllowed(const char* albumId, uint32_t now);
    bool buildUrl(const char* path);
    void start(Job job, const char* path, const char* id = "");
    void startControl(const MusicPlayer::Command& command);
    void finish(MusicPlayer& player);
    void applyState(MusicPlayer& player, Outcome outcome);
    void execute();
    void readCover(HTTPClient& http);
    void parseNow(JsonDocument& doc);
    void parseAlbum(JsonDocument& doc);
    void parseLyrics(JsonDocument& doc);
    static void taskMain(void* self);
};
