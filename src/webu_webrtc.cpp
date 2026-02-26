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
 * webu_webrtc.cpp - WebRTC Peer Connection Manager
 *
 * Per-camera WebRTC signaling, peer connection lifecycle management,
 * and H.264 RTP frame distribution. Handles SDP offer/answer exchange,
 * ICE candidate trickle, media track setup, and per-peer encoded frame
 * distribution via libdatachannel's H264RtpPacketizer.
 *
 * Threading: libdatachannel fires callbacks from internal threads.
 * peers_mutex protects the peers map. Never hold h264_mutex when
 * calling libdatachannel APIs (deadlock risk).
 *
 * Lock ordering: h264_mutex -> peers_mutex (always).
 * distribute_frame() is called with h264_mutex held, then acquires
 * peers_mutex. No code path acquires them in reverse order.
 *
 * Phase 7c: Signaling over libmicrohttpd
 * Phase 7d: H.264 NAL -> RTP -> Browser
 * Phase 7e: DataChannel for PTZ control commands
 * Phase 7h: Opus audio capture, encoding, and distribution
 */

#include "config.hpp"

#ifdef HAVE_WEBRTC

#include "motion.hpp"
#include "util.hpp"
#include "logger.hpp"
#include "camera.hpp"
#include "conf.hpp"
#include "sound.hpp"
#include "h264_encoder.hpp"
#include "json_parse.hpp"
#include "webu_webrtc.hpp"

#include <cstring>
#include <cmath>

/* Audio thread entry point (C-style for pthread_create) */
static void *webrtc_audio_thread(void *arg)
{
    ((cls_webu_webrtc *)arg)->audio_thread_run();
    return nullptr;
}

cls_webu_webrtc::cls_webu_webrtc(cls_camera *p_cam)
{
    cam = p_cam;
    peer_id_counter = 0;

    pthread_mutex_init(&peers_mutex, NULL);

    /* Initialize audio state */
    audio_ssrc = 0;
    audio_enabled = cam->cfg->webrtc_audio;
    audio_thread_running = false;
    audio_thread_stop = false;
    opus_ctx = nullptr;
    opus_frame = nullptr;
    opus_pkt = nullptr;
    #ifdef HAVE_SWRESAMPLE
    swr_ctx = nullptr;
    #endif
    resample_buf = nullptr;
    resample_buf_sz = 0;
    opus_frame_samples = 960;  /* 20ms at 48kHz */
    audio_source = nullptr;

    /* Configure STUN server */
    if (!cam->cfg->webrtc_stun_server.empty()) {
        rtc_config.iceServers.emplace_back(cam->cfg->webrtc_stun_server);
        MOTION_LOG(INF, TYPE_STREAM, NO_ERRNO
            , _("WebRTC STUN server: %s"), cam->cfg->webrtc_stun_server.c_str());
    }

    /* Configure port range for ICE */
    if (cam->cfg->webrtc_port_min > 0 && cam->cfg->webrtc_port_max > 0) {
        rtc_config.portRangeBegin = (uint16_t)cam->cfg->webrtc_port_min;
        rtc_config.portRangeEnd = (uint16_t)cam->cfg->webrtc_port_max;
        MOTION_LOG(INF, TYPE_STREAM, NO_ERRNO
            , _("WebRTC port range: %d-%d")
            , cam->cfg->webrtc_port_min, cam->cfg->webrtc_port_max);
    }

    MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO
        , _("WebRTC signaling initialized for camera %d (max peers: %d, audio: %s)")
        , cam->cfg->device_id, cam->cfg->webrtc_max_peers
        , audio_enabled ? "on" : "off");
}

cls_webu_webrtc::~cls_webu_webrtc()
{
    /* Stop audio thread first */
    audio_stop();

    /* Close all peer connections */
    pthread_mutex_lock(&peers_mutex);
    for (auto &[id, peer] : peers) {
        try {
            peer.pc->close();
        } catch (...) {
            /* Ignore errors during shutdown */
        }
    }
    peers.clear();
    pthread_mutex_unlock(&peers_mutex);

    pthread_mutex_destroy(&peers_mutex);

    MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO
        , _("WebRTC signaling shutdown for camera %d")
        , cam->cfg->device_id);
}

