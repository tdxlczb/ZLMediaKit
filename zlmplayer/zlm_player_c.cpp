#include "zlm_player_c.h"
#include "zlm_player.h"

void *API_CALL ZP_CreateZlmplayer() {
    zlmplayer::ZlmPlayer *impl = new zlmplayer::ZlmPlayer();
    return impl;
}

void API_CALL ZP_DeleteZlmplayer(void *pPlayer) {
    delete pPlayer;
    pPlayer = nullptr;
}

void API_CALL ZP_SetOnPacket(void *pPlayer, OnPacket callback, void *user) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->SetOnPacket([callback, user](const zlmplayer::Packet &pkt) {
        ZP_Packet cpkt;
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

void API_CALL ZP_SetOnPlayStatus(void *pPlayer, OnPlayStatus callback, void *user) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->SetOnPlayStatus([callback, user](zlmplayer::PlayStatus status) { callback((ZP_PlayStatus)status, user); });
}

bool API_CALL ZP_Play(void *pPlayer, const char *url, ZP_PlayOptions options) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return false;
    zlmplayer::PlayOptions opts;
    opts.isTcp = options.isTcp;
    return impl->Play(url, opts);
}

void API_CALL ZP_Stop(void *pPlayer) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->Stop();
}

void API_CALL ZP_Pause(void *pPlayer) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->Pause();
}

void API_CALL ZP_Resume(void *pPlayer) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->Resume();
}

void API_CALL ZP_Seek(void *pPlayer, int seconds) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->Seek(seconds);
}

void API_CALL ZP_Speed(void *pPlayer, double speed) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->Speed(speed);
}

ZP_StreamInfo API_CALL ZP_GetVideoStream(void *pPlayer) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    ZP_StreamInfo info;
    if (!impl)
        return info;
    zlmplayer::StreamInfo sinfo = impl->GetVideoStream();
    info.mediaType = sinfo.mediaType;
    info.codecId = sinfo.codecId;
    info.width = sinfo.width;
    info.height = sinfo.height;
    info.frameFps = sinfo.frameFps;
    info.sampleRate = sinfo.sampleRate;
    info.channels = sinfo.channels;
    info.sampleBit = sinfo.sampleBit;
    info.clockRate = sinfo.clockRate;
    info.extradata = sinfo.extradata; //注意，sinfo是局部变量，extradata不能使用string的data()，否则调用该接口获取的extradata可能会数据异常
    info.extrasize = sinfo.extrasize;
    return info;
}

ZP_StreamInfo API_CALL ZP_GetAudioStream(void *pPlayer) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    ZP_StreamInfo info;
    if (!impl)
        return info;
    zlmplayer::StreamInfo sinfo = impl->GetAudioStream();
    info.mediaType = sinfo.mediaType;
    info.codecId = sinfo.codecId;
    info.width = sinfo.width;
    info.height = sinfo.height;
    info.frameFps = sinfo.frameFps;
    info.sampleRate = sinfo.sampleRate;
    info.channels = sinfo.channels;
    info.sampleBit = sinfo.sampleBit;
    info.clockRate = sinfo.clockRate;
    info.extradata = sinfo.extradata;
    info.extrasize = sinfo.extrasize;
    return info;
}