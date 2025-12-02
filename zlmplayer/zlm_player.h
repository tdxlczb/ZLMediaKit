#pragma once

#if defined(_WIN32)
#ifdef zlmplayer_EXPORTS
#define ZPLIB_API __declspec(dllexport)
#else
#define ZPLIB_API __declspec(dllimport)
#endif
#else
#define ZPLIB_API __attribute__((visibility("default")))
#endif

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

/*
* zlm中CodecId的枚举值如下
*
    XX(CodecH264,  TrackVideo, 0, "H264", PSI_STREAM_H264, MOV_OBJECT_H264)          \
    XX(CodecH265,  TrackVideo, 1, "H265", PSI_STREAM_H265, MOV_OBJECT_HEVC)          \
    XX(CodecAAC,   TrackAudio, 2, "mpeg4-generic", PSI_STREAM_AAC, MOV_OBJECT_AAC)   \
    XX(CodecG711A, TrackAudio, 3, "PCMA", PSI_STREAM_AUDIO_G711A, MOV_OBJECT_G711a)  \
    XX(CodecG711U, TrackAudio, 4, "PCMU", PSI_STREAM_AUDIO_G711U, MOV_OBJECT_G711u)  \
    XX(CodecOpus,  TrackAudio, 5, "opus", PSI_STREAM_AUDIO_OPUS, MOV_OBJECT_OPUS)    \
    XX(CodecL16,   TrackAudio, 6, "L16", PSI_STREAM_RESERVED, MOV_OBJECT_NONE)       \
    XX(CodecVP8,   TrackVideo, 7, "VP8", PSI_STREAM_VP8, MOV_OBJECT_VP8)             \
    XX(CodecVP9,   TrackVideo, 8, "VP9", PSI_STREAM_VP9, MOV_OBJECT_VP9)             \
    XX(CodecAV1,   TrackVideo, 9, "AV1", PSI_STREAM_AV1, MOV_OBJECT_AV1)             \
    XX(CodecJPEG,  TrackVideo, 10, "JPEG", PSI_STREAM_JPEG_2000, MOV_OBJECT_JPEG)    \
    XX(CodecH266,  TrackVideo, 11, "H266", PSI_STREAM_H266, MOV_OBJECT_H266)         \
    XX(CodecTS,    TrackVideo, 12, "MP2T", PSI_STREAM_RESERVED, MOV_OBJECT_NONE)     \
    XX(CodecPS,    TrackVideo, 13, "MPEG", PSI_STREAM_RESERVED, MOV_OBJECT_NONE)     \
    XX(CodecMP3,   TrackAudio, 14, "MP3",  PSI_STREAM_MP3, MOV_OBJECT_MP3)           \
    XX(CodecADPCM, TrackAudio, 15, "ADPCM", PSI_STREAM_RESERVED, MOV_OBJECT_NONE)    \
    XX(CodecSVACV, TrackVideo, 16, "SVACV", PSI_STREAM_VIDEO_SVAC, MOV_OBJECT_NONE)  \
    XX(CodecSVACA, TrackAudio, 17, "SVACA", PSI_STREAM_AUDIO_SVAC, MOV_OBJECT_NONE)  \
    XX(CodecG722,  TrackAudio, 18, "G722", PSI_STREAM_AUDIO_G722, MOV_OBJECT_NONE)   \
    XX(CodecG723,  TrackAudio, 19, "G723", PSI_STREAM_AUDIO_G723, MOV_OBJECT_NONE)   \
    XX(CodecG728,  TrackAudio, 20, "G728", PSI_STREAM_RESERVED, MOV_OBJECT_NONE)     \
    XX(CodecG729,  TrackAudio, 21, "G729", PSI_STREAM_AUDIO_G729, MOV_OBJECT_NONE)
*/

namespace zlmplayer {

using PlayOptions = std::map<std::string, std::string>;
/*
 * rtsp_transport=tcp
 */

using StreamType = int;
const StreamType kStreamNone = -1;
const StreamType kStreamVideo = 0;
const StreamType kStreamAudio = 1;

using CodecId = int;

struct StreamInfo {
    StreamType type = kStreamNone;
    CodecId codecId = -1; // 使用zlm的CodecId
    int width = 0;
    int height = 0;
    int frameFps = 0;
    int sampleRate = 0;
    int channels = 0;
    int clockRate = 0; //时钟频率，用于获取timebase
};

struct Packet {
    bool isAudio = false;
    bool isKey = false;
    uint8_t *data = nullptr;
    size_t size = 0;
    int64_t pts = 0;
    int64_t dts = 0;
};

enum class PlayStatus {
    None = -1, //初始状态
    Success = 0, // 播放成功
    Failed, // 播放失败
    Stop // 停止播放
};

using OnPacket = std::function<void(const Packet &pkt)>;
using OnPlayStatus = std::function<void(PlayStatus status)>;

class ZlmPlayerImpl;

class ZPLIB_API ZlmPlayer {
public:
    ZlmPlayer();
    ~ZlmPlayer();

    void SetOnPacket(OnPacket callback);
    void SetOnPlayStatus(OnPlayStatus callback);
    bool Play(const std::string &url, const PlayOptions &options = PlayOptions());
    void Stop();
    void Pause();
    void Resume();
    void Seek(int seconds);
    void Speed(double speed);
    StreamInfo GetVideoStream();
    StreamInfo GetAudioStream();

private:
    std::unique_ptr<ZlmPlayerImpl> m_impl;
};

} // namespace zlmplayer
