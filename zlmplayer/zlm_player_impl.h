#pragma once
#include "zlm_player.h"
#include <condition_variable>
#include <memory>
#include <string>

namespace zlmplayer {

class RawBuffer {
public:
    RawBuffer() {
        m_capacity = 1024 * 1024;
        m_buf = new uint8_t[m_capacity];
        memset(m_buf, 0, m_capacity);
    };
    ~RawBuffer() {
        delete[] m_buf;
        m_buf = nullptr;
    };

    void push(uint8_t *data, size_t size) {
        size_t new_size = m_size + size;
        if (m_capacity < new_size) {
            m_capacity = new_size;
            uint8_t *buf = new uint8_t[m_capacity];
            memcpy(buf, m_buf, m_size);
            delete[] m_buf;
            m_buf = buf;
        }
        memcpy(m_buf + m_size, data, size);
        m_size = new_size;
    }

    uint8_t *data() { return m_buf; }
    size_t size() { return m_size; }
    void clear() {
        memset(m_buf, 0, m_capacity);
        m_size = 0;
    }

private:
    uint8_t *m_buf = nullptr;
    size_t m_size = 0;
    size_t m_capacity = 0;
};

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

    std::unique_ptr<RawBuffer> m_packetBuf;
    uint64_t m_packetPts = 0;
};

} // namespace zlmplayer
