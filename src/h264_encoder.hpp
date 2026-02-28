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
 * h264_encoder.hpp - Shared H.264 Encoder for WebRTC + Movie Recording
 *
 * Per-camera shared H.264 encoder that serves both movie recording and
 * WebRTC viewers from a single encoder instance. Double-buffered output
 * allows lock-free encoding while consumers read the previous frame.
 *
 * State machine: IDLE / RECORD_ONLY / WEBRTC_ONLY / BOTH
 */

#ifndef _INCLUDE_H264_ENCODER_HPP_
#define _INCLUDE_H264_ENCODER_HPP_

/* config.hpp must be included before the HAVE_WEBRTC guard so that
 * the autoconf-generated define is visible when this header is the
 * first include in a translation unit (e.g., h264_encoder.cpp). */
#include "config.hpp"

#ifdef HAVE_WEBRTC

#include "motion.hpp"
#include <atomic>

class cls_camera;

/* Per-camera H.264 encoded frame buffer (consumed by movie + WebRTC).
 * Double-buffered: camera thread writes to back buffer, then swaps under mutex.
 * Consumers read from front buffer. */
struct ctx_h264_data {
    u_char      *nal_data;      /* Encoded NAL unit data (owned, malloc'd) */
    int          nal_sz;        /* Size of nal_data in bytes */
    bool         is_keyframe;   /* AV_PKT_FLAG_KEY was set */
    int64_t      pts;           /* Presentation timestamp (timebase units) */
    int64_t      dts;           /* Decode timestamp */
};

/* Cached SPS/PPS NAL units from the encoder */
struct ctx_h264_sps_pps {
    u_char      *data;          /* SPS+PPS NAL data (Annex B format) */
    int          sz;            /* Size in bytes */
};

enum h264_encoder_state {
    H264_STATE_IDLE,
    H264_STATE_RECORD_ONLY,
    H264_STATE_WEBRTC_ONLY,
    H264_STATE_BOTH
};

enum h264_codec_type {
    H264_CODEC_LIBX264,     /* Software fallback */
    H264_CODEC_V4L2M2M,     /* Pi 4 hardware (V4L2 M2M) */
    H264_CODEC_NVENC,        /* NVIDIA hardware */
    H264_CODEC_VAAPI,        /* Intel/AMD VAAPI */
    H264_CODEC_QSV           /* Intel Quick Sync */
};

class cls_h264_encoder {
public:
    cls_h264_encoder(cls_camera *p_cam);
    ~cls_h264_encoder();

    int encode_frame(u_char *yuv_data, int width, int height,
                     const struct timespec *ts);
    int start(h264_encoder_state new_state);
    void stop();
    void transition(h264_encoder_state new_state);

    void webrtc_peer_connected();
    void webrtc_peer_disconnected();
    void recording_started();
    void recording_stopped();

    void request_keyframe();

    ctx_h264_data   h264_front;
    ctx_h264_data   h264_back;
    pthread_mutex_t h264_mutex;

    ctx_h264_sps_pps sps_pps;

    h264_encoder_state  state;
    std::atomic<int>    webrtc_cnct;
    bool                recording;

    void swap_buffers();

    /* Codec context accessor for movie passthrough mode */
    AVCodecContext *get_ctx_codec() { return ctx_codec; }

private:
    cls_camera      *cam;
    AVCodecContext  *ctx_codec;
    const AVCodec   *codec;
    AVFrame         *picture;
    AVPacket        *pkt;
    AVDictionary    *opts;

    int              gop_size;
    int64_t          frame_cnt;
    int64_t          base_pts;
    struct timespec  start_time;
    h264_codec_type  codec_type;

    int  open_encoder(int profile, int gop);
    void close_encoder();
    int  select_codec();
    void set_quality_params(int profile);
    void put_yuv420(u_char *yuv_data, int width, int height);
    void cache_sps_pps();
    void encode_nal();

    /* NAL fixup data for v4l2m2m (same logic as cls_movie::encode_nal) */
    char            *nal_info;
    int              nal_info_len;

    /* VAAPI hardware context (only used when codec_type == H264_CODEC_VAAPI) */
    AVBufferRef     *hw_device_ctx;
    AVBufferRef     *hw_frames_ref;
    AVFrame         *hw_frame;

    /* QSV NV12 conversion (only used when codec_type == H264_CODEC_QSV) */
    struct SwsContext *sws_ctx;
    AVFrame         *nv12_frame;

    bool         keyframe_requested;
    int          hysteresis_countdown;
};

#endif /* HAVE_WEBRTC */

#endif /* _INCLUDE_H264_ENCODER_HPP_ */