std::string cls_webu_webrtc::handle_offer(const std::string &sdp_offer)
{
    /* Check peer count limit */
    pthread_mutex_lock(&peers_mutex);
    int cnt = (int)peers.size();
    pthread_mutex_unlock(&peers_mutex);

    if (cnt >= cam->cfg->webrtc_max_peers) {
        MOTION_LOG(WRN, TYPE_STREAM, NO_ERRNO
            , _("WebRTC max peers (%d) reached for camera %d")
            , cam->cfg->webrtc_max_peers, cam->cfg->device_id);
        return "{\"error\":\"max_peers_reached\"}";
    }

    /* Generate unique peer ID */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    peer_id_counter++;
    std::string peer_id = std::to_string(ts.tv_sec) + "-" +
                          std::to_string(peer_id_counter);

    /* Create peer connection */
    std::shared_ptr<rtc::PeerConnection> pc;
    try {
        pc = std::make_shared<rtc::PeerConnection>(rtc_config);
    } catch (const std::exception &e) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            , _("WebRTC PeerConnection creation failed: %s"), e.what());
        return "{\"error\":\"peer_creation_failed\"}";
    }

    /* Set up callbacks before setting remote description */
    setup_peer_callbacks(pc, peer_id);

    /* Add H.264 video track (SendOnly) with RTP packetizer.
     * This must happen BEFORE setRemoteDescription so the track
     * is included in the generated SDP answer. */
    std::shared_ptr<rtc::Track> video_track;
    try {
        uint32_t ssrc = static_cast<uint32_t>(rand());
        std::string cname = "motion-cam-" + std::to_string(cam->cfg->device_id);

        rtc::Description::Video video(cname, rtc::Description::Direction::SendOnly);
        video.addH264Codec(96);
        video.addSSRC(ssrc, cname);

        video_track = pc->addTrack(video);

        auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(
            ssrc, cname, 96, rtc::H264RtpPacketizer::ClockRate);
        auto packetizer = std::make_shared<rtc::H264RtpPacketizer>(
            rtc::NalUnit::Separator::LongStartSequence, rtpConfig);
        video_track->setMediaHandler(packetizer);

        MOTION_LOG(INF, TYPE_STREAM, NO_ERRNO
            , _("WebRTC peer %s: H.264 video track added (SSRC: %u)")
            , peer_id.c_str(), ssrc);
    } catch (const std::exception &e) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            , _("WebRTC peer %s: failed to add video track: %s")
            , peer_id.c_str(), e.what());
        return "{\"error\":\"track_setup_failed\",\"detail\":\"" +
               std::string(e.what()) + "\"}";
    }

    /* Add Opus audio track (Phase 7h) if audio is enabled.
     * Must be created before setRemoteDescription so it appears in the
     * SDP answer. */
    std::shared_ptr<rtc::Track> audio_track;
    if (audio_enabled) {
        try {
            if (audio_ssrc == 0) {
                audio_ssrc = static_cast<uint32_t>(rand());
            }
            std::string audio_cname = "motion-audio-" + std::to_string(cam->cfg->device_id);

            rtc::Description::Audio audio("audio", rtc::Description::Direction::SendOnly);
            audio.addOpusCodec(111);
            audio.addSSRC(audio_ssrc, audio_cname);

            audio_track = pc->addTrack(audio);

            auto audioRtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(
                audio_ssrc, audio_cname, 111, 48000);
            auto audioPacketizer = std::make_shared<rtc::OpusRtpPacketizer>(audioRtpConfig);
            audio_track->setMediaHandler(audioPacketizer);

            MOTION_LOG(INF, TYPE_STREAM, NO_ERRNO
                , _("WebRTC peer %s: Opus audio track added (SSRC: %u)")
                , peer_id.c_str(), audio_ssrc);
        } catch (const std::exception &e) {
            MOTION_LOG(WRN, TYPE_STREAM, NO_ERRNO
                , _("WebRTC peer %s: failed to add audio track: %s (audio disabled)")
                , peer_id.c_str(), e.what());
            audio_track = nullptr;
        }
    }

    /* Create DataChannel for PTZ control commands (Phase 7e).
     * Must be created before setRemoteDescription so it appears in the
     * SDP answer. Uses ordered, reliable delivery for control messages. */
    std::shared_ptr<rtc::DataChannel> dc;
    try {
        dc = pc->createDataChannel("control");

        std::string dc_peer_id = peer_id;
        dc->onOpen([dc_peer_id]() {
            MOTION_LOG(INF, TYPE_STREAM, NO_ERRNO
                , _("WebRTC peer %s: DataChannel 'control' opened")
                , dc_peer_id.c_str());
        });

        dc->onClosed([dc_peer_id]() {
            MOTION_LOG(INF, TYPE_STREAM, NO_ERRNO
                , _("WebRTC peer %s: DataChannel 'control' closed")
                , dc_peer_id.c_str());
        });

        dc->onMessage([this, dc_peer_id](auto data) {
            if (std::holds_alternative<std::string>(data)) {
                handle_ptz_command(std::get<std::string>(data));
            } else {
                MOTION_LOG(DBG, TYPE_STREAM, NO_ERRNO
                    , _("WebRTC peer %s: ignoring binary DataChannel message")
                    , dc_peer_id.c_str());
            }
        });

        MOTION_LOG(INF, TYPE_STREAM, NO_ERRNO
            , _("WebRTC peer %s: DataChannel 'control' created")
            , peer_id.c_str());
    } catch (const std::exception &e) {
        MOTION_LOG(WRN, TYPE_STREAM, NO_ERRNO
            , _("WebRTC peer %s: DataChannel creation failed: %s (PTZ disabled)")
            , peer_id.c_str(), e.what());
        /* Non-fatal: video still works without DataChannel */
        dc = nullptr;
    }

    /* Set the remote description (the SDP offer from the browser) */
    try {
        pc->setRemoteDescription(rtc::Description(sdp_offer, rtc::Description::Type::Offer));
    } catch (const std::exception &e) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            , _("WebRTC setRemoteDescription failed: %s"), e.what());
        return "{\"error\":\"invalid_offer\",\"detail\":\"" +
               std::string(e.what()) + "\"}";
    }

    /* Set local description (generates the SDP answer) */
    try {
        pc->setLocalDescription(rtc::Description::Type::Answer);
    } catch (const std::exception &e) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            , _("WebRTC setLocalDescription failed: %s"), e.what());
        return "{\"error\":\"answer_generation_failed\"}";
    }

    /* Get the local description (the SDP answer) */
    std::string answer_sdp;
    auto local_desc = pc->localDescription();
    if (local_desc.has_value()) {
        answer_sdp = std::string(local_desc.value());
    } else {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            , _("WebRTC localDescription not available"));
        return "{\"error\":\"no_local_description\"}";
    }

    /* Store peer struct (PeerConnection + video/audio tracks + DataChannel) in map */
    webrtc_peer wpeer;
    wpeer.pc = pc;
    wpeer.video_track = video_track;
    wpeer.audio_track = audio_track;
    wpeer.data_channel = dc;

    pthread_mutex_lock(&peers_mutex);
    peers[peer_id] = wpeer;
    last_peer_id = peer_id;
    pthread_mutex_unlock(&peers_mutex);

    /* Notify encoder that a WebRTC peer connected */
    if (cam->h264_enc != nullptr) {
        cam->h264_enc->webrtc_peer_connected();
    }

    /* Start audio thread if this is the first peer and audio is enabled */
    if (audio_enabled && !audio_thread_running) {
        audio_start();
    }

    MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO
        , _("WebRTC peer %s connected to camera %d (total: %d)")
        , peer_id.c_str(), cam->cfg->device_id, peer_count());

    /* Build JSON response with SDP answer and peer ID
     * Escape the SDP string for JSON (replace \ with \\, " with \",
     * newlines with \n, carriage returns with \r) */
    std::string escaped_sdp;
    for (size_t i = 0; i < answer_sdp.length(); i++) {
        char c = answer_sdp[i];
        if (c == '\\') {
            escaped_sdp += "\\\\";
        } else if (c == '"') {
            escaped_sdp += "\\\"";
        } else if (c == '\n') {
            escaped_sdp += "\\n";
        } else if (c == '\r') {
            escaped_sdp += "\\r";
        } else if (c == '\t') {
            escaped_sdp += "\\t";
        } else {
            escaped_sdp += c;
        }
    }

    return "{\"sdp\":\"" + escaped_sdp +
           "\",\"type\":\"answer\",\"peerId\":\"" + peer_id + "\"}";
}

