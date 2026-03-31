#include "zlm_player_impl.h"
#include "rtsp_player_impl.h"

class CallbackChannel : public toolkit::LogChannel {
public:
    CallbackChannel(const std::string &name = "CallbackChannel", toolkit::LogLevel level = toolkit::LogLevel::LTrace);
    ~CallbackChannel() override = default;
    void write(const toolkit::Logger &logger, const toolkit::LogContextPtr &logContext) override;
    void setCallback(LogCallback callback);

private:
    LogCallback _callback;
};

CallbackChannel::CallbackChannel(const std::string &name, toolkit::LogLevel level)
    : LogChannel(name, level)
    , _callback(NULL) {}

void CallbackChannel::write(const toolkit::Logger &logger, const toolkit::LogContextPtr &ctx) {
    std::ostringstream oss;
    format(logger, oss, ctx);
    if (_callback) {
        _callback(ctx->_level, oss.str().c_str());
    }
}

void CallbackChannel::setCallback(LogCallback callback) {
    //只设置一次
    if (!_callback) {
        _callback = callback;
    }
}

void SetLogCallback(LogCallback callback) {
    if (callback) {
        auto chanel = std::make_shared<CallbackChannel>();
        chanel->setCallback(callback);
        // Logger会添加一个Trace的默认日志通道，并且该日志通道无法更改level，想要控制日志等级，必须主动添加一个日志通道
        toolkit::Logger::Instance().add(chanel);
    }
}

namespace zlmplayer {

ZlmPlayerImpl::ZlmPlayerImpl() {
    m_packetBuf = std::unique_ptr<RawBuffer>(new RawBuffer());
    m_videoExtraBuf = std::unique_ptr<RawBuffer>(new RawBuffer());
    m_audioExtraBuf = std::unique_ptr<RawBuffer>(new RawBuffer());

}

ZlmPlayerImpl::~ZlmPlayerImpl() {}

void ZlmPlayerImpl::SetOnStream(OnStream callback) {
    m_onStream = callback;
}

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
    if (m_playStatus.load() != PlayStatus::Success || !m_rtspPlayerImpl)
        return;
    m_rtspPlayerImpl->pause(true);
}

void ZlmPlayerImpl::Resume() {
    if (m_playStatus.load() != PlayStatus::Success || !m_rtspPlayerImpl)
        return;
    m_rtspPlayerImpl->pause(false);
}

void ZlmPlayerImpl::Seek(int seconds) {
    if (m_playStatus.load() != PlayStatus::Success || !m_rtspPlayerImpl)
        return;
    m_rtspPlayerImpl->seekTo((uint32_t)seconds);
}

void ZlmPlayerImpl::Speed(double speed) {
    if (m_playStatus.load() != PlayStatus::Success || !m_rtspPlayerImpl)
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
            if (m_onPlayStatus)
                m_onPlayStatus((PlayStatus)(m_playStatus.load()));
            return;
        }
        m_playStatus = PlayStatus::Success;
        m_cv.notify_all();
        if (m_onPlayStatus)
            m_onPlayStatus((PlayStatus)(m_playStatus.load()));
    });

    m_rtspPlayerImpl->setOnShutdown([this](const toolkit::SockException &ex) {
        ErrorL << "OnShutdown:" << ex.what();
        m_playStatus = PlayStatus::Stop;
        if (m_onPlayStatus)
            m_onPlayStatus((PlayStatus)(m_playStatus.load()));
    });

    if (options.isTcp) {
        // RTP transport over TCP
        (*m_rtspPlayerImpl)[mediakit::Client::kRtpType] = mediakit::Rtsp::RTP_TCP;
    } else {
        (*m_rtspPlayerImpl)[mediakit::Client::kRtpType] = mediakit::Rtsp::RTP_UDP;
    }
    m_rtspPlayerImpl->play(url);

    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait_for(lock, std::chrono::milliseconds(5000), [this]() { return m_playStatus.load() != zlmplayer::PlayStatus::None; });

    if (m_playStatus.load() == zlmplayer::PlayStatus::Success) {
        CreateStream();
        return true;
    }
    return false;
}

