#include "zlm_player_impl.h"
#include "rtsp_player_impl.h"

namespace zlmplayer {

ZlmPlayerImpl::ZlmPlayerImpl()
    : m_packetBuf(std::make_unique<RawBuffer>()) {}

ZlmPlayerImpl::~ZlmPlayerImpl() {}

void ZlmPlayerImpl::SetOnPacket(OnPacket callback) {
    m_onPacket = callback;
}

void ZlmPlayerImpl::SetOnPlayStatus(OnPlayStatus callback) {
    m_onPlayStatus = callback;
}

bool ZlmPlayerImpl::Play(const std::string &url, const PlayOptions &options) {
    StreamClose();
    return StreamOpen(url, options);
}

void ZlmPlayerImpl::Stop() {
    m_onPacket = nullptr;
    m_onPlayStatus = nullptr;
    StreamClose();
}

void ZlmPlayerImpl::Pause() {
    if (m_playStatus != PlayStatus::Success || !m_rtspPlayerImpl)
        return;
    m_rtspPlayerImpl->pause(true);
}

void ZlmPlayerImpl::Resume() {
    if (m_playStatus != PlayStatus::Success || !m_rtspPlayerImpl)
        return;
    m_rtspPlayerImpl->pause(false);
}

void ZlmPlayerImpl::Seek(int seconds) {
    if (m_playStatus != PlayStatus::Success || !m_rtspPlayerImpl)
        return;
    m_rtspPlayerImpl->seekTo((uint32_t)seconds);
}

void ZlmPlayerImpl::Speed(double speed) {
    if (m_playStatus != PlayStatus::Success || !m_rtspPlayerImpl)
        return;
    m_rtspPlayerImpl->speed(speed);
}

StreamInfo ZlmPlayerImpl::GetVideoStream() {
    return m_videoStream;
}

StreamInfo ZlmPlayerImpl::GetAudioStream() {
    return m_audioStream;
}

bool ZlmPlayerImpl::StreamOpen(const std::string &url, const PlayOptions &options) {
    auto poller = toolkit::EventPollerPool::Instance().getPoller();
    m_rtspPlayerImpl = std::make_shared<RtspPlayerImpl>(poller);
    m_rtspPlayerImpl->setOnPlayResult([this](const toolkit::SockException &ex) {
        InfoL << "OnPlayResult:" << ex.what();
        if (ex) {
            m_playStatus = PlayStatus::Failed;
            return;
        }

        auto videoTrack = std::dynamic_pointer_cast<mediakit::VideoTrack>(m_rtspPlayerImpl->getTrack(mediakit::TrackVideo));
        if (videoTrack) {
            m_videoStream.type = kStreamVideo;
            m_videoStream.codecId = videoTrack->getCodecId();
            m_videoStream.width = videoTrack->getVideoWidth();
            m_videoStream.height = videoTrack->getVideoHeight();
            m_videoStream.frameFps = videoTrack->getVideoFps();
            m_videoStream.clockRate = m_rtspPlayerImpl->getVideoClockRate();
            videoTrack->addDelegate([this](const mediakit::Frame::Ptr &frame) {
                if (frame && m_onPacket) {
                    uint8_t *data = (uint8_t *)frame->data();
                    size_t size = frame->size();
                    bool isKey = frame->keyFrame();
                    //可丢弃的帧
                    if (frame->dropAble()) {
                        return true;
                    }
                    // 配置帧，譬如sps pps vps需要和关键帧一起发送，否则解码器解码可能会出现异常
                    if (frame->configFrame()) {
                        m_packetPts = frame->pts();
                        m_packetBuf->push(data, size);
                        return true;
                    } 
                    if (m_packetBuf->size() > 0 && (frame->pts() == m_packetPts)) {
                        m_packetPts = frame->pts();
                        m_packetBuf->push(data, size);
                        data = m_packetBuf->data();
                        size = m_packetBuf->size();
                    }
                    Packet pkt;
                    pkt.isAudio = false;
                    pkt.isKey = isKey;
                    pkt.data = data;
                    pkt.size = size;
                    pkt.dts = frame->dts();
                    pkt.pts = frame->pts();
                    m_onPacket(pkt);

                    m_packetBuf->clear();
                    m_packetPts = 0;
                }
                return true;
            });
        }

        auto audioTrack = std::dynamic_pointer_cast<mediakit::AudioTrack>(m_rtspPlayerImpl->getTrack(mediakit::TrackAudio));
        if (audioTrack) {
            m_audioStream.type = kStreamAudio;
            m_audioStream.codecId = audioTrack->getCodecId();
            m_audioStream.sampleRate = audioTrack->getAudioSampleRate();
            m_audioStream.channels = audioTrack->getAudioChannel();
            m_audioStream.sampleBit = audioTrack->getAudioSampleBit();
            m_audioStream.clockRate = m_audioStream.sampleRate;
            audioTrack->addDelegate([this](const mediakit::Frame::Ptr &frame) {
                if (frame && m_onPacket) {
                    Packet pkt;
                    pkt.isAudio = true;
                    pkt.isKey = frame->keyFrame();
                    pkt.data = (uint8_t *)frame->data();
                    pkt.size = frame->size();
                    pkt.dts = frame->dts();
                    pkt.pts = frame->pts();
                    m_onPacket(pkt);
                }
                return true;
            });
        }

        m_playStatus = PlayStatus::Success;
        if (m_onPlayStatus)
            m_onPlayStatus(m_playStatus);
    });

    m_rtspPlayerImpl->setOnShutdown([this](const toolkit::SockException &ex) {
        ErrorL << "OnShutdown:" << ex.what();
        m_playStatus = PlayStatus::Stop;
        if (m_onPlayStatus)
            m_onPlayStatus(m_playStatus);
    });

    auto tempOptions = options;
    if (tempOptions["rtsp_transport"] == "tcp") {
        // RTP transport over TCP
        (*m_rtspPlayerImpl)[mediakit::Client::kRtpType] = mediakit::Rtsp::RTP_TCP;
    } else {
        (*m_rtspPlayerImpl)[mediakit::Client::kRtpType] = mediakit::Rtsp::RTP_UDP;
    }
    m_rtspPlayerImpl->play(url);
    return true;
}

void ZlmPlayerImpl::StreamClose() {
    if (m_rtspPlayerImpl) {
        m_rtspPlayerImpl->teardown();
    }
    m_playStatus = PlayStatus::None;
    m_rtspPlayerImpl = nullptr;
}

} // namespace zlmplayer