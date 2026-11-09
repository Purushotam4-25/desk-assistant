#include "SpotifyService.h"
#include <HTTPClient.h>
#include <WiFiClient.h>

namespace {
constexpr uint32_t pollMs[] = {5000, 10000, 30000, 60000};
constexpr uint32_t commandRepollMs = 700;
constexpr uint32_t trackEndRepollMs = 800;
constexpr uint32_t retryAfterFailureMs = 30000;
constexpr uint32_t busyBackoffMs = 30000;
constexpr uint16_t defaultTimeoutMs = 6000, lyricsTimeoutMs = 12000, connectTimeoutMs = 5000;
constexpr uint8_t httpPrefixLength = 7;
constexpr uint8_t maxCoverTries = 3;

bool retryAllowed(const char* id, const char* triedId, uint32_t untilMs, uint32_t now) {
    return strcmp(id, triedId) != 0 || (int32_t)(now - untilMs) >= 0;
}
}

SpotifyService::SpotifyService(const ConfigService& config, const WifiService& wifi) : _config(config), _wifi(wifi) {}

void SpotifyService::begin() {
    _coverBuffer = static_cast<uint16_t*>(malloc(musicCoverBytes));
    if (!_coverBuffer) Serial.println("Spotify: no memory for cover art");
    xTaskCreatePinnedToCore(taskMain, "spotify", 10240, this, 1, nullptr, 0);
}

uint32_t SpotifyService::intervalMs() const {
    int16_t choice = _config.value(SettingId::SpotifyPoll);
    return pollMs[choice < 0 || choice > 3 ? 1 : choice];
}

void SpotifyService::setActive(bool active) {
    if (active && !_active) _nextPollMs = millis();
    _active = active;
}

void SpotifyService::refreshNow() {
    _dropCover = true;
    _forcePoll = true;
    _nextPollMs = millis();
}

bool SpotifyService::buildUrl(const char* path) {
    const char* base = _config.config().bridgeUrl;
    if (strncmp(base, "http://", httpPrefixLength) != 0) {
        snprintf(_error, sizeof(_error), "Bridge must be http://");
        _link = Link::Error;
        return false;
    }
    size_t length = strlen(base);
    while (length > 0 && base[length - 1] == '/') length--;
    snprintf(_url, sizeof(_url), "%.*s%s", (int)length, base, path);
    return true;
}

void SpotifyService::start(Job job, const char* path, const char* id) {
    if (!buildUrl(path)) return;
    _job = job;
    strlcpy(_jobId, id, sizeof(_jobId));
    _post = false;
    _timeoutMs = job == Job::Lyrics || job == Job::Cover ? lyricsTimeoutMs : defaultTimeoutMs;
    _inFlight = true;
    _requested = true;
}

void SpotifyService::startControl(const MusicPlayer::Command& command) {
    const char* action = nullptr;
    switch (command.type) {
        case MusicPlayer::Command::Play: action = "play"; break;
        case MusicPlayer::Command::Pause: action = "pause"; break;
        case MusicPlayer::Command::Next: action = "next"; break;
        case MusicPlayer::Command::Previous: action = "prev"; break;
        default: break;
    }
    bool playTrack = command.type == MusicPlayer::Command::PlayTrack;
    if (!action && !playTrack) return;

    if (playTrack) {
        snprintf(_body, sizeof(_body), "{\"album\":\"%s\",\"index\":%u}", command.albumId, command.index);
    } else {
        snprintf(_body, sizeof(_body), "{\"action\":\"%s\"}", action);
    }
    start(Job::Control, playTrack ? "/spotify/play" : "/spotify/control");
    _post = true;
}

