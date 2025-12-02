#include "rtsp_player_impl.h"

namespace zlmplayer {

///////////////////////////////////////////////////
// RtspPlayerImpl

float RtspPlayerImpl::getProgress() const {
    if (getDuration() > 0) {
        return getProgressMilliSecond() / (getDuration() * 1000);
    }
    return PlayerBase::getProgress();
}

uint32_t RtspPlayerImpl::getProgressPos() const {
    if (getDuration() > 0) {
        return getProgressMilliSecond();
    }
    return PlayerBase::getProgressPos();
}

void RtspPlayerImpl::seekTo(float fProgress) {
    fProgress = MAX(float(0), MIN(fProgress, float(1.0)));
    seekToMilliSecond((uint32_t)(fProgress * getDuration() * 1000));
}

void RtspPlayerImpl::seekTo(uint32_t seekPos) {
    seekToMilliSecond(seekPos * 1000);
}

float RtspPlayerImpl::getDuration() const {
    return _demuxer ? _demuxer->getDuration() : 0;
}

void RtspPlayerImpl::onPlayResult(const toolkit::SockException &ex) {
    if (!(*this)[mediakit::Client::kWaitTrackReady].as<bool>() || ex) {
        Super::onPlayResult(ex);
        return;
    }
}

void RtspPlayerImpl::addTrackCompleted() {
    if ((*this)[mediakit::Client::kWaitTrackReady].as<bool>()) {
        Super::onPlayResult(toolkit::SockException(toolkit::Err_success, "play success"));
    }
}

std::vector<mediakit::Track::Ptr> RtspPlayerImpl::getTracks(bool ready /*= true*/) const {
    return _demuxer ? _demuxer->getTracks(ready) : Super::getTracks(ready);
}

int RtspPlayerImpl::getAVCodecId(mediakit::CodecId id) {
    //暂时就这几种，后续补充
    switch (id) {
        case mediakit::CodecInvalid: return 0; // AV_CODEC_ID_NONE
        case mediakit::CodecH264: return 27; // AV_CODEC_ID_H264
        case mediakit::CodecH265: return 173; // AV_CODEC_ID_HEVC
        case mediakit::CodecAAC: return 86018; // AV_CODEC_ID_AAC
        case mediakit::CodecG711A: return 65543; // AV_CODEC_ID_PCM_ALAW
        case mediakit::CodecG711U: return 65542; // AV_CODEC_ID_PCM_MULAW
        case mediakit::CodecOpus: return 86076; // AV_CODEC_ID_OPUS
        default: break;
    }
    return 0;
}

int RtspPlayerImpl::getVideoClockRate() {
    mediakit::SdpTrack::Ptr sdpTrack = getSdpTrackByTrackType(mediakit::TrackVideo); 
    if (sdpTrack) {
        return sdpTrack->_samplerate;
    }
    //默认返回90000
    return 90000;
}

bool RtspPlayerImpl::onCheckSDP(const std::string &sdp) {
    RtspPlayer::setDisableNtpStamp();
    _demuxer = std::make_shared<mediakit::RtspDemuxer>();
    _demuxer->setTrackListener(this, (*this)[mediakit::Client::kWaitTrackReady].as<bool>());
    _demuxer->loadSdp(sdp);
    return true;
}

void RtspPlayerImpl::onRecvRTP(mediakit::RtpPacket::Ptr rtp, const mediakit::SdpTrack::Ptr &track) {
    // rtp解复用时可以判断是否为关键帧起始位置  [AUTO-TRANSLATED:fb7d9b6e]
    // When demultiplexing RTP, it can be determined whether it is the starting position of the key frame
    auto key_pos = _demuxer->inputRtp(rtp);
}

} // namespace zlmplayer