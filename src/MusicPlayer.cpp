#include "MusicPlayer.h"

const char* MusicPlayer::stateText() const {
    switch (_state) {
        case State::Connecting: return "Connecting";
        case State::NoBridge: return "No bridge";
        case State::NoWifi: return "No Wi-Fi";
        case State::NotLinked: return "Not connected";
        case State::NoDevice: return "No device";
        case State::Idle: return "Nothing playing";
        case State::PremiumRequired: return "Premium needed";
        case State::Error: return "Can't reach NAS";
        case State::Ready: return "";
    }
    return "";
}

const char* MusicPlayer::stateHint() const {
    switch (_state) {
        case State::NoBridge: return "Set bridgeUrl";
        case State::NotLinked: return "Run the PC login";
        case State::NoDevice: return "Open Spotify";
        case State::Idle: return "Press play on Spotify";
        case State::PremiumRequired: return "Spotify Premium only";
        case State::Error: return "Check the bridge";
        default: return "";
    }
}

uint32_t MusicPlayer::rawElapsedMs() const {
    uint32_t ms = _positionMs;
    if (_now.playing) ms += millis() - _positionAt;
    return ms;
}

uint32_t MusicPlayer::elapsedMs() const {
    if (!ready()) return 0;
    uint32_t ms = rawElapsedMs();
    if (_now.durationMs && ms > _now.durationMs) ms = _now.durationMs;
    return ms;
}

uint32_t MusicPlayer::elapsedSeconds() const {
    return elapsedMs() / 1000;
}

bool MusicPlayer::trackEnded() const {
    return ready() && _now.playing && _now.durationMs && rawElapsedMs() >= _now.durationMs;
}

void MusicPlayer::rebase() {
    _positionMs = rawElapsedMs();
    if (_now.durationMs && _positionMs > _now.durationMs) _positionMs = _now.durationMs;
    _positionAt = millis();
}

bool MusicPlayer::albumLoaded() const {
    return _album.count > 0 && _now.albumId[0] && strcmp(_album.id, _now.albumId) == 0;
}

uint32_t MusicPlayer::albumSeconds() const {
    uint32_t total = 0;
    for (uint8_t i = 0; i < _album.count; i++) total += _album.tracks[i].seconds;
    return total;
}

void MusicPlayer::findCurrent() {
    _current = -1;
    if (!albumLoaded()) return;
    for (uint8_t i = 0; i < _album.count; i++) {
        if (strcmp(_album.tracks[i].id, _now.id) == 0) {
            _current = i;
            return;
        }
    }
}

void MusicPlayer::clearLyrics() {
    _lyricCount = 0;
    _lyricsId[0] = '\0';
    _lyricsState = LyricsState::Pending;
    _lyricsRevision++;
}

int MusicPlayer::lyricIndex() const {
    if (!ready() || !lyricsLoaded()) return -1;
    uint32_t elapsed = elapsedMs() + lyricLeadMs;
    int index = -1;
    for (uint8_t i = 0; i < _lyricCount; i++) {
        if (_lyrics[i].startMs > elapsed) break;
        index = i;
    }
    return index;
}

void MusicPlayer::setState(State state) {
    if (state == _state) return;
    _state = state;
    _trackRevision++;
    _lyricsRevision++;
    _albumRevision++;
    _revision++;
}

void MusicPlayer::setNow(const MusicNowInfo& info) {
    bool wasReady = ready();
    bool idChanged = strcmp(_now.id, info.id) != 0;
    bool trackChanged = !wasReady || idChanged;
    bool albumChanged = strcmp(_now.albumId, info.albumId) != 0;
    bool playingChanged = _now.playing != info.playing;

    _now = info;
    _positionMs = info.positionMs;
    _positionAt = millis();
    _state = State::Ready;

    if (albumChanged) {
        _albumFailed = false;
        clearCover();
    }
    if (albumChanged || !wasReady) _albumRevision++;
    if (idChanged) clearLyrics();
    if (!wasReady) _lyricsRevision++;
    if (trackChanged) _trackRevision++;
    findCurrent();
    if (trackChanged || playingChanged) _revision++;
}

void MusicPlayer::setAlbum(const MusicAlbumInfo& album) {
    _album = album;
    _albumFailed = false;
    findCurrent();
    _albumRevision++;
    _revision++;
}

void MusicPlayer::setAlbumFailed() {
    if (_albumFailed) return;
    _albumFailed = true;
    _albumRevision++;
}

void MusicPlayer::clearCover() {
    if (!_coverPixels) return;
    _coverPixels = nullptr;
    _coverAlbum[0] = '\0';
    _coverRevision++;
}

void MusicPlayer::setCover(const char* albumId, const uint16_t* pixels) {
    if (strcmp(albumId, _now.albumId) != 0) return;
    strlcpy(_coverAlbum, albumId, sizeof(_coverAlbum));
    _coverPixels = pixels;
    _coverRevision++;
}

void MusicPlayer::setLyrics(const char* trackId, const MusicLyric* blocks, uint8_t count) {
    if (strcmp(trackId, _now.id) != 0) return;
    _lyricCount = count < musicMaxLyrics ? count : musicMaxLyrics;
    for (uint8_t i = 0; i < _lyricCount; i++) _lyrics[i] = blocks[i];
    strlcpy(_lyricsId, trackId, sizeof(_lyricsId));
    _lyricsState = LyricsState::Ready;
    _lyricsRevision++;
}

void MusicPlayer::setLyricsFailed() {
    if (_lyricsState == LyricsState::Failed) return;
    _lyricsState = LyricsState::Failed;
    _lyricsRevision++;
}

void MusicPlayer::push(Command::Type type, uint8_t index) {
    if (_queued == queueSize) return;
    Command& slot = _queue[(_queueHead + _queued) % queueSize];
    slot.type = type;
    slot.index = index;
    strlcpy(slot.albumId, _album.id, sizeof(slot.albumId));
    _queued++;
}

bool MusicPlayer::takeCommand(Command& command) {
    if (_queued == 0) return false;
    command = _queue[_queueHead];
    _queueHead = (_queueHead + 1) % queueSize;
    _queued--;
    return true;
}

void MusicPlayer::togglePlay() {
    if (!ready()) return;
    rebase();
    _now.playing = !_now.playing;
    push(_now.playing ? Command::Play : Command::Pause);
    _revision++;
}

void MusicPlayer::next() {
    if (ready()) push(Command::Next);
}

void MusicPlayer::previous() {
    if (ready()) push(Command::Previous);
}

void MusicPlayer::playTrack(uint8_t index) {
    if (!ready() || !albumLoaded() || index >= _album.count) return;
    push(Command::PlayTrack, index);
}