void SpotifyService::update(MusicPlayer& player) {
    if (_resultReady) {
        finish(player);
        return;
    }
    if (_inFlight) return;

    if (_dropCover) {
        _dropCover = false;
        player.dropCover();
        _coverTryId[0] = '\0';
    }

    if (_config.config().bridgeUrl[0] == '\0') {
        _link = Link::NoBridge;
        _wasOffline = true;
        player.setState(MusicPlayer::State::NoBridge);
        return;
    }
    if (!_wifi.connected()) {
        _link = Link::WaitingForWifi;
        _wasOffline = true;
        player.setState(MusicPlayer::State::NoWifi);
        return;
    }
    if (_wasOffline) {
        _wasOffline = false;
        _nextPollMs = millis();
        _link = Link::Connecting;
        player.setState(MusicPlayer::State::Connecting);
    }

    MusicPlayer::Command command;
    if (player.takeCommand(command)) {
        startControl(command);
        return;
    }

    uint32_t now = millis();
    if (_active && player.trackEnded() && (int32_t)(_nextPollMs - now) > (int32_t)trackEndRepollMs) {
        _nextPollMs = now + trackEndRepollMs;
    }
    if ((_active || _forcePoll) && (int32_t)(now - _nextPollMs) >= 0) {
        _nextPollMs = now + intervalMs();
        start(Job::Now, "/spotify/now");
        return;
    }
    if (!_active || !player.ready()) return;

    const char* albumId = player.nowAlbumId();
    if (albumId[0] && !player.albumLoaded() && retryAllowed(albumId, _albumRetryId, _albumRetryMs, now)) {
        strlcpy(_albumRetryId, albumId, sizeof(_albumRetryId));
        _albumRetryMs = now + retryAfterFailureMs;
        char path[48];
        snprintf(path, sizeof(path), "/spotify/album?id=%s", albumId);
        start(Job::Album, path, albumId);
        return;
    }

    if (_coverBuffer && albumId[0] && !player.coverLoaded() && coverAttemptAllowed(albumId, now)) {
        char path[48];
        snprintf(path, sizeof(path), "/spotify/cover?id=%s", albumId);
        start(Job::Cover, path, albumId);
        return;
    }

    const char* trackId = player.nowTrackId();
    if (trackId[0] && strcmp(player.lyricsTrackId(), trackId) != 0 &&
        retryAllowed(trackId, _lyricsRetryId, _lyricsRetryMs, now)) {
        strlcpy(_lyricsRetryId, trackId, sizeof(_lyricsRetryId));
        _lyricsRetryMs = now + retryAfterFailureMs;
        char path[48];
        snprintf(path, sizeof(path), "/spotify/lyrics?id=%s", trackId);
        start(Job::Lyrics, path, trackId);
    }
}

bool SpotifyService::coverAttemptAllowed(const char* albumId, uint32_t now) {
    if (strcmp(albumId, _coverTryId) != 0) {
        strlcpy(_coverTryId, albumId, sizeof(_coverTryId));
        _coverTries = 0;
        _coverRetryMs = now;
    }
    if (_coverTries >= maxCoverTries || (int32_t)(now - _coverRetryMs) < 0) return false;
    _coverTries++;
    _coverRetryMs = now + retryAfterFailureMs;
    return true;
}

void SpotifyService::applyState(MusicPlayer& player, Outcome outcome) {
    switch (outcome) {
        case Outcome::Ok:
        case Outcome::Idle:
        case Outcome::NoDevice:
            _failures = 0;
            _link = Link::Ok;
            _lastOkMs = millis();
            _error[0] = '\0';
            if (outcome == Outcome::Ok) {
                player.setNow(_nowResult);
            } else {
                player.setState(outcome == Outcome::Idle ? MusicPlayer::State::Idle : MusicPlayer::State::NoDevice);
            }
            break;
        case Outcome::NotLinked:
            _failures = 0;
            _link = Link::NotLinked;
            _lastOkMs = millis();
            snprintf(_error, sizeof(_error), "Not linked");
            player.setState(MusicPlayer::State::NotLinked);
            break;
        case Outcome::PremiumRequired:
            _failures = 0;
            _link = Link::PremiumRequired;
            snprintf(_error, sizeof(_error), "Premium needed");
            player.setState(MusicPlayer::State::PremiumRequired);
            break;
        case Outcome::Busy:
            snprintf(_error, sizeof(_error), "Rate limited");
            _nextPollMs = millis() + busyBackoffMs;
            break;
        case Outcome::Failed:
            strlcpy(_error, _taskError, sizeof(_error));
            if (_failures < 255) _failures++;
            if (_failures >= failuresBeforeError) {
                _link = Link::Error;
                player.setState(MusicPlayer::State::Error);
            }
            break;
    }
}

