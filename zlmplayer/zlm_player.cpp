#include "zlm_player.h"
#include "zlm_player_impl.h"

namespace zlmplayer {

ZlmPlayer::ZlmPlayer()
    : m_impl(std::make_unique<ZlmPlayerImpl>()) {}

ZlmPlayer::~ZlmPlayer() {}

void ZlmPlayer::SetOnPacket(OnPacket callback) {
    m_impl->SetOnPacket(callback);
}

void ZlmPlayer::SetOnPlayStatus(OnPlayStatus callback) {
    m_impl->SetOnPlayStatus(callback);
}

bool ZlmPlayer::Play(const std::string &url, const PlayOptions &options) {
    return m_impl->Play(url, options);
}

void ZlmPlayer::Stop() {
    m_impl->Stop();
}

void ZlmPlayer::Pause() {
    m_impl->Pause();
}

void ZlmPlayer::Resume() {
    m_impl->Resume();
}

void ZlmPlayer::Seek(int seconds) {
    m_impl->Seek(seconds);
}

void ZlmPlayer::Speed(double speed) {
    m_impl->Speed(speed);
}

StreamInfo ZlmPlayer::GetVideoStream() {
    return m_impl->GetVideoStream();
}

StreamInfo ZlmPlayer::GetAudioStream() {
    return m_impl->GetAudioStream();
}

} // namespace zlmplayer