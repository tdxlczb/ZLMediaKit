#ifndef ZLM_PLAYER_C_H
#define ZLM_PLAYER_C_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) && defined(_MSC_VER)
    #ifdef zlmplayer_EXPORTS
        #define ZLMPLAYER_API __declspec(dllexport)
    #else
        #define ZLMPLAYER_API __declspec(dllimport)
    #endif
#else
    #define ZLMPLAYER_API
#endif

#if defined(_WIN32) && defined(_MSC_VER)
    #define API_CALL __stdcall
#else
    #define API_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

// C 语言不支持默认值，因此需要显式地设置参数，否则会导致编译错误
typedef struct ZP_PlayOptionsC {
    bool isTcp;
} ZP_PlayOptions;

typedef struct ZP_StreamInfoC {
    int streamIndex;
    int mediaType;
    int codecId; // 使用zlm的CodecId
    int width;
    int height;
    int frameFps;
    int sampleRate;
    int channels;
    int sampleBit; // 采样点位数
    int clockRate; // 时钟频率，用于获取timebase
    uint8_t *extradata; // 流扩展信息
    size_t extrasize;
} ZP_StreamInfo;

typedef struct ZP_PacketC {
    int streamIndex;
    int mediaType;
    bool isKey;
    uint8_t *data;
    size_t size;
    int64_t pts;
    int64_t dts;
    int clockRate; // 时钟频率，用于获取timebase
} ZP_Packet;

enum ZP_PlayStatus {
    None = -1, // 初始状态
    Success,   // 播放成功
    Failed,    // 播放失败
    Stop       // 停止播放
};

typedef void (*OnStream)(ZP_StreamInfo info, void *user);
typedef void (*OnPacket)(ZP_Packet pkt, void *user);
typedef void (*OnPlayStatus)(ZP_PlayStatus status, void *user);
typedef void (*OnLogCallback)(int level, const char *data);

ZLMPLAYER_API void ZP_SetLogCallback(OnLogCallback callback); // 设置日志回调只有第一次生效
ZLMPLAYER_API void *ZP_CreateZlmplayer();
ZLMPLAYER_API void ZP_DeleteZlmplayer(void *pPlayer);
ZLMPLAYER_API void ZP_SetOnStream(void *pPlayer, OnStream callback, void *user);
ZLMPLAYER_API void ZP_SetOnPacket(void *pPlayer, OnPacket callback, void *user);
ZLMPLAYER_API void ZP_SetOnPlayStatus(void *pPlayer, OnPlayStatus callback, void *user);
ZLMPLAYER_API bool ZP_Play(void *pPlayer, const char *url, ZP_PlayOptions options);
ZLMPLAYER_API void ZP_Stop(void *pPlayer);
ZLMPLAYER_API void ZP_Pause(void *pPlayer);
ZLMPLAYER_API void ZP_Resume(void *pPlayer);
ZLMPLAYER_API void ZP_Seek(void *pPlayer, int seconds);
ZLMPLAYER_API void ZP_Speed(void *pPlayer, double speed);
ZLMPLAYER_API ZP_StreamInfo ZP_GetVideoStream(void *pPlayer);
ZLMPLAYER_API ZP_StreamInfo ZP_GetAudioStream(void *pPlayer);

#ifdef __cplusplus
}
#endif

#endif // ZLM_PLAYER_C_H