void SpotifyService::finish(MusicPlayer& player) {
    _resultReady = false;
    _inFlight = false;
    Job job = _job;
    _job = Job::None;

    switch (job) {
        case Job::Now:
            _forcePoll = false;
            if (player.hasCommand()) {
                _nextPollMs = millis() + commandRepollMs;
                return;
            }
            applyState(player, _outcome);
            if (_outcome == Outcome::Ok) {
                if (_tuneKnown && _nowTuneRev != _tuneRev) _dropCover = true;
                _tuneRev = _nowTuneRev;
                _tuneKnown = true;
            }
            break;
        case Job::Control:
            _nextPollMs = millis() + commandRepollMs;
            if (_outcome == Outcome::Busy) {
                snprintf(_error, sizeof(_error), "Rate limited");
            } else if (_outcome == Outcome::Failed) {
                strlcpy(_error, _taskError, sizeof(_error));
            } else if (_outcome != Outcome::Ok) {
                applyState(player, _outcome);
            }
            break;
        case Job::Album:
            if (_outcome == Outcome::Ok) {
                player.setAlbum(_albumResult);
            } else {
                player.setAlbumFailed();
            }
            break;
        case Job::Lyrics:
            if (_outcome == Outcome::Ok) {
                player.setLyrics(_jobId, _lyricsResult, _lyricsCount);
            } else {
                player.setLyricsFailed();
            }
            break;
        case Job::Cover:
            if (_outcome == Outcome::Ok) player.setCover(_jobId, _coverBuffer);
            break;
        case Job::None:
            break;
    }
}