void ZlmPlayerImpl::StreamClose() {
    if (m_rtspPlayerImpl) {
        m_rtspPlayerImpl->teardown();
    }
    m_playStatus = PlayStatus::None;
    m_rtspPlayerImpl = nullptr;
}

void ZlmPlayerImpl::CreateStream() {

    auto videoTrack = std::dynamic_pointer_cast<mediakit::VideoTrack>(m_rtspPlayerImpl->getTrack(mediakit::TrackVideo));
    if (videoTrack) {
        m_videoStream.streamIndex = 0;
        m_videoStream.mediaType = kMediaVideo;
        m_videoStream.codecId = videoTrack->getCodecId();
        m_videoStream.width = videoTrack->getVideoWidth();
        m_videoStream.height = videoTrack->getVideoHeight();
        m_videoStream.frameFps = videoTrack->getVideoFps();
        m_videoStream.clockRate = m_rtspPlayerImpl->getVideoClockRate();

        m_videoExtraBuf->clear();
        auto configs = videoTrack->getConfigFrames();
        for (size_t i = 0; i < configs.size(); i++) {
            m_videoExtraBuf->push((uint8_t *)configs[i]->data(), configs[i]->size());
        }
        if (m_videoExtraBuf->size() <= 0) {
            auto extra = videoTrack->getExtraData();
            m_videoExtraBuf->push((uint8_t *)extra->data(), extra->size());
        }
        m_videoStream.extradata = m_videoExtraBuf->data();
        m_videoStream.extrasize = m_videoExtraBuf->size();

        if (m_onStream)
            m_onStream(m_videoStream);

        videoTrack->addDelegate([this](const mediakit::Frame::Ptr &frame) {
            if (frame && m_onPacket) {
                uint8_t *data = (uint8_t *)frame->data();
                size_t size = frame->size();
                bool isKey = frame->keyFrame();
                // 可丢弃的帧
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
                pkt.streamIndex = m_videoStream.streamIndex;
                pkt.mediaType = m_videoStream.mediaType;
                pkt.isKey = isKey;
                pkt.data = data;
                pkt.size = size;
                pkt.dts = frame->dts();
                pkt.pts = frame->pts();
                pkt.clockRate = m_videoStream.clockRate;
                m_onPacket(pkt);

                m_packetBuf->clear();
                m_packetPts = 0;
            }
            return true;
        });
    }

    auto audioTrack = std::dynamic_pointer_cast<mediakit::AudioTrack>(m_rtspPlayerImpl->getTrack(mediakit::TrackAudio));
    if (audioTrack) {
        m_audioStream.streamIndex = m_audioStream.streamIndex;
        m_audioStream.mediaType = m_audioStream.mediaType;
        m_audioStream.codecId = audioTrack->getCodecId();
        m_audioStream.sampleRate = audioTrack->getAudioSampleRate();
        m_audioStream.channels = audioTrack->getAudioChannel();
        m_audioStream.sampleBit = audioTrack->getAudioSampleBit();
        m_audioStream.clockRate = m_audioStream.sampleRate;

        m_audioExtraBuf->clear();
        auto extra = audioTrack->getExtraData();
        m_audioExtraBuf->push((uint8_t *)extra->data(), extra->size());
        m_audioStream.extradata = m_audioExtraBuf->data();
        m_audioStream.extrasize = m_audioExtraBuf->size();
        if (m_onStream)
            m_onStream(m_audioStream);

        audioTrack->addDelegate([this](const mediakit::Frame::Ptr &frame) {
            if (frame && m_onPacket) {
                Packet pkt;
                pkt.streamIndex = 1;
                pkt.mediaType = kMediaAudio;
                pkt.isKey = frame->keyFrame();
                pkt.data = (uint8_t *)frame->data();
                pkt.size = frame->size();
                pkt.dts = frame->dts();
                pkt.pts = frame->pts();
                pkt.clockRate = m_audioStream.clockRate;
                m_onPacket(pkt);
            }
            return true;
        });
    }
}

} // namespace zlmplayer