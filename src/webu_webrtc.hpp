/*
 *    This file is part of Motion.
 *
 *    Motion is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Motion is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Motion.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

/*
 * webu_webrtc.hpp - WebRTC Peer Connection Manager Interface
 *
 * Per-camera WebRTC peer connection manager. Handles SDP offer/answer
 * exchange, ICE candidate trickle, peer lifecycle, and H.264 RTP
 * frame distribution to connected browsers.
 * Uses libdatachannel for WebRTC peer connections.
 *
 * Phase 7c: Signaling over libmicrohttpd
 * Phase 7d: H.264 NAL -> RTP -> Browser (media track + distribute_frame)
 * Phase 7e: DataChannel for PTZ control commands
 */

#ifndef _INCLUDE_WEBU_WEBRTC_HPP_
#define _INCLUDE_WEBU_WEBRTC_HPP_

/* config.hpp must be included before HAVE_WEBRTC guard.
 * It defines the macro via autoconf. */
#include "config.hpp"

#ifdef HAVE_WEBRTC

#include <rtc/rtc.hpp>
#include <map>
#include <string>
#include <memory>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
    #ifdef HAVE_SWRESAMPLE
        #include <libswresample/swresample.h>
    #endif
}

class cls_camera;
class cls_sound;
struct ctx_h264_data;
struct ctx_audio_ring;

/* Per-peer state: PeerConnection + video/audio tracks + control DataChannel. */
struct webrtc_peer {
    std::shared_ptr<rtc::PeerConnection> pc;
    std::shared_ptr<rtc::Track>          video_track;
    std::shared_ptr<rtc::Track>          audio_track;   /* Phase 7h: Opus audio */
    std::shared_ptr<rtc::DataChannel>    data_channel;  /* Phase 7e: PTZ control */
};

class cls_webu_webrtc {
public:
    cls_webu_webrtc(cls_camera *p_cam);
    ~cls_webu_webrtc();

    /* Called from HTTP request handlers (webu_json API methods) */
    std::string handle_offer(const std::string &sdp_offer);
    std::string handle_candidate(const std::string &candidate,
                          const std::string &sdp_mid, int sdp_mline_index);
    std::string get_status();
    std::string disconnect_peer(const std::string &peer_id);

    /* Called from camera thread to distribute encoded frames (Phase 7d) */
    void distribute_frame(const ctx_h264_data *h264);

    /* Called from audio thread to distribute Opus frames (Phase 7h) */
    void distribute_audio(const uint8_t *opus_data, int opus_sz);

    int peer_count();

    /* Send PTZ status update to a specific peer (Phase 7e) */
    void send_ptz_status(const std::string &peer_id,
                         const std::string &action, bool success);

    /* Audio encoder thread (Phase 7h) */
    void audio_thread_run();
    void audio_start();
    void audio_stop();

private:
    /* DataChannel PTZ command handler (Phase 7e) */
    void handle_ptz_command(const std::string &msg);
    cls_camera *cam;
    std::map<std::string, webrtc_peer> peers;
    pthread_mutex_t peers_mutex;
    std::string     last_peer_id;   /* Most recently created peer ID */

    rtc::Configuration rtc_config;
    void setup_peer_callbacks(std::shared_ptr<rtc::PeerConnection> pc,
                              const std::string &peer_id);
    void on_peer_disconnected(const std::string &peer_id);

    int peer_id_counter;

    /* Audio encoder state (Phase 7h) */
    uint32_t        audio_ssrc;
    bool            audio_enabled;         /* Derived from config at construction */
    pthread_t       audio_thread;
    bool            audio_thread_running;
    bool            audio_thread_stop;

    AVCodecContext  *opus_ctx;             /* FFmpeg Opus encoder context */
    AVFrame         *opus_frame;           /* Input frame for Opus encoder */
    AVPacket        *opus_pkt;             /* Output packet from Opus encoder */

    #ifdef HAVE_SWRESAMPLE
    SwrContext      *swr_ctx;              /* Resampler: source rate -> 48kHz */
    #endif

    int16_t         *resample_buf;         /* Resampled 48kHz output buffer */
    int              resample_buf_sz;      /* Size in samples */

    int              opus_frame_samples;   /* Samples per Opus frame (960 for 20ms@48kHz) */

    cls_sound       *audio_source;         /* Sound device providing PCM ring buffer */

    int  audio_init_encoder();
    void audio_close_encoder();
    int  audio_read_ring(int16_t *dest, int samples);
};

#endif /* HAVE_WEBRTC */

#endif /* _INCLUDE_WEBU_WEBRTC_HPP_ */