std::string cls_webu_webrtc::handle_candidate(const std::string &candidate,
    const std::string &sdp_mid, int sdp_mline_index)
{
    pthread_mutex_lock(&peers_mutex);
    std::string target_id = last_peer_id;
    auto it = peers.find(target_id);
    if (it == peers.end()) {
        pthread_mutex_unlock(&peers_mutex);
        MOTION_LOG(WRN, TYPE_STREAM, NO_ERRNO
            , _("WebRTC no peer found for ICE candidate"));
        return "{\"error\":\"no_peer_found\"}";
    }
    auto pc = it->second.pc;
    pthread_mutex_unlock(&peers_mutex);

    try {
        pc->addRemoteCandidate(rtc::Candidate(candidate, sdp_mid));
    } catch (const std::exception &e) {
        MOTION_LOG(WRN, TYPE_STREAM, NO_ERRNO
            , _("WebRTC addRemoteCandidate failed: %s"), e.what());
        return "{\"error\":\"candidate_failed\",\"detail\":\"" +
               std::string(e.what()) + "\"}";
    }

    (void)sdp_mline_index;  /* Not used by libdatachannel but kept for API compatibility */

    return "{\"status\":\"ok\"}";
}

std::string cls_webu_webrtc::get_status()
{
    std::string state_str;
    int pcnt = peer_count();

    if (cam->h264_enc != nullptr) {
        switch (cam->h264_enc->state) {
            case H264_STATE_IDLE:
                state_str = "IDLE";
                break;
            case H264_STATE_RECORD_ONLY:
                state_str = "RECORD_ONLY";
                break;
            case H264_STATE_WEBRTC_ONLY:
                state_str = "WEBRTC_ONLY";
                break;
            case H264_STATE_BOTH:
                state_str = "BOTH";
                break;
            default:
                state_str = "UNKNOWN";
                break;
        }
    } else {
        state_str = "NO_ENCODER";
    }

    return "{\"peers\":" + std::to_string(pcnt) +
           ",\"encoder_state\":\"" + state_str +
           "\",\"max_peers\":" + std::to_string(cam->cfg->webrtc_max_peers) + "}";
}

std::string cls_webu_webrtc::disconnect_peer(const std::string &peer_id)
{
    pthread_mutex_lock(&peers_mutex);
    auto it = peers.find(peer_id);
    if (it == peers.end()) {
        pthread_mutex_unlock(&peers_mutex);
        MOTION_LOG(WRN, TYPE_STREAM, NO_ERRNO
            , _("WebRTC peer %s not found for disconnect"), peer_id.c_str());
        return "{\"error\":\"peer_not_found\"}";
    }

    auto pc = it->second.pc;
    peers.erase(it);
    pthread_mutex_unlock(&peers_mutex);

    /* Close the connection (outside mutex to avoid deadlock with callbacks) */
    try {
        pc->close();
    } catch (...) {
        /* Ignore errors during close */
    }

    /* Notify encoder */
    if (cam->h264_enc != nullptr) {
        cam->h264_enc->webrtc_peer_disconnected();
    }

    MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO
        , _("WebRTC peer %s disconnected from camera %d (remaining: %d)")
        , peer_id.c_str(), cam->cfg->device_id, peer_count());

    return "{\"status\":\"ok\"}";
}

