#pragma once
#include "zlm_player.h"
#include <condition_variable>
#include <memory>
#include <string>

namespace zlmplayer {

class RtspPlayerImpl;
class ZlmPlayerImpl {
public:
    ZlmPlayerImpl();
    ~ZlmPlayerImpl();

    void SetOnPacket(OnPacket callback);
    void SetOnPlayStatus(OnPlayStatus callback);
    bool Play(const std::string &url, const PlayOptions &options);
    void Stop();
    void Pause();
    void Resume();
    void Seek(int seconds);
    void Speed(double speed);
    StreamInfo GetVideoStream();
    StreamInfo GetAudioStream();

private:
    bool StreamOpen(const std::string &url, const PlayOptions &options);
    void StreamClose();

private:
    OnPacket m_onPacket = nullptr;
    OnPlayStatus m_onPlayStatus = nullptr;
    std::shared_ptr<RtspPlayerImpl> m_rtspPlayerImpl;
    PlayStatus m_playStatus = PlayStatus::None;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    StreamInfo m_videoStream;
    StreamInfo m_audioStream;
};

} // namespace zlmplayer
