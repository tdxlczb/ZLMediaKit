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

typedef struct ZP_PlayOptionsC {
    bool isTcp = false;
} ZP_PlayOptions;

typedef struct ZP_StreamInfoC {
    int mediaType = -1;
    int codecId = -1; // 使用zlm的CodecId
    int width = 0;
    int height = 0;
    int frameFps = 0;
    int sampleRate = 0;
    int channels = 0;
    int sampleBit = 0; // 采样点位数
    int clockRate = 0; // 时钟频率，用于获取timebase
} ZP_StreamInfo;

typedef struct ZP_PacketC {
    int mediaType = -1;
    bool isKey = false;
    uint8_t *data = nullptr;
    size_t size = 0;
    int64_t pts = 0;
    int64_t dts = 0;
    int clockRate = 0; // 时钟频率，用于获取timebase
} ZP_Packet;

enum ZP_PlayStatus {
    None = -1, // 初始状态
    Success,   // 播放成功
    Failed,    // 播放失败
    Stop       // 停止播放
};

typedef void(API_CALL *OnPacket)(ZP_Packet pkt, void *user);
typedef void(API_CALL *OnPlayStatus)(ZP_PlayStatus status, void *user);

ZLMPLAYER_API void *API_CALL ZP_CreateZlmplayer();
ZLMPLAYER_API void API_CALL ZP_DeleteZlmplayer(void *pPlayer);
ZLMPLAYER_API void API_CALL ZP_SetOnPacket(void *pPlayer, OnPacket callback, void *user);
ZLMPLAYER_API void API_CALL ZP_SetOnPlayStatus(void *pPlayer, OnPlayStatus callback, void *user);
ZLMPLAYER_API bool API_CALL ZP_Play(void *pPlayer, const char *url, ZP_PlayOptions options);
ZLMPLAYER_API void API_CALL ZP_Stop(void *pPlayer);
ZLMPLAYER_API void API_CALL ZP_Pause(void *pPlayer);
ZLMPLAYER_API void API_CALL ZP_Resume(void *pPlayer);
ZLMPLAYER_API void API_CALL ZP_Seek(void *pPlayer, int seconds);
ZLMPLAYER_API void API_CALL ZP_Speed(void *pPlayer, double speed);
ZLMPLAYER_API ZP_StreamInfo API_CALL ZP_GetVideoStream(void *pPlayer);
ZLMPLAYER_API ZP_StreamInfo API_CALL ZP_GetAudioStream(void *pPlayer);

#ifdef __cplusplus
}
#endif

#endif // ZLM_PLAYER_C_H