/* Distribute an encoded H.264 frame to all connected WebRTC peers.
 *
 * Called from the camera thread with h264_mutex HELD (protecting
 * h264_front). We acquire peers_mutex inside, maintaining the lock
 * order: h264_mutex -> peers_mutex (never reversed).
 *
 * The H.264 NAL data is in Annex B format with 00 00 00 01 start codes.
 * libdatachannel's H264RtpPacketizer handles FU-A fragmentation for
 * NAL units exceeding MTU and performs RTP packetization + SRTP
 * encryption before sending over UDP to the browser. */
void cls_webu_webrtc::distribute_frame(const ctx_h264_data *h264)
{
    if (h264 == nullptr || h264->nal_data == nullptr || h264->nal_sz <= 0) {
        return;
    }

    pthread_mutex_lock(&peers_mutex);
    for (auto &[id, peer] : peers) {
        if (peer.pc &&
            peer.pc->state() == rtc::PeerConnection::State::Connected &&
            peer.video_track && peer.video_track->isOpen()) {
            try {
                peer.video_track->send(
                    reinterpret_cast<const std::byte *>(h264->nal_data),
                    static_cast<size_t>(h264->nal_sz));
            } catch (const std::exception &e) {
                MOTION_LOG(DBG, TYPE_STREAM, NO_ERRNO
                    , _("WebRTC peer %s: send failed: %s")
                    , id.c_str(), e.what());
                /* Continue to other peers -- do not abort distribution
                 * because one peer has a transient issue. */
            }
        }
    }
    pthread_mutex_unlock(&peers_mutex);
}

int cls_webu_webrtc::peer_count()
{
    int cnt;
    pthread_mutex_lock(&peers_mutex);
    cnt = (int)peers.size();
    pthread_mutex_unlock(&peers_mutex);
    return cnt;
}

void cls_webu_webrtc::setup_peer_callbacks(
    std::shared_ptr<rtc::PeerConnection> pc, const std::string &peer_id)
{
    /* Capture peer_id by value (it will outlive this scope via the callback) */
    std::string pid = peer_id;
    cls_webu_webrtc *self = this;

    pc->onStateChange([self, pid](rtc::PeerConnection::State state) {
        const char *state_str;
        switch (state) {
            case rtc::PeerConnection::State::New:
                state_str = "new";
                break;
            case rtc::PeerConnection::State::Connecting:
                state_str = "connecting";
                break;
            case rtc::PeerConnection::State::Connected:
                state_str = "connected";
                break;
            case rtc::PeerConnection::State::Disconnected:
                state_str = "disconnected";
                break;
            case rtc::PeerConnection::State::Failed:
                state_str = "failed";
                break;
            case rtc::PeerConnection::State::Closed:
                state_str = "closed";
                break;
            default:
                state_str = "unknown";
                break;
        }

        MOTION_LOG(INF, TYPE_STREAM, NO_ERRNO
            , _("WebRTC peer %s state: %s"), pid.c_str(), state_str);

        /* Handle disconnection/failure states */
        if (state == rtc::PeerConnection::State::Disconnected ||
            state == rtc::PeerConnection::State::Failed ||
            state == rtc::PeerConnection::State::Closed) {
            self->on_peer_disconnected(pid);
        }
    });

    pc->onGatheringStateChange([pid](rtc::PeerConnection::GatheringState state) {
        const char *state_str;
        switch (state) {
            case rtc::PeerConnection::GatheringState::New:
                state_str = "new";
                break;
            case rtc::PeerConnection::GatheringState::InProgress:
                state_str = "in_progress";
                break;
            case rtc::PeerConnection::GatheringState::Complete:
                state_str = "complete";
                break;
            default:
                state_str = "unknown";
                break;
        }

        MOTION_LOG(DBG, TYPE_STREAM, NO_ERRNO
            , _("WebRTC peer %s ICE gathering: %s"), pid.c_str(), state_str);
    });
}

void cls_webu_webrtc::on_peer_disconnected(const std::string &peer_id)
{
    /* Called from libdatachannel callback thread. Remove peer from map. */
    pthread_mutex_lock(&peers_mutex);
    auto it = peers.find(peer_id);
    if (it == peers.end()) {
        /* Already removed (e.g., by explicit disconnect_peer call) */
        pthread_mutex_unlock(&peers_mutex);
        return;
    }
    peers.erase(it);
    int remaining = (int)peers.size();
    pthread_mutex_unlock(&peers_mutex);

    /* Notify encoder (outside mutex) */
    if (cam->h264_enc != nullptr) {
        cam->h264_enc->webrtc_peer_disconnected();
    }

    /* Stop audio thread if no peers remain */
    if (remaining == 0 && audio_thread_running) {
        audio_stop();
    }

    MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO
        , _("WebRTC peer %s auto-disconnected from camera %d (remaining: %d)")
        , peer_id.c_str(), cam->cfg->device_id, remaining);
}

/* Handle a PTZ command received via DataChannel (Phase 7e).
 *
 * Called from libdatachannel callback thread. Parses the JSON message
 * and dispatches to Motion's existing PTZ infrastructure, which executes
 * shell commands configured via ptz_pan_left, ptz_pan_right, etc.
 *
 * Expected JSON format:
 *   {"type":"ptz","action":"pan_left"}
 *   {"type":"ptz","action":"pan_right"}
 *   {"type":"ptz","action":"tilt_up"}
 *   {"type":"ptz","action":"tilt_down"}
 *   {"type":"ptz","action":"zoom_in"}
 *   {"type":"ptz","action":"zoom_out"}
 */
