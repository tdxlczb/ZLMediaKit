#pragma once
#include "Rtsp/RtspDemuxer.h"
#include "Rtsp/RtspMediaSource.h"
#include "Rtsp/RtspPlayer.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <thread>
#include <signal.h>

namespace test {

class RtspPlayerImp
    : public mediakit::PlayerImp<mediakit::RtspPlayer, mediakit::PlayerBase>
    , private mediakit::TrackListener {
public:
    using Ptr = std::shared_ptr<RtspPlayerImp>;
    using Super = PlayerImp<RtspPlayer, PlayerBase>;

    RtspPlayerImp(const toolkit::EventPoller::Ptr &poller)
        : Super(poller) {}

    ~RtspPlayerImp() override { DebugL; }

    float getProgress() const override {
        if (getDuration() > 0) {
            return getProgressMilliSecond() / (getDuration() * 1000);
        }
        return PlayerBase::getProgress();
    }

    uint32_t getProgressPos() const override {
        if (getDuration() > 0) {
            return getProgressMilliSecond();
        }
        return PlayerBase::getProgressPos();
    }

    void seekTo(float fProgress) override {
        fProgress = MAX(float(0), MIN(fProgress, float(1.0)));
        seekToMilliSecond((uint32_t)(fProgress * getDuration() * 1000));
    }

    void seekTo(uint32_t seekPos) override {
        seekToMilliSecond(seekPos * 1000);
    }

    float getDuration() const override;

    std::vector<mediakit::Track::Ptr> getTracks(bool ready = true) const override;

    size_t getRecvSpeed() override {
        size_t ret = TcpClient::getRecvSpeed();
        for (auto &rtp : _rtp_sock) {
            if (rtp) {
                ret += rtp->getRecvSpeed();
            }
        }
        for (auto &rtcp : _rtcp_sock) {
            if (rtcp) {
                ret += rtcp->getRecvSpeed();
            }
        }
        return ret;
    }

    size_t getRecvTotalBytes() override {
        size_t ret = TcpClient::getRecvTotalBytes();
        for (auto &rtp : _rtp_sock) {
            if (rtp) {
                ret += rtp->getRecvTotalBytes();
            }
        }
        for (auto &rtcp : _rtcp_sock) {
            if (rtcp) {
                ret += rtcp->getRecvTotalBytes();
            }
        }
        return ret;
    }

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
    mediakit::RtspMediaSource::Ptr _rtsp_media_src;
};

///////////////////////////////////////////////////
// RtspPlayerImp
float RtspPlayerImp::getDuration() const {
    return _demuxer ? _demuxer->getDuration() : 0;
}

void RtspPlayerImp::onPlayResult(const toolkit::SockException &ex) {
    if (!(*this)[mediakit::Client::kWaitTrackReady].as<bool>() || ex) {
        Super::onPlayResult(ex);
        return;
    }
}

void RtspPlayerImp::addTrackCompleted() {
    if ((*this)[mediakit::Client::kWaitTrackReady].as<bool>()) {
        Super::onPlayResult(toolkit::SockException(toolkit::Err_success, "play success"));
    }
}

std::vector<mediakit::Track::Ptr> RtspPlayerImp::getTracks(bool ready /*= true*/) const {
    return _demuxer ? _demuxer->getTracks(ready) : Super::getTracks(ready);
}

bool RtspPlayerImp::onCheckSDP(const std::string &sdp) {
    RtspPlayer::setDisableNtpStamp();
    _rtsp_media_src = std::dynamic_pointer_cast<mediakit::RtspMediaSource>(_media_src);
    if (_rtsp_media_src) {
        _rtsp_media_src->setSdp(sdp);
    }
    _demuxer = std::make_shared<mediakit::RtspDemuxer>();
    _demuxer->setTrackListener(this, (*this)[mediakit::Client::kWaitTrackReady].as<bool>());
    _demuxer->loadSdp(sdp);
    return true;
}

void RtspPlayerImp::onRecvRTP(mediakit::RtpPacket::Ptr rtp, const mediakit::SdpTrack::Ptr &track) {
    // rtp解复用时可以判断是否为关键帧起始位置  [AUTO-TRANSLATED:fb7d9b6e]
    // When demultiplexing RTP, it can be determined whether it is the starting position of the key frame
    auto key_pos = _demuxer->inputRtp(rtp);
    if (_rtsp_media_src) {
        _rtsp_media_src->onWrite(std::move(rtp), key_pos);
    }

}

} // namespace test

