#include "zlm_player_c.h"
#include "zlm_player.h"

static void InitStreamInfo(ZP_StreamInfo &info) 
{
    info.streamIndex = -1;
    info.mediaType = -1;
    info.codecId = -1;
    info.width = 0;
    info.height = 0;
    info.frameFps = 0;
    info.sampleRate = 0;
    info.channels = 0;
    info.sampleBit = 0;
    info.clockRate = 0;
    info.extradata = nullptr;
    info.extrasize = 0;
}

static void InitPacket(ZP_Packet &pkt) {
    pkt.streamIndex = -1;
    pkt.mediaType = -1;
    pkt.isKey = false;
    pkt.data = nullptr;
    pkt.size = 0;
    pkt.pts = 0;
    pkt.dts = 0;
    pkt.clockRate = 0;
}

void ZP_SetLogCallback(OnLogCallback callback) {
    SetLogCallback(callback);
}

void *ZP_CreateZlmplayer() {
    zlmplayer::ZlmPlayer *impl = new zlmplayer::ZlmPlayer();
    return impl;
}

void ZP_DeleteZlmplayer(void *pPlayer) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    delete impl;
    impl = nullptr;
}

void ZP_SetOnStream(void *pPlayer, OnStream callback, void *user) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->SetOnStream([callback, user](const zlmplayer::StreamInfo &sinfo) {
        ZP_StreamInfo info;
        info.streamIndex = sinfo.streamIndex;
        info.mediaType = sinfo.mediaType;
        info.codecId = sinfo.codecId;
        info.width = sinfo.width;
        info.height = sinfo.height;
        info.frameFps = sinfo.frameFps;
        info.sampleRate = sinfo.sampleRate;
        info.channels = sinfo.channels;
        info.sampleBit = sinfo.sampleBit;
        info.clockRate = sinfo.clockRate;
        info.extradata = sinfo.extradata; // 注意，sinfo是局部变量，extradata不能使用string的data()，否则调用该接口获取的extradata可能会数据异常
        info.extrasize = sinfo.extrasize;
        callback(info, user);
    });
}

void ZP_SetOnPacket(void *pPlayer, OnPacket callback, void *user) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->SetOnPacket([callback, user](const zlmplayer::Packet &pkt) {
        ZP_Packet cpkt;
        cpkt.streamIndex = pkt.streamIndex;
        cpkt.mediaType = pkt.mediaType;
        cpkt.isKey = pkt.isKey;
        cpkt.data = pkt.data;
        cpkt.size = pkt.size;
        cpkt.pts = pkt.pts;
        cpkt.dts = pkt.dts;
        cpkt.clockRate = pkt.clockRate;
        callback(cpkt, user);
    });
}

void ZP_SetOnPlayStatus(void *pPlayer, OnPlayStatus callback, void *user) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->SetOnPlayStatus([callback, user](zlmplayer::PlayStatus status) { callback((ZP_PlayStatus)status, user); });
}

bool ZP_Play(void *pPlayer, const char *url, ZP_PlayOptions options) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return false;
    zlmplayer::PlayOptions opts;
    opts.isTcp = options.isTcp;
    return impl->Play(url, opts);
}

void ZP_Stop(void *pPlayer) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->Stop();
}

void ZP_Pause(void *pPlayer) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->Pause();
}

void ZP_Resume(void *pPlayer) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->Resume();
}

void ZP_Seek(void *pPlayer, int seconds) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->Seek(seconds);
}

void ZP_Speed(void *pPlayer, double speed) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->Speed(speed);
}

ZP_StreamInfo ZP_GetVideoStream(void *pPlayer) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    ZP_StreamInfo info;
    InitStreamInfo(info);
    if (!impl)
        return info;
    zlmplayer::StreamInfo sinfo = impl->GetVideoStream();
    info.streamIndex = sinfo.streamIndex;
    info.mediaType = sinfo.mediaType;
    info.codecId = sinfo.codecId;
    info.width = sinfo.width;
    info.height = sinfo.height;
    info.frameFps = sinfo.frameFps;
    info.sampleRate = sinfo.sampleRate;
    info.channels = sinfo.channels;
    info.sampleBit = sinfo.sampleBit;
    info.clockRate = sinfo.clockRate;
    info.extradata = sinfo.extradata; // 注意，sinfo是局部变量，extradata不能使用string的data()，否则调用该接口获取的extradata可能会数据异常
    info.extrasize = sinfo.extrasize;
    return info;
}

ZP_StreamInfo ZP_GetAudioStream(void *pPlayer) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    ZP_StreamInfo info;
    InitStreamInfo(info);

    if (!impl)
        return info;
    zlmplayer::StreamInfo sinfo = impl->GetAudioStream();
    info.streamIndex = sinfo.streamIndex;
    info.mediaType = sinfo.mediaType;
    info.codecId = sinfo.codecId;
    info.width = sinfo.width;
    info.height = sinfo.height;
    info.frameFps = sinfo.frameFps;
    info.sampleRate = sinfo.sampleRate;
    info.channels = sinfo.channels;
    info.sampleBit = sinfo.sampleBit;
    info.clockRate = sinfo.clockRate;
    info.extradata = sinfo.extradata; // 注意，sinfo是局部变量，extradata不能使用string的data()，否则调用该接口获取的extradata可能会数据异常
    info.extrasize = sinfo.extrasize;
    return info;
}