void cls_webu_webrtc::handle_ptz_command(const std::string &msg)
{
    JsonParser parser;

    if (!parser.parse(msg)) {
        MOTION_LOG(DBG, TYPE_STREAM, NO_ERRNO
            , _("WebRTC DataChannel: invalid JSON: %s"), parser.getError().c_str());
        return;
    }

    std::string msg_type = parser.getString("type");
    if (msg_type != "ptz") {
        MOTION_LOG(DBG, TYPE_STREAM, NO_ERRNO
            , _("WebRTC DataChannel: ignoring non-ptz message type '%s'")
            , msg_type.c_str());
        return;
    }

    std::string action = parser.getString("action");
    if (action.empty()) {
        MOTION_LOG(DBG, TYPE_STREAM, NO_ERRNO
            , _("WebRTC DataChannel: ptz message missing 'action' field"));
        return;
    }

    /* Dispatch to Motion's PTZ infrastructure.
     * This mirrors the dispatch in cls_webu_post::ptz() (webu_post.cpp),
     * executing the configured shell command for each PTZ direction. */
    bool dispatched = false;

    if (action == "pan_left" && !cam->cfg->ptz_pan_left.empty()) {
        cam->frame_skip = cam->cfg->ptz_wait;
        util_exec_command(cam, cam->cfg->ptz_pan_left.c_str(), NULL);
        dispatched = true;

    } else if (action == "pan_right" && !cam->cfg->ptz_pan_right.empty()) {
        cam->frame_skip = cam->cfg->ptz_wait;
        util_exec_command(cam, cam->cfg->ptz_pan_right.c_str(), NULL);
        dispatched = true;

    } else if (action == "tilt_up" && !cam->cfg->ptz_tilt_up.empty()) {
        cam->frame_skip = cam->cfg->ptz_wait;
        util_exec_command(cam, cam->cfg->ptz_tilt_up.c_str(), NULL);
        dispatched = true;

    } else if (action == "tilt_down" && !cam->cfg->ptz_tilt_down.empty()) {
        cam->frame_skip = cam->cfg->ptz_wait;
        util_exec_command(cam, cam->cfg->ptz_tilt_down.c_str(), NULL);
        dispatched = true;

    } else if (action == "zoom_in" && !cam->cfg->ptz_zoom_in.empty()) {
        cam->frame_skip = cam->cfg->ptz_wait;
        util_exec_command(cam, cam->cfg->ptz_zoom_in.c_str(), NULL);
        dispatched = true;

    } else if (action == "zoom_out" && !cam->cfg->ptz_zoom_out.empty()) {
        cam->frame_skip = cam->cfg->ptz_wait;
        util_exec_command(cam, cam->cfg->ptz_zoom_out.c_str(), NULL);
        dispatched = true;
    }

    if (dispatched) {
        MOTION_LOG(INF, TYPE_STREAM, NO_ERRNO
            , _("WebRTC PTZ command: %s (camera %d)")
            , action.c_str(), cam->cfg->device_id);
    } else {
        MOTION_LOG(DBG, TYPE_STREAM, NO_ERRNO
            , _("WebRTC PTZ command '%s' not configured for camera %d")
            , action.c_str(), cam->cfg->device_id);
    }
}

/* Send a PTZ status response to a specific peer's DataChannel (Phase 7e).
 *
 * Thread-safe: acquires peers_mutex to find the peer. The DataChannel
 * send is done outside the mutex since libdatachannel handles its own
 * thread safety internally. */
void cls_webu_webrtc::send_ptz_status(const std::string &peer_id,
                                       const std::string &action, bool success)
{
    std::shared_ptr<rtc::DataChannel> dc;

    pthread_mutex_lock(&peers_mutex);
    auto it = peers.find(peer_id);
    if (it != peers.end() && it->second.data_channel) {
        dc = it->second.data_channel;
    }
    pthread_mutex_unlock(&peers_mutex);

    if (!dc || !dc->isOpen()) {
        return;
    }

    std::string status_msg = "{\"type\":\"ptz_status\",\"action\":\"" +
                             action + "\",\"success\":" +
                             (success ? "true" : "false") +
                             ",\"camera\":" +
                             std::to_string(cam->cfg->device_id) + "}";

    try {
        dc->send(status_msg);
    } catch (const std::exception &e) {
        MOTION_LOG(DBG, TYPE_STREAM, NO_ERRNO
            , _("WebRTC peer %s: failed to send PTZ status: %s")
            , peer_id.c_str(), e.what());
    }
}

/* ================================================================
 * Phase 7h: Audio support - Opus encoding and distribution
 * ================================================================ */

/* Distribute an Opus-encoded audio frame to all connected WebRTC peers.
 * Called from the audio thread. Acquires peers_mutex internally. */
