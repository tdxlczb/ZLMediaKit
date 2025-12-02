#pragma once
#include "Rtsp/RtspDemuxer.h"
#include "Rtsp/RtspPlayer.h"
#include <string>

namespace zlmplayer {

class RtspPlayerImpl
    : public mediakit::PlayerImp<mediakit::RtspPlayer, mediakit::PlayerBase>
    , private mediakit::TrackListener {
public:
    using Ptr = std::shared_ptr<RtspPlayerImpl>;
    using Super = PlayerImp<RtspPlayer, PlayerBase>;

    RtspPlayerImpl(const toolkit::EventPoller::Ptr &poller)
        : Super(poller) {}

    ~RtspPlayerImpl() override {}

    float getProgress() const override;

    uint32_t getProgressPos() const override;

    void seekTo(float fProgress) override;

    void seekTo(uint32_t seekPos) override;

    float getDuration() const override;

    std::vector<mediakit::Track::Ptr> getTracks(bool ready = true) const override;

    //转ffmpeg的AVCodecID
    int getAVCodecId(mediakit::CodecId id);

    int getVideoClockRate();

private:
    // 派生类回调函数  [AUTO-TRANSLATED:61e20903]
    // Derived class callback function
    bool onCheckSDP(const std::string &sdp) override;

    void onRecvRTP(mediakit::RtpPacket::Ptr rtp, const mediakit::SdpTrack::Ptr &track) override;

    void onPlayResult(const toolkit::SockException &ex) override;

    bool addTrack(const mediakit::Track::Ptr &track) override { return true; }

    void addTrackCompleted() override;

private:
    mediakit::RtspDemuxer::Ptr _demuxer;
};

} // namespace zlmplayer
