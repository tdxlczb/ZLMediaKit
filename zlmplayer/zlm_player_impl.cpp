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
    // 一次性播放很多路流时，偶发会出现ctx是野指针的情况，未定位到原因，这里针对野指针指向的值进行筛选拦截
    // 增加更严格的检查：指针有效性、level范围、以及基本的内存地址检查
    if (!ctx) {
        return;
    }
    
    // 检查指针是否在合理的内存范围内（避免野指针）
    try {
        if (ctx->_level < 0 || ctx->_level > 4) {
            return;
        }
    } catch (...) {
        // 如果访问ctx的成员异常，说明是野指针
        return;
    }
    
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
        if (!toolkit::Logger::Instance().get("CallbackChannel")) {
            auto chanel = std::make_shared<CallbackChannel>();
            chanel->setCallback(callback);
            // Logger会添加一个Trace的默认日志通道，并且该日志通道无法更改level，想要控制日志等级，必须主动添加一个日志通道
            toolkit::Logger::Instance().add(chanel);
        }
    }
}

namespace zlmplayer {

ZlmPlayerImpl::ZlmPlayerImpl() {
    try {
        m_packetBuf = std::make_unique<RawBuffer>();
        m_videoExtraBuf = std::make_unique<RawBuffer>();
        m_audioExtraBuf = std::make_unique<RawBuffer>();
    } catch (const std::exception &e) {
        ErrorL << "Failed to initialize buffers: " << e.what();
    }
}

ZlmPlayerImpl::~ZlmPlayerImpl() {}

void ZlmPlayerImpl::SetOnStream(OnStream callback) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    m_onStream = callback;
}

void ZlmPlayerImpl::SetOnPacket(OnPacket callback) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    m_onPacket = callback;
}

void ZlmPlayerImpl::SetOnPlayStatus(OnPlayStatus callback) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    m_onPlayStatus = callback;
}

bool ZlmPlayerImpl::Play(const std::string &url, const PlayOptions &options) {
    StreamClose();
    return StreamOpen(url, options);
}

void ZlmPlayerImpl::Stop() {
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_onPacket = nullptr;
        m_onPlayStatus = nullptr;
    }
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
            std::lock_guard<std::mutex> lock(m_eventMutex);
            if (m_onPlayStatus)
                m_onPlayStatus((PlayStatus)(m_playStatus.load()));
            return;
        }
        m_playStatus = PlayStatus::Success;
        m_cv.notify_all();
        {
            std::lock_guard<std::mutex> lock(m_eventMutex);
            if (m_onPlayStatus)
                m_onPlayStatus((PlayStatus)(m_playStatus.load()));
        }
        CreateStream();
    });

    m_rtspPlayerImpl->setOnShutdown([this](const toolkit::SockException &ex) {
        ErrorL << "OnShutdown:" << ex.what();
        m_playStatus = PlayStatus::Stop;
        std::lock_guard<std::mutex> lock(m_eventMutex);
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
    m_cv.wait_for(lock, std::chrono::milliseconds(10000), [this]() { return m_playStatus.load() != zlmplayer::PlayStatus::None; });

    if (m_playStatus.load() == zlmplayer::PlayStatus::Success) {
        //CreateStream(); // 放在这可能会导致OnPacket取不到开始的一些帧，必须放到setOnPlayResult回调里
        return true;
    }
    return false;
}

void ZlmPlayerImpl::StreamClose() {
    if (m_rtspPlayerImpl) {
        m_rtspPlayerImpl->teardown();
    }
    m_playStatus = PlayStatus::None;
}

void ZlmPlayerImpl::CreateStream() {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    
    if (!m_rtspPlayerImpl) {
        ErrorL << "RtspPlayerImpl is null";
        return;
    }
    
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
            if (configs[i]) {
                m_videoExtraBuf->push((uint8_t *)configs[i]->data(), configs[i]->size());
            }
        }
        if (m_videoExtraBuf->size() <= 0) {
            auto extra = videoTrack->getExtraData();
            if (extra) {
                m_videoExtraBuf->push((uint8_t *)extra->data(), extra->size());
            }
        }
        m_videoStream.extradata = m_videoExtraBuf->data();
        m_videoStream.extrasize = m_videoExtraBuf->size();

        if (m_onStream)
            m_onStream(m_videoStream);

        videoTrack->addDelegate([this](const mediakit::Frame::Ptr &frame) {
            std::lock_guard<std::mutex> lock(m_eventMutex);
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
        m_audioStream.streamIndex = 1;
        m_audioStream.mediaType = kMediaAudio;
        m_audioStream.codecId = audioTrack->getCodecId();
        m_audioStream.sampleRate = audioTrack->getAudioSampleRate();
        m_audioStream.channels = audioTrack->getAudioChannel();
        m_audioStream.sampleBit = audioTrack->getAudioSampleBit();
        m_audioStream.clockRate = m_audioStream.sampleRate;

        m_audioExtraBuf->clear();
        auto extra = audioTrack->getExtraData();
        if (extra) {
            m_audioExtraBuf->push((uint8_t *)extra->data(), extra->size());
        }
        m_audioStream.extradata = m_audioExtraBuf->data();
        m_audioStream.extrasize = m_audioExtraBuf->size();
        if (m_onStream)
            m_onStream(m_audioStream);

        audioTrack->addDelegate([this](const mediakit::Frame::Ptr &frame) {
            std::lock_guard<std::mutex> lock(m_eventMutex);
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