void cls_webu_webrtc::distribute_audio(const uint8_t *opus_data, int opus_sz)
{
    if (opus_data == nullptr || opus_sz <= 0) {
        return;
    }

    pthread_mutex_lock(&peers_mutex);
    for (auto &[id, peer] : peers) {
        if (peer.pc &&
            peer.pc->state() == rtc::PeerConnection::State::Connected &&
            peer.audio_track && peer.audio_track->isOpen()) {
            try {
                peer.audio_track->send(
                    reinterpret_cast<const std::byte *>(opus_data),
                    static_cast<size_t>(opus_sz));
            } catch (const std::exception &e) {
                MOTION_LOG(DBG, TYPE_STREAM, NO_ERRNO
                    , _("WebRTC peer %s: audio send failed: %s")
                    , id.c_str(), e.what());
            }
        }
    }
    pthread_mutex_unlock(&peers_mutex);
}

/* Initialize FFmpeg Opus encoder and resampler.
 * Returns 0 on success, -1 on failure. */
int cls_webu_webrtc::audio_init_encoder()
{
    const AVCodec *codec;
    int retcd;

    /* Find the libopus encoder */
    codec = avcodec_find_encoder_by_name("libopus");
    if (codec == nullptr) {
        codec = avcodec_find_encoder(AV_CODEC_ID_OPUS);
    }
    if (codec == nullptr) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            , _("WebRTC audio: Opus encoder not found"));
        return -1;
    }

    opus_ctx = avcodec_alloc_context3(codec);
    if (opus_ctx == nullptr) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            , _("WebRTC audio: failed to allocate Opus codec context"));
        return -1;
    }

    /* Configure Opus encoder for 48kHz mono */
    opus_ctx->sample_rate = 48000;
    opus_ctx->bit_rate = cam->cfg->webrtc_audio_opus_bitrate;
    #ifdef HAVE_FFMPEG7
    opus_ctx->ch_layout = AV_CHANNEL_LAYOUT_MONO;
    #else
    opus_ctx->channels = 1;
    opus_ctx->channel_layout = AV_CH_LAYOUT_MONO;
    #endif
    opus_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
    opus_ctx->time_base = {1, 48000};

    /* Set Opus-specific options */
    av_opt_set(opus_ctx->priv_data, "application", "audio", 0);
    av_opt_set_int(opus_ctx->priv_data, "frame_duration", 20, 0);

    retcd = avcodec_open2(opus_ctx, codec, nullptr);
    if (retcd < 0) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            , _("WebRTC audio: failed to open Opus encoder (error %d)"), retcd);
        avcodec_free_context(&opus_ctx);
        opus_ctx = nullptr;
        return -1;
    }

    /* Opus frame size: 960 samples for 20ms at 48kHz */
    opus_frame_samples = opus_ctx->frame_size;
    if (opus_frame_samples <= 0) {
        opus_frame_samples = 960;
    }

    /* Allocate encoder input frame */
    opus_frame = av_frame_alloc();
    if (opus_frame == nullptr) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            , _("WebRTC audio: failed to allocate opus frame"));
        audio_close_encoder();
        return -1;
    }
    opus_frame->format = AV_SAMPLE_FMT_S16;
    opus_frame->nb_samples = opus_frame_samples;
    opus_frame->sample_rate = 48000;
    #ifdef HAVE_FFMPEG7
    opus_frame->ch_layout = AV_CHANNEL_LAYOUT_MONO;
    #else
    opus_frame->channels = 1;
    opus_frame->channel_layout = AV_CH_LAYOUT_MONO;
    #endif

    retcd = av_frame_get_buffer(opus_frame, 0);
    if (retcd < 0) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            , _("WebRTC audio: failed to allocate opus frame buffer"));
        audio_close_encoder();
        return -1;
    }

    /* Allocate encoder output packet */
    opus_pkt = av_packet_alloc();
    if (opus_pkt == nullptr) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            , _("WebRTC audio: failed to allocate opus packet"));
        audio_close_encoder();
        return -1;
    }

    /* Initialize resampler if source rate != 48kHz */
    #ifdef HAVE_SWRESAMPLE
    if (audio_source != nullptr &&
        audio_source->audio_ring.sample_rate != 48000) {
        int src_rate = audio_source->audio_ring.sample_rate;

        #ifdef HAVE_FFMPEG7
        AVChannelLayout out_ch = AV_CHANNEL_LAYOUT_MONO;
        AVChannelLayout in_ch = AV_CHANNEL_LAYOUT_MONO;
        retcd = swr_alloc_set_opts2(&swr_ctx,
            &out_ch, AV_SAMPLE_FMT_S16, 48000,
            &in_ch, AV_SAMPLE_FMT_S16, src_rate,
            0, nullptr);
        if (retcd < 0 || swr_ctx == nullptr) {
        #else
        swr_ctx = swr_alloc_set_opts(nullptr,
            AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, 48000,
            AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, src_rate,
            0, nullptr);
        if (swr_ctx == nullptr) {
        #endif
            MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
                , _("WebRTC audio: failed to create resampler (%d -> 48000)")
                , src_rate);
            audio_close_encoder();
            return -1;
        }
        retcd = swr_init(swr_ctx);
        if (retcd < 0) {
            MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
                , _("WebRTC audio: failed to init resampler (error %d)"), retcd);
            audio_close_encoder();
            return -1;
        }

        /* Calculate resampled buffer size (source samples needed for one 20ms output) */
        resample_buf_sz = opus_frame_samples;
        resample_buf = (int16_t *)malloc((size_t)resample_buf_sz * sizeof(int16_t));
        if (resample_buf == nullptr) {
            audio_close_encoder();
            return -1;
        }

        MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO
            , _("WebRTC audio: resampler %dHz -> 48000Hz"), src_rate);
    }
    #endif

    MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO
        , _("WebRTC audio: Opus encoder initialized (bitrate: %d, frame: %d samples)")
        , cam->cfg->webrtc_audio_opus_bitrate, opus_frame_samples);

    return 0;
}

