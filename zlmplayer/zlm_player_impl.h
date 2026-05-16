#pragma once
#include "zlm_player.h"
#include <condition_variable>
#include <cstring>
#include <memory>
#include <vector>

namespace zlmplayer {

class RawBuffer {
public:
    RawBuffer() {
        m_buf.clear();
        m_buf.reserve(1024 * 1024);
    };
    ~RawBuffer() {
        m_buf.clear();
        m_buf.shrink_to_fit();
    };

    // 禁止拷贝，防止多线程误操作
    RawBuffer(const RawBuffer &) = delete;
    RawBuffer &operator=(const RawBuffer &) = delete;

    bool push(const uint8_t *data, int len) {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (!data || len <= 0)
            return false;
        // 追加到现有数据后面
        m_buf.insert(m_buf.end(), data, data + len);
        return true;
    }

    uint8_t *data() {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_buf.data();
    }
    size_t size() {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_buf.size();
    }
    void clear() {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_buf.size() > 0)
            m_buf.clear();
    }

private:
    mutable std::mutex m_mtx;
    std::vector<uint8_t> m_buf;
};

class RtspPlayerImpl;
class ZlmPlayerImpl {
public:
    ZlmPlayerImpl();
    ~ZlmPlayerImpl();

    void SetOnStream(OnStream callback);
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
    void CreateStream();

private:
    OnStream m_onStream = nullptr;
    OnPacket m_onPacket = nullptr;
    OnPlayStatus m_onPlayStatus = nullptr;
    std::shared_ptr<RtspPlayerImpl> m_rtspPlayerImpl;
    std::atomic_int m_playStatus = PlayStatus::None;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::mutex m_eventMutex;
    StreamInfo m_videoStream;
    StreamInfo m_audioStream;
    std::unique_ptr<RawBuffer> m_videoExtraBuf;
    std::unique_ptr<RawBuffer> m_audioExtraBuf;

    std::unique_ptr<RawBuffer> m_packetBuf;
    uint64_t m_packetPts = 0;
};

} // namespace zlmplayer