int main(int argc, char *argv[]) {

    std::string url = "rtsp://admin:admin@123@172.16.25.11:554/c2/b1764374400/e1764378000/replay/s0/";
    //std::string url = "rtsp://172.16.19.69/live/test";
    // 设置日志  [AUTO-TRANSLATED:50ba02ba]
    // Set log
    toolkit::Logger::Instance().add(std::make_shared<toolkit::ConsoleChannel>());
    toolkit::Logger::Instance().setWriter(std::make_shared<toolkit::AsyncLogWriter>());

    std::shared_ptr<test::RtspPlayerImp> player = nullptr;

    std::thread th1([&player,url](){

    auto poller = toolkit::EventPollerPool::Instance().getPoller();
    player = std::move(std::make_shared<test::RtspPlayerImp>(poller));
    player->setOnPlayResult([player](const toolkit::SockException &ex) {
        InfoL << "OnPlayResult:" << ex.what();
        if (ex) {
            return;
        }
        {
            auto videoTrack = std::dynamic_pointer_cast<mediakit::VideoTrack>(player->getTrack(mediakit::TrackVideo));
            if (!videoTrack) {
                WarnL << "No video Track!";
                return;
            }
            auto id = videoTrack->getCodecId();
            auto br = videoTrack->getBitRate();
            auto name = videoTrack->getCodecName();
            auto dur = videoTrack->getDuration();
            auto index = videoTrack->getIndex();
            auto num = videoTrack->getFrames();
            auto fps = videoTrack->getVideoFps();
            auto h = videoTrack->getVideoHeight();
            auto w = videoTrack->getVideoWidth();
            videoTrack->addDelegate([](const mediakit::Frame::Ptr &frame) {
                // please decode video here
                if (frame) {
                    static int frameIndex = 0;
                    frameIndex++;
                    auto pts = frame->pts();
                    auto size = frame->size();
                    auto isKey = frame->keyFrame();
                    InfoL << "======= get video Track frame index：" << frameIndex << +", pts:" << pts << +", size:" << size
                          << ", keyFrame:" << frame->keyFrame();
                }
                return true;
            });
        }
        {
            auto audioTrack = std::dynamic_pointer_cast<mediakit::AudioTrack>(player->getTrack(mediakit::TrackAudio));
            if (!audioTrack) {
                WarnL << "No audio Track!";
                return;
            }
            auto id = audioTrack->getCodecId();
            auto br = audioTrack->getBitRate();
            auto name = audioTrack->getCodecName();
            auto dur = audioTrack->getDuration();
            auto index = audioTrack->getIndex();
            auto num = audioTrack->getFrames();
            auto samplerate = audioTrack->getAudioSampleRate();
            auto channels = audioTrack->getAudioChannel();

            audioTrack->addDelegate([](const mediakit::Frame::Ptr &frame) {
                // please decode audio here
                if (frame) {
                    static int frameIndex = 0;
                    frameIndex++;
                    InfoL << "get audio Track frame index：" << frameIndex << +", pts:" << frame->pts() << ", keyFrame:" << frame->keyFrame();
                }
                return true;
            });
        }
    });

    player->setOnShutdown([](const toolkit::SockException &ex) { ErrorL << "OnShutdown:" << ex.what(); });

    // RTP transport over TCP
    (*player)[mediakit::Client::kRtpType] = mediakit::Rtsp::RTP_TCP;
    player->play(url);

    });
    th1.join();

    std::thread th([player]() {
        int i = 0;
        while (true) {
            i++;
            Sleep(100);
            if (i == 10) {
                player->seekTo((uint32_t)120);
                //player->speed(16.0);
            }

            if (i == 20) {
                player->pause(true);
            }
            if (i == 25) {
                player->pause(false);
            }
            //if (i == 30) {
            //    player->pause(true);
            //}
            //if (i == 35) {
            //    player->pause(false);
            //}
        }
        });
    th.join();
    
    static toolkit::semaphore sem;
    signal(SIGINT, [](int) { sem.post(); }); // 设置退出信号
    sem.wait();
    return 0;
}

#include "../zlmplayer/zlm_player.h"
#pragma comment(lib, "../../release/windows/Debug/Release/zlmplayer.lib")

int main2(int argc, char *argv[]) {

    std::string url = "rtsp://admin:admin@123@172.16.25.11:554/c2/b1764374400/e1764378000/replay/s0/";
    // 设置日志  [AUTO-TRANSLATED:50ba02ba]
    // Set log
    toolkit::Logger::Instance().add(std::make_shared<toolkit::ConsoleChannel>());
    toolkit::Logger::Instance().setWriter(std::make_shared<toolkit::AsyncLogWriter>());

    std::shared_ptr<zlmplayer::ZlmPlayer> player = nullptr;

    std::thread th1([&player, url]() {
        player = std::move(std::make_shared<zlmplayer::ZlmPlayer>());
        player->SetOnPlayStatus([](zlmplayer::PlayStatus status) {
            });
        player->SetOnPacket([](const zlmplayer::Packet&packet) {
            if (packet.isAudio) {
                InfoL << "get audio Track frame:" << (int64_t)packet.pts;
            } else {
                InfoL << "get video Track frame:" << (int64_t)packet.pts;
            }
            });
        
        zlmplayer::PlayOptions options;
        options["rtsp_transport"] = "tcp";
        player->Play(url, options);
    });
    th1.join();

    std::thread th([player]() {
        int i = 0;
        while (true) {
            i++;
            Sleep(100);
            if (i == 10) {
                //player->Seek((uint32_t)120);
                player->Speed(8.0);
            }

            //if (i == 20) {
            //    player->Pause();
            //}
            //if (i == 25) {
            //    player->Resume();
            //}
            //if (i == 30) {
            //    player->Pause();
            //}
            //if (i == 35) {
            //    player->Resume();
            //}
        }
    });
    th.join();

    static toolkit::semaphore sem;
    signal(SIGINT, [](int) { sem.post(); }); // 设置退出信号
    sem.wait();
    return 0;
}