/* Close and free the Opus encoder, resampler, and associated buffers. */
void cls_webu_webrtc::audio_close_encoder()
{
    if (opus_pkt != nullptr) {
        av_packet_free(&opus_pkt);
        opus_pkt = nullptr;
    }
    if (opus_frame != nullptr) {
        av_frame_free(&opus_frame);
        opus_frame = nullptr;
    }
    if (opus_ctx != nullptr) {
        avcodec_free_context(&opus_ctx);
        opus_ctx = nullptr;
    }
    #ifdef HAVE_SWRESAMPLE
    if (swr_ctx != nullptr) {
        swr_free(&swr_ctx);
        swr_ctx = nullptr;
    }
    #endif
    if (resample_buf != nullptr) {
        free(resample_buf);
        resample_buf = nullptr;
    }
    resample_buf_sz = 0;
}

/* Read exactly 'samples' from the audio ring buffer into 'dest'.
 * Returns the number of samples actually read (may be < samples
 * if the ring buffer has insufficient data). */
int cls_webu_webrtc::audio_read_ring(int16_t *dest, int samples)
{
    if (audio_source == nullptr) {
        return 0;
    }

    ctx_audio_ring *ring = &audio_source->audio_ring;
    int count = 0;

    pthread_mutex_lock(&ring->mutex);
    if (!ring->active || ring->buffer == nullptr) {
        pthread_mutex_unlock(&ring->mutex);
        return 0;
    }

    /* Calculate available samples */
    int avail = ring->write_pos - ring->read_pos;
    if (avail < 0) {
        avail += ring->capacity;
    }

    if (avail < samples) {
        /* Not enough data yet -- return 0 and let caller sleep */
        pthread_mutex_unlock(&ring->mutex);
        return 0;
    }

    /* Read samples from ring buffer */
    for (count = 0; count < samples; count++) {
        dest[count] = ring->buffer[ring->read_pos];
        ring->read_pos = (ring->read_pos + 1) % ring->capacity;
    }

    pthread_mutex_unlock(&ring->mutex);
    return count;
}

/* Audio encoding thread.
 *
 * Reads PCM from the sound device's ring buffer, resamples to 48kHz
 * if needed, encodes to Opus via FFmpeg, and distributes to peers.
 * Runs at 20ms cadence (one Opus frame per iteration). */
void cls_webu_webrtc::audio_thread_run()
{
    int retcd;
    int64_t pts_counter = 0;

    mythreadname_set("wa", cam->cfg->device_id, cam->cfg->device_name.c_str());

    MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO
        , _("WebRTC audio thread started for camera %d"), cam->cfg->device_id);

    /* audio_source must be set before the thread starts (by audio_start).
     * If not set, the thread exits gracefully. */
    if (audio_source == nullptr) {
        MOTION_LOG(WRN, TYPE_STREAM, NO_ERRNO
            , _("WebRTC audio: no sound device found, audio thread exiting"));
        audio_thread_running = false;
        return;
    }

    /* Wait for the ring buffer to become active */
    int wait_count = 0;
    while (!audio_thread_stop && !audio_source->audio_ring.active) {
        SLEEP(0, 100000000L);  /* 100ms */
        wait_count++;
        if (wait_count > 50) {  /* 5 seconds */
            MOTION_LOG(WRN, TYPE_STREAM, NO_ERRNO
                , _("WebRTC audio: ring buffer not active after 5s, giving up"));
            audio_thread_running = false;
            return;
        }
    }

    /* Initialize encoder */
    retcd = audio_init_encoder();
    if (retcd != 0) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            , _("WebRTC audio: encoder init failed, audio thread exiting"));
        audio_thread_running = false;
        return;
    }

    /* Calculate how many source samples we need per 20ms frame.
     * If resampling, we need source_rate/48000 * opus_frame_samples */
    int src_rate = audio_source->audio_ring.sample_rate;
    int src_samples_needed;
    if (src_rate != 48000) {
        /* Number of source samples to produce opus_frame_samples output samples */
        src_samples_needed = (int)((int64_t)opus_frame_samples * src_rate / 48000);
        if (src_samples_needed < 1) {
            src_samples_needed = opus_frame_samples;
        }
    } else {
        src_samples_needed = opus_frame_samples;
    }

    /* Allocate a temporary buffer for reading source samples */
    int16_t *src_buf = (int16_t *)malloc((size_t)src_samples_needed * sizeof(int16_t));
    if (src_buf == nullptr) {
        audio_close_encoder();
        audio_thread_running = false;
        return;
    }

    /* Main audio loop: read -> resample -> encode -> distribute */
    while (!audio_thread_stop) {
        /* Read PCM from ring buffer */
        int got = audio_read_ring(src_buf, src_samples_needed);
        if (got < src_samples_needed) {
            /* Not enough data yet, sleep ~5ms and retry */
            SLEEP(0, 5000000L);
            continue;
        }

        /* Resample or copy to encoder input frame */
        retcd = av_frame_make_writable(opus_frame);
        if (retcd < 0) {
            MOTION_LOG(DBG, TYPE_STREAM, NO_ERRNO
                , _("WebRTC audio: frame not writable"));
            continue;
        }

        #ifdef HAVE_SWRESAMPLE
        if (swr_ctx != nullptr) {
            /* Resample from src_rate to 48kHz */
            const uint8_t *in_data[1] = { (const uint8_t *)src_buf };
            uint8_t *out_data[1] = { opus_frame->data[0] };
            int converted = swr_convert(swr_ctx,
                out_data, opus_frame_samples,
                in_data, src_samples_needed);
            if (converted < 0) {
                MOTION_LOG(DBG, TYPE_STREAM, NO_ERRNO
                    , _("WebRTC audio: resample error"));
                continue;
            }
            opus_frame->nb_samples = converted;
        } else
        #endif
        {
            /* No resampling needed -- copy directly */
            memcpy(opus_frame->data[0], src_buf,
                   (size_t)opus_frame_samples * sizeof(int16_t));
            opus_frame->nb_samples = opus_frame_samples;
        }

        opus_frame->pts = pts_counter;
        pts_counter += opus_frame->nb_samples;

        /* Send frame to Opus encoder */
        retcd = avcodec_send_frame(opus_ctx, opus_frame);
        if (retcd < 0) {
            MOTION_LOG(DBG, TYPE_STREAM, NO_ERRNO
                , _("WebRTC audio: encoder send_frame error %d"), retcd);
            continue;
        }

        /* Receive encoded Opus packets */
        while (retcd >= 0) {
            retcd = avcodec_receive_packet(opus_ctx, opus_pkt);
            if (retcd == AVERROR(EAGAIN) || retcd == AVERROR_EOF) {
                break;
            }
            if (retcd < 0) {
                MOTION_LOG(DBG, TYPE_STREAM, NO_ERRNO
                    , _("WebRTC audio: encoder receive_packet error %d"), retcd);
                break;
            }

            /* Distribute Opus packet to all peers */
            distribute_audio(opus_pkt->data, opus_pkt->size);

            av_packet_unref(opus_pkt);
        }
    }

    free(src_buf);
    audio_close_encoder();

    MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO
        , _("WebRTC audio thread stopped for camera %d"), cam->cfg->device_id);

    audio_thread_running = false;
}