void SpotifyService::taskMain(void* self) {
    SpotifyService* service = static_cast<SpotifyService*>(self);
    for (;;) {
        if (service->_requested) {
            service->execute();
            service->_requested = false;
            service->_resultReady = true;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void SpotifyService::execute() {
    _outcome = Outcome::Failed;
    _taskError[0] = '\0';

    WiFiClient client;
    HTTPClient http;
    http.setTimeout(_timeoutMs);
    http.setConnectTimeout(connectTimeoutMs);
    if (!http.begin(client, _url)) {
        snprintf(_taskError, sizeof(_taskError), "Bad bridge address");
        return;
    }

    int code;
    if (_post) {
        http.addHeader("Content-Type", "application/json");
        code = http.POST(reinterpret_cast<uint8_t*>(_body), strlen(_body));
    } else {
        code = http.GET();
    }
    if (code != HTTP_CODE_OK) {
        if (code > 0) {
            snprintf(_taskError, sizeof(_taskError), "Bridge HTTP %d", code);
        } else {
            snprintf(_taskError, sizeof(_taskError), "%s", HTTPClient::errorToString(code).c_str());
        }
        http.end();
        return;
    }

    if (_job == Job::Cover) {
        readCover(http);
        http.end();
        return;
    }

    String payload = http.getString();
    http.end();
    JsonDocument doc;
    if (deserializeJson(doc, payload)) {
        snprintf(_taskError, sizeof(_taskError), "Bad data");
        return;
    }

    if (!(doc["ok"] | false)) {
        const char* err = doc["err"] | "";
        if (strcmp(err, "not_linked") == 0) {
            _outcome = Outcome::NotLinked;
        } else if (strcmp(err, "no_device") == 0) {
            _outcome = Outcome::NoDevice;
        } else if (strcmp(err, "premium_required") == 0) {
            _outcome = Outcome::PremiumRequired;
        } else if (strcmp(err, "rate_limited") == 0) {
            _outcome = Outcome::Busy;
        } else {
            snprintf(_taskError, sizeof(_taskError), "Spotify %s", err[0] ? err : "error");
        }
        return;
    }

    switch (_job) {
        case Job::Now: parseNow(doc); break;
        case Job::Album: parseAlbum(doc); break;
        case Job::Lyrics: parseLyrics(doc); break;
        default: _outcome = Outcome::Ok; break;
    }
}

void SpotifyService::readCover(HTTPClient& http) {
    if (http.getSize() != (int)musicCoverBytes) {
        snprintf(_taskError, sizeof(_taskError), "No cover");
        return;
    }
    WiFiClient* stream = http.getStreamPtr();
    uint8_t* out = reinterpret_cast<uint8_t*>(_coverBuffer);
    size_t got = 0;
    uint32_t lastData = millis();
    while (got < musicCoverBytes && millis() - lastData < _timeoutMs) {
        size_t available = stream->available();
        if (available == 0) {
            if (!stream->connected()) break;
            delay(2);
            continue;
        }
        size_t wanted = musicCoverBytes - got;
        got += stream->readBytes(out + got, available < wanted ? available : wanted);
        lastData = millis();
    }
    if (got == musicCoverBytes) {
        _outcome = Outcome::Ok;
    } else {
        snprintf(_taskError, sizeof(_taskError), "Short cover");
    }
}

void SpotifyService::parseNow(JsonDocument& doc) {
    if (doc["idle"] | false) {
        _outcome = Outcome::Idle;
        return;
    }
    MusicNowInfo& now = _nowResult;
    now = {};
    now.playing = doc["playing"] | false;
    strlcpy(now.id, doc["id"] | "", sizeof(now.id));
    strlcpy(now.title, doc["title"] | "", sizeof(now.title));
    strlcpy(now.artist, doc["artist"] | "", sizeof(now.artist));
    strlcpy(now.albumId, doc["albumId"] | "", sizeof(now.albumId));
    strlcpy(now.album, doc["album"] | "", sizeof(now.album));
    now.positionMs = doc["pos"] | 0;
    now.durationMs = doc["dur"] | 0;
    _nowTuneRev = doc["cv"] | 0;
    if (now.id[0]) {
        _outcome = Outcome::Ok;
    } else {
        snprintf(_taskError, sizeof(_taskError), "Bad data");
    }
}

void SpotifyService::parseAlbum(JsonDocument& doc) {
    JsonArray tracks = doc["tracks"];
    MusicAlbumInfo& album = _albumResult;
    album = {};
    strlcpy(album.id, doc["id"] | "", sizeof(album.id));
    strlcpy(album.title, doc["title"] | "", sizeof(album.title));
    strlcpy(album.artist, doc["artist"] | "", sizeof(album.artist));
    strlcpy(album.year, doc["year"] | "", sizeof(album.year));
    size_t count = tracks.size() < musicMaxTracks ? tracks.size() : musicMaxTracks;
    for (size_t i = 0; i < count; i++) {
        JsonObject track = tracks[i];
        strlcpy(album.tracks[i].id, track["id"] | "", sizeof(album.tracks[i].id));
        strlcpy(album.tracks[i].title, track["t"] | "", sizeof(album.tracks[i].title));
        album.tracks[i].seconds = (uint32_t)(track["ms"] | 0) / 1000;
    }
    album.count = count;
    if (album.id[0] && count > 0) {
        _outcome = Outcome::Ok;
    } else {
        snprintf(_taskError, sizeof(_taskError), "Bad data");
    }
}

void SpotifyService::parseLyrics(JsonDocument& doc) {
    if (!doc["blocks"].is<JsonArray>()) {
        snprintf(_taskError, sizeof(_taskError), "Bad data");
        return;
    }
    JsonArray blocks = doc["blocks"];
    size_t count = blocks.size() < musicMaxLyrics ? blocks.size() : musicMaxLyrics;
    for (size_t i = 0; i < count; i++) {
        JsonObject block = blocks[i];
        MusicLyric& lyric = _lyricsResult[i];
        lyric.startMs = block["ms"] | 0;
        strlcpy(lyric.line1, block["a"] | "", sizeof(lyric.line1));
        strlcpy(lyric.line2, block["b"] | "", sizeof(lyric.line2));
    }
    _lyricsCount = count;
    _outcome = Outcome::Ok;
}
