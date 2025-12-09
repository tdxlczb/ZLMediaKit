#include "zlm_player_c.h"
#include "zlm_player.h"

void *__stdcall ZP_CreateZlmplayer() {
    zlmplayer::ZlmPlayer *impl = new zlmplayer::ZlmPlayer();
    return impl;
}

void __stdcall ZP_DeleteZlmplayer(void *pPlayer) {
    delete pPlayer;
    pPlayer = nullptr;
}

void __stdcall ZP_SetOnPacket(void *pPlayer, OnPacket callback, void *user) {
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

void __stdcall ZP_SetOnPlayStatus(void *pPlayer, OnPlayStatus callback, void *user) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->SetOnPlayStatus([callback, user](zlmplayer::PlayStatus status) { callback((ZP_PlayStatus)status, user); });
}

bool __stdcall ZP_Play(void *pPlayer, const char *url, ZP_PlayOptions options) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return false;
    zlmplayer::PlayOptions opts;
    opts.isTcp = options.isTcp;
    return impl->Play(url, opts);
}

void __stdcall ZP_Stop(void *pPlayer) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->Stop();
}

void __stdcall ZP_Pause(void *pPlayer) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->Pause();
}

void __stdcall ZP_Resume(void *pPlayer) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->Resume();
}

void __stdcall ZP_Seek(void *pPlayer, int seconds) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->Seek(seconds);
}

void __stdcall ZP_Speed(void *pPlayer, double speed) {
    zlmplayer::ZlmPlayer *impl = (zlmplayer::ZlmPlayer *)pPlayer;
    if (!impl)
        return;
    impl->Speed(speed);
}

ZP_StreamInfo __stdcall ZP_GetVideoStream(void *pPlayer) {
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
    return info;
}

ZP_StreamInfo __stdcall ZP_GetAudioStream(void *pPlayer) {
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
    return info;
}