/* Start the audio encoding thread.
 * Finds the appropriate sound device and launches the thread. */
void cls_webu_webrtc::audio_start()
{
    if (audio_thread_running || !audio_enabled) {
        return;
    }

    /* Find the sound device to use as audio source.
     * We access the motapp's snd_list via the extern declared in motion.hpp.
     * The camera object has an `app` reference through its parent. */
    audio_source = nullptr;

    /* cam inherits from cls_motapp indirectly -- we need the app pointer.
     * In this codebase, cls_camera stores a pointer to cls_motapp as a member.
     * Let's check if we can access it. The snd_list is on cls_motapp. */

    /* Access the global app via the camera's motapp pointer.
     * cls_camera has `cls_motapp *app` as a public member from motion.hpp. */
    cls_motapp *motapp = cam->app;
    if (motapp == nullptr) {
        MOTION_LOG(WRN, TYPE_STREAM, NO_ERRNO
            , _("WebRTC audio: cannot access motapp, audio disabled"));
        return;
    }

    /* Search snd_list for a matching device */
    std::string wanted_dev = cam->cfg->webrtc_audio_device;
    for (size_t i = 0; i < motapp->snd_list.size(); i++) {
        cls_sound *snd = motapp->snd_list[i];
        if (snd == nullptr || snd->cfg == nullptr) {
            continue;
        }
        /* If a specific device was requested, match it */
        if (!wanted_dev.empty()) {
            if (snd->cfg->snd_device == wanted_dev) {
                audio_source = snd;
                break;
            }
        } else {
            /* Use the first available sound device */
            audio_source = snd;
            break;
        }
    }

    if (audio_source == nullptr) {
        MOTION_LOG(WRN, TYPE_STREAM, NO_ERRNO
            , _("WebRTC audio: no sound device found (device: '%s'), audio disabled")
            , wanted_dev.c_str());
        return;
    }

    MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO
        , _("WebRTC audio: using sound device '%s' (rate: %d)")
        , audio_source->cfg->snd_device.c_str()
        , audio_source->audio_ring.sample_rate);

    /* Launch audio thread */
    audio_thread_stop = false;
    audio_thread_running = true;

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    int retcd = pthread_create(&audio_thread, &thread_attr, &webrtc_audio_thread, this);
    if (retcd != 0) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            , _("WebRTC audio: failed to create audio thread (error %d)"), retcd);
        audio_thread_running = false;
    }
    pthread_attr_destroy(&thread_attr);
}

/* Stop the audio encoding thread. */
void cls_webu_webrtc::audio_stop()
{
    if (!audio_thread_running) {
        return;
    }

    audio_thread_stop = true;

    /* Wait for audio thread to exit (max 3 seconds) */
    int wait_count = 0;
    while (audio_thread_running && wait_count < 30) {
        SLEEP(0, 100000000L);  /* 100ms */
        wait_count++;
    }

    if (audio_thread_running) {
        MOTION_LOG(WRN, TYPE_STREAM, NO_ERRNO
            , _("WebRTC audio thread did not stop in 3s for camera %d")
            , cam->cfg->device_id);
    }

    audio_source = nullptr;
}

#endif /* HAVE_WEBRTC */
