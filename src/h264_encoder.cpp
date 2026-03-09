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
 * h264_encoder.cpp - Shared H.264 Encoder Implementation
 *
 * Camera-level shared H.264 encoder serving both movie recording and
 * WebRTC viewers from a single encoder instance per camera.
 *
 * Supports multiple hardware encoders:
 *   - h264_v4l2m2m (Pi 4 hardware)
 *   - h264_nvenc   (NVIDIA GPU)
 *   - h264_vaapi   (Intel/AMD VAAPI)
 *   - h264_qsv     (Intel Quick Sync)
 *   - libx264      (software fallback)
 *
 * State machine transitions are driven by recording_started/stopped
 * and webrtc_peer_connected/disconnected calls from the camera thread.
 */

#include "h264_encoder.hpp"

#ifdef HAVE_WEBRTC

#include "motion.hpp"
#include "util.hpp"
#include "camera.hpp"
#include "conf.hpp"
#include "logger.hpp"
#include "movie.hpp"

cls_h264_encoder::cls_h264_encoder(cls_camera *p_cam)
{
    cam = p_cam;

    ctx_codec = nullptr;
    codec = nullptr;
    picture = nullptr;
    pkt = nullptr;
    opts = nullptr;

    gop_size = 30;
    frame_cnt = 0;
    base_pts = 0;
    start_time.tv_sec = 0;
    start_time.tv_nsec = 0;
    codec_type = H264_CODEC_LIBX264;

    nal_info = nullptr;
    nal_info_len = 0;

    hw_device_ctx = nullptr;
    hw_frames_ref = nullptr;
    hw_frame = nullptr;

    sws_ctx = nullptr;
    nv12_frame = nullptr;

    keyframe_requested = false;
    hysteresis_countdown = 0;

    h264_front.nal_data = nullptr;
    h264_front.nal_sz = 0;
    h264_front.is_keyframe = false;
    h264_front.pts = 0;
    h264_front.dts = 0;

    h264_back.nal_data = nullptr;
    h264_back.nal_sz = 0;
    h264_back.is_keyframe = false;
    h264_back.pts = 0;
    h264_back.dts = 0;

    sps_pps.data = nullptr;
    sps_pps.sz = 0;

    state = H264_STATE_IDLE;
    webrtc_cnct = 0;
    recording = false;

    pthread_mutex_init(&h264_mutex, nullptr);
}

cls_h264_encoder::~cls_h264_encoder()
{
    stop();

    pthread_mutex_lock(&h264_mutex);
    if (h264_front.nal_data) {
        free(h264_front.nal_data);
        h264_front.nal_data = nullptr;
    }
    if (h264_back.nal_data) {
        free(h264_back.nal_data);
        h264_back.nal_data = nullptr;
    }
    if (sps_pps.data) {
        free(sps_pps.data);
        sps_pps.data = nullptr;
    }
    pthread_mutex_unlock(&h264_mutex);

    pthread_mutex_destroy(&h264_mutex);
}

/* Select the best available H.264 codec.
 * Priority: v4l2m2m -> nvenc -> vaapi -> qsv -> libx264
 * Each encoder is only tried if it was successfully probed at startup. */
int cls_h264_encoder::select_codec()
{
    codec_type = H264_CODEC_LIBX264;

    if (cam->app->hw_encoders.probed) {
        /* Pi 4 hardware encoder */
        if (cam->app->hw_encoders.h264_v4l2m2m) {
            codec = avcodec_find_encoder_by_name("h264_v4l2m2m");
            if (codec != nullptr) {
                codec_type = H264_CODEC_V4L2M2M;
                MOTION_LOG(NTC, TYPE_ENCODER, NO_ERRNO
                    , _("H264 shared encoder: using h264_v4l2m2m (hardware)"));
                return 0;
            }
        }

        /* NVIDIA GPU encoder */
        if (cam->app->hw_encoders.h264_nvenc) {
            codec = avcodec_find_encoder_by_name("h264_nvenc");
            if (codec != nullptr) {
                codec_type = H264_CODEC_NVENC;
                MOTION_LOG(NTC, TYPE_ENCODER, NO_ERRNO
                    , _("H264 shared encoder: using h264_nvenc (NVIDIA hardware)"));
                return 0;
            }
        }

        /* Intel/AMD VAAPI encoder */
        if (cam->app->hw_encoders.h264_vaapi) {
            codec = avcodec_find_encoder_by_name("h264_vaapi");
            if (codec != nullptr) {
                codec_type = H264_CODEC_VAAPI;
                MOTION_LOG(NTC, TYPE_ENCODER, NO_ERRNO
                    , _("H264 shared encoder: using h264_vaapi (VAAPI hardware)"));
                return 0;
            }
        }

        /* Intel Quick Sync encoder */
        if (cam->app->hw_encoders.h264_qsv) {
            codec = avcodec_find_encoder_by_name("h264_qsv");
            if (codec != nullptr) {
                codec_type = H264_CODEC_QSV;
                MOTION_LOG(NTC, TYPE_ENCODER, NO_ERRNO
                    , _("H264 shared encoder: using h264_qsv (Intel QSV hardware)"));
                return 0;
            }
        }
    }

    /* Fallback to libx264 */
    codec = avcodec_find_encoder_by_name("libx264");
    if (codec != nullptr) {
        MOTION_LOG(NTC, TYPE_ENCODER, NO_ERRNO
            , _("H264 shared encoder: using libx264 (software)"));
        return 0;
    }

    /* Last resort: default H.264 encoder */
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (codec != nullptr) {
        MOTION_LOG(NTC, TYPE_ENCODER, NO_ERRNO
            , _("H264 shared encoder: using default H.264 encoder: %s"), codec->name);
        return 0;
    }

    MOTION_LOG(ERR, TYPE_ENCODER, NO_ERRNO
        , _("H264 shared encoder: no H.264 encoder found"));
    return -1;
}

/* Set quality parameters based on profile and codec type */
void cls_h264_encoder::set_quality_params(int profile)
{
    int quality;

    opts = nullptr;

    quality = cam->cfg->webrtc_quality;
    if (quality <= 0) {
        quality = 50;
    }
    if (quality > 100) {
        quality = 100;
    }

    switch (codec_type) {
    case H264_CODEC_V4L2M2M: {
        /* v4l2m2m: bitrate mode */
        int fps = cam->cfg->framerate;
        if (fps < 2) fps = 2;

        int bitrate = (int)(((int64_t)cam->imgs.width * cam->imgs.height * fps * quality) >> 7);
        if (bitrate < 4000) {
            bitrate = 4000;
        }
        ctx_codec->bit_rate = bitrate;
        ctx_codec->profile = profile;
        av_dict_set(&opts, "preset", "superfast", 0);
        av_dict_set(&opts, "tune", "zerolatency", 0);
        av_dict_set(&opts, "num_output_buffers", "32", 0);
        av_dict_set(&opts, "num_capture_buffers", "16", 0);
        break;
    }

    case H264_CODEC_NVENC: {
        /* NVENC: Constant Quality mode (CQ) — closest to CRF */
        int cq = (int)(((100 - quality) * 51) / 100);
        if (cq < 1) cq = 1;
        char cq_str[10];
        snprintf(cq_str, sizeof(cq_str), "%d", cq);

        av_opt_set(ctx_codec->priv_data, "rc", "vbr", 0);
        av_opt_set(ctx_codec->priv_data, "cq", cq_str, 0);
        av_opt_set(ctx_codec->priv_data, "preset", "p4", 0);
        av_opt_set(ctx_codec->priv_data, "tune", "ll", 0);
        /* NVENC has no constrained_baseline; use baseline for WebRTC */
        if (profile == MY_PROFILE_H264_CONSTRAINED_BASELINE) {
            av_opt_set(ctx_codec->priv_data, "profile", "baseline", 0);
        } else {
            av_opt_set(ctx_codec->priv_data, "profile", "high", 0);
        }
        break;
    }

    case H264_CODEC_VAAPI: {
        /* VAAPI: CQP mode — uses QP value */
        int qp = (int)(((100 - quality) * 51) / 100);
        if (qp < 1) qp = 1;
        ctx_codec->global_quality = qp;
        /* VAAPI: must use MY_PROFILE_H264_CONSTRAINED_BASELINE (578),
         * NOT FF_PROFILE_H264_BASELINE (66) which fails on VAAPI */
        if (profile == MY_PROFILE_H264_CONSTRAINED_BASELINE) {
            ctx_codec->profile = MY_PROFILE_H264_CONSTRAINED_BASELINE;
        } else {
            ctx_codec->profile = MY_PROFILE_H264_HIGH;
        }
        break;
    }

    case H264_CODEC_QSV: {
        /* QSV: ICQ mode — Intelligent Constant Quality */
        int icq = (int)(((100 - quality) * 51) / 100);
        if (icq < 1) icq = 1;
        ctx_codec->global_quality = icq;

        av_opt_set(ctx_codec->priv_data, "preset", "medium", 0);
        if (profile == MY_PROFILE_H264_CONSTRAINED_BASELINE) {
            av_opt_set(ctx_codec->priv_data, "profile", "baseline", 0);
        } else {
            av_opt_set(ctx_codec->priv_data, "profile", "high", 0);
        }
        break;
    }

    case H264_CODEC_LIBX264:
    default: {
        /* libx264: CRF mode */
        int crf = (int)(((100 - quality) * 51) / 100);
        if (crf < 1) {
            crf = 1;
        }
        char crf_str[10];
        snprintf(crf_str, sizeof(crf_str), "%d", crf);

        if (profile == MY_PROFILE_H264_CONSTRAINED_BASELINE) {
            av_opt_set(ctx_codec->priv_data, "profile", "baseline", 0);
        } else {
            av_opt_set(ctx_codec->priv_data, "profile", "high", 0);
        }
        av_opt_set(ctx_codec->priv_data, "crf", crf_str, 0);
        av_opt_set(ctx_codec->priv_data, "tune", "zerolatency", 0);
        av_opt_set(ctx_codec->priv_data, "preset", "superfast", 0);
        break;
    }
    }
}

/* Open the encoder with specified profile and GOP size */
int cls_h264_encoder::open_encoder(int profile, int gop)
{
    int retcd;
    char errstr[128];

    if (select_codec() != 0) {
        return -1;
    }

    ctx_codec = avcodec_alloc_context3(codec);
    if (ctx_codec == nullptr) {
        MOTION_LOG(ERR, TYPE_ENCODER, NO_ERRNO
            , _("H264 shared encoder: failed to allocate codec context"));
        return -1;
    }

    /* Use configured framerate, not volatile lastrate. lastrate can be
     * temporarily low during startup or CPU load. A low time_base
     * denominator causes PTS truncation and collision-bumping that
     * stretches video duration (e.g., fps=16 at 26fps actual → 1.6x). */
    int fps = cam->cfg->framerate;
    if (fps < 2) fps = 2;

    ctx_codec->codec_id = codec->id;
    ctx_codec->codec_type = AVMEDIA_TYPE_VIDEO;
    ctx_codec->width = cam->imgs.width;
    ctx_codec->height = cam->imgs.height;
    ctx_codec->time_base.num = 1;
    ctx_codec->time_base.den = fps;
    ctx_codec->max_b_frames = 0;
    ctx_codec->gop_size = gop;
    ctx_codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    gop_size = gop;

    /* Set pixel format based on encoder type */
    if (codec_type == H264_CODEC_VAAPI) {
        ctx_codec->pix_fmt = AV_PIX_FMT_VAAPI;
    } else if (codec_type == H264_CODEC_QSV) {
        ctx_codec->pix_fmt = AV_PIX_FMT_NV12;
    } else {
        ctx_codec->pix_fmt = AV_PIX_FMT_YUV420P;
    }

    /* VAAPI: set up hardware device and frames context */
    if (codec_type == H264_CODEC_VAAPI) {
        retcd = av_hwdevice_ctx_create(&hw_device_ctx,
            AV_HWDEVICE_TYPE_VAAPI, NULL, NULL, 0);
        if (retcd < 0) {
            av_strerror(retcd, errstr, sizeof(errstr));
            MOTION_LOG(WRN, TYPE_ENCODER, NO_ERRNO
                , _("H264 shared encoder: VAAPI device context creation failed: %s"), errstr);
            avcodec_free_context(&ctx_codec);
            ctx_codec = nullptr;
            goto fallback_libx264;
        }

        hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx);
        if (hw_frames_ref == nullptr) {
            MOTION_LOG(WRN, TYPE_ENCODER, NO_ERRNO
                , _("H264 shared encoder: VAAPI frames context allocation failed"));
            av_buffer_unref(&hw_device_ctx);
            avcodec_free_context(&ctx_codec);
            ctx_codec = nullptr;
            goto fallback_libx264;
        }

        AVHWFramesContext *frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
        frames_ctx->format    = AV_PIX_FMT_VAAPI;
        frames_ctx->sw_format = AV_PIX_FMT_NV12;
        frames_ctx->width     = cam->imgs.width;
        frames_ctx->height    = cam->imgs.height;
        frames_ctx->initial_pool_size = 20;

        retcd = av_hwframe_ctx_init(hw_frames_ref);
        if (retcd < 0) {
            av_strerror(retcd, errstr, sizeof(errstr));
            MOTION_LOG(WRN, TYPE_ENCODER, NO_ERRNO
                , _("H264 shared encoder: VAAPI frames context init failed: %s"), errstr);
            av_buffer_unref(&hw_frames_ref);
            av_buffer_unref(&hw_device_ctx);
            avcodec_free_context(&ctx_codec);
            ctx_codec = nullptr;
            goto fallback_libx264;
        }

        ctx_codec->hw_frames_ctx = av_buffer_ref(hw_frames_ref);

        /* Allocate reusable HW frame for uploads */
        hw_frame = av_frame_alloc();
        if (hw_frame == nullptr) {
            MOTION_LOG(WRN, TYPE_ENCODER, NO_ERRNO
                , _("H264 shared encoder: failed to allocate VAAPI hw_frame"));
            av_buffer_unref(&hw_frames_ref);
            av_buffer_unref(&hw_device_ctx);
            avcodec_free_context(&ctx_codec);
            ctx_codec = nullptr;
            goto fallback_libx264;
        }
    }

    set_quality_params(profile);

    retcd = avcodec_open2(ctx_codec, codec, &opts);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        MOTION_LOG(WRN, TYPE_ENCODER, NO_ERRNO
            , _("H264 shared encoder: could not open codec %s: %s"), codec->name, errstr);

        /* If hardware encoder failed, try libx264 fallback */
        if (codec_type != H264_CODEC_LIBX264) {
            /* Clean up any HW resources from failed encoder */
            if (hw_frame) { av_frame_free(&hw_frame); hw_frame = nullptr; }
            if (hw_frames_ref) { av_buffer_unref(&hw_frames_ref); hw_frames_ref = nullptr; }
            if (hw_device_ctx) { av_buffer_unref(&hw_device_ctx); hw_device_ctx = nullptr; }
            avcodec_free_context(&ctx_codec);
            ctx_codec = nullptr;
            av_dict_free(&opts);
            opts = nullptr;

            goto fallback_libx264;
        } else {
            avcodec_free_context(&ctx_codec);
            ctx_codec = nullptr;
            av_dict_free(&opts);
            opts = nullptr;
            return -1;
        }
    }

    goto encoder_opened;

fallback_libx264:
    MOTION_LOG(NTC, TYPE_ENCODER, NO_ERRNO
        , _("H264 shared encoder: falling back to libx264"));
    av_dict_free(&opts);
    opts = nullptr;

    codec_type = H264_CODEC_LIBX264;
    codec = avcodec_find_encoder_by_name("libx264");
    if (codec == nullptr) {
        MOTION_LOG(ERR, TYPE_ENCODER, NO_ERRNO
            , _("H264 shared encoder: libx264 not found for fallback"));
        return -1;
    }

    ctx_codec = avcodec_alloc_context3(codec);
    if (ctx_codec == nullptr) {
        MOTION_LOG(ERR, TYPE_ENCODER, NO_ERRNO
            , _("H264 shared encoder: failed to allocate fallback codec context"));
        return -1;
    }

    ctx_codec->codec_id = codec->id;
    ctx_codec->codec_type = AVMEDIA_TYPE_VIDEO;
    ctx_codec->width = cam->imgs.width;
    ctx_codec->height = cam->imgs.height;
    ctx_codec->time_base.num = 1;
    ctx_codec->time_base.den = fps;
    ctx_codec->pix_fmt = AV_PIX_FMT_YUV420P;
    ctx_codec->max_b_frames = 0;
    ctx_codec->gop_size = gop;
    ctx_codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    set_quality_params(profile);

    retcd = avcodec_open2(ctx_codec, codec, &opts);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        MOTION_LOG(ERR, TYPE_ENCODER, NO_ERRNO
            , _("H264 shared encoder: fallback also failed: %s"), errstr);
        avcodec_free_context(&ctx_codec);
        ctx_codec = nullptr;
        av_dict_free(&opts);
        opts = nullptr;
        return -1;
    }

encoder_opened:
    av_dict_free(&opts);
    opts = nullptr;

    /* Allocate the picture frame */
    picture = av_frame_alloc();
    if (picture == nullptr) {
        MOTION_LOG(ERR, TYPE_ENCODER, NO_ERRNO
            , _("H264 shared encoder: could not allocate frame"));
        close_encoder();
        return -1;
    }

    picture->format = ctx_codec->pix_fmt;
    picture->width = ctx_codec->width;
    picture->height = ctx_codec->height;

    /* QSV: allocate NV12 frame for format conversion */
    if (codec_type == H264_CODEC_QSV) {
        nv12_frame = av_frame_alloc();
        if (nv12_frame == nullptr) {
            MOTION_LOG(ERR, TYPE_ENCODER, NO_ERRNO
                , _("H264 shared encoder: could not allocate NV12 frame"));
            close_encoder();
            return -1;
        }
        nv12_frame->format = AV_PIX_FMT_NV12;
        nv12_frame->width = cam->imgs.width;
        nv12_frame->height = cam->imgs.height;
        retcd = av_frame_get_buffer(nv12_frame, 0);
        if (retcd < 0) {
            MOTION_LOG(ERR, TYPE_ENCODER, NO_ERRNO
                , _("H264 shared encoder: could not allocate NV12 frame buffer"));
            close_encoder();
            return -1;
        }
    }

    frame_cnt = 0;

    /* Reset NAL info for v4l2m2m fixup */
    if (nal_info) {
        free(nal_info);
        nal_info = nullptr;
        nal_info_len = 0;
    }

    MOTION_LOG(NTC, TYPE_ENCODER, NO_ERRNO
        , _("H264 shared encoder started: codec=%s profile=%d gop=%d %dx%d")
        , codec->name, profile, gop
        , ctx_codec->width, ctx_codec->height);

    return 0;
}

/* Close the encoder and free all resources */
void cls_h264_encoder::close_encoder()
{
    if (picture != nullptr) {
        av_frame_free(&picture);
        picture = nullptr;
    }

    if (ctx_codec != nullptr) {
        avcodec_free_context(&ctx_codec);
        ctx_codec = nullptr;
    }

    if (pkt != nullptr) {
        av_packet_free(&pkt);
        pkt = nullptr;
    }

    if (opts != nullptr) {
        av_dict_free(&opts);
        opts = nullptr;
    }

    if (nal_info) {
        free(nal_info);
        nal_info = nullptr;
        nal_info_len = 0;
    }

    /* VAAPI cleanup */
    if (hw_frame != nullptr) {
        av_frame_free(&hw_frame);
        hw_frame = nullptr;
    }
    if (hw_frames_ref != nullptr) {
        av_buffer_unref(&hw_frames_ref);
        hw_frames_ref = nullptr;
    }
    if (hw_device_ctx != nullptr) {
        av_buffer_unref(&hw_device_ctx);
        hw_device_ctx = nullptr;
    }

    /* QSV cleanup */
    if (nv12_frame != nullptr) {
        av_frame_free(&nv12_frame);
        nv12_frame = nullptr;
    }
    if (sws_ctx != nullptr) {
        sws_freeContext(sws_ctx);
        sws_ctx = nullptr;
    }

    /* Clear encoded frame buffers to prevent stale data from poisoning
     * last_pts in movie recordings after movie_max_time split.
     * Without this, put_encoded_packet() reads a stale high-PTS packet
     * from h264_front, sets last_pts high, and all subsequent frames
     * from the new encoder (PTS starting at 0) are silently dropped. */
    pthread_mutex_lock(&h264_mutex);
    if (h264_front.nal_data) {
        free(h264_front.nal_data);
        h264_front.nal_data = nullptr;
    }
    h264_front.nal_sz = 0;
    h264_front.is_keyframe = false;
    h264_front.pts = 0;
    h264_front.dts = 0;

    if (h264_back.nal_data) {
        free(h264_back.nal_data);
        h264_back.nal_data = nullptr;
    }
    h264_back.nal_sz = 0;
    h264_back.is_keyframe = false;
    h264_back.pts = 0;
    h264_back.dts = 0;
    pthread_mutex_unlock(&h264_mutex);

    codec = nullptr;
    frame_cnt = 0;
}

/* NAL fixup for v4l2m2m encoder.
 * Duplicated from cls_movie::encode_nal() — the v4l2m2m encoder sends
 * SPS/PPS in a separate non-keyframe packet at pts==0. This saves that
 * data and prepends it to subsequent packets for proper NAL structure. */
void cls_h264_encoder::encode_nal()
{
    if ((pkt->pts == 0) && (!(pkt->flags & AV_PKT_FLAG_KEY))) {
        if (nal_info) {
            free(nal_info);
            nal_info = nullptr;
            nal_info_len = 0;
        }
        nal_info_len = pkt->size;
        nal_info = (char *)malloc((uint)nal_info_len);
        if (nal_info) {
            memcpy(nal_info, &pkt->data[0], (uint)nal_info_len);
        } else {
            nal_info_len = 0;
        }
    } else if (nal_info) {
        int old_size = pkt->size;
        av_grow_packet(pkt, nal_info_len);
        memmove(&pkt->data[nal_info_len], &pkt->data[0], (uint)old_size);
        memcpy(&pkt->data[0], nal_info, (uint)nal_info_len);
        free(nal_info);
        nal_info = nullptr;
        nal_info_len = 0;
    }
}

/* Cache SPS/PPS from the codec extradata (used by WebRTC SDP generation) */
void cls_h264_encoder::cache_sps_pps()
{
    if (ctx_codec == nullptr || ctx_codec->extradata == nullptr ||
        ctx_codec->extradata_size <= 0) {
        return;
    }

    pthread_mutex_lock(&h264_mutex);
    if (sps_pps.data) {
        free(sps_pps.data);
        sps_pps.data = nullptr;
        sps_pps.sz = 0;
    }

    sps_pps.sz = ctx_codec->extradata_size;
    sps_pps.data = (u_char *)malloc((uint)sps_pps.sz);
    if (sps_pps.data) {
        memcpy(sps_pps.data, ctx_codec->extradata, (uint)sps_pps.sz);
    } else {
        sps_pps.sz = 0;
    }
    pthread_mutex_unlock(&h264_mutex);
}

/* Set up the YUV420P frame data pointers from raw image data */
void cls_h264_encoder::put_yuv420(u_char *yuv_data, int width, int height)
{
    picture->data[0] = yuv_data;
    picture->data[1] = yuv_data + (width * height);
    picture->data[2] = picture->data[1] + ((width * height) / 4);

    picture->linesize[0] = width;
    picture->linesize[1] = width / 2;
    picture->linesize[2] = width / 2;
}

/* Encode a single YUV420P frame.
 * Called from the camera thread in webu_getimg_main().
 * Stores the encoded result in h264_back (not yet visible to consumers). */
int cls_h264_encoder::encode_frame(u_char *yuv_data, int width, int height,
                                    const struct timespec *ts)
{
    int retcd;
    char errstr[128];

    if (ctx_codec == nullptr || picture == nullptr) {
        return -1;
    }

    /* Set up frame data (YUV420P source for all encoder types) */
    put_yuv420(yuv_data, width, height);

    /* Calculate PTS from timestamp */
    if (frame_cnt == 0) {
        start_time = *ts;
        picture->pts = 0;
    } else {
        int64_t pts_interval = ((1000000L * (ts->tv_sec - start_time.tv_sec)) +
            (ts->tv_nsec / 1000) - (start_time.tv_nsec / 1000));
        if (pts_interval < 0) {
            pts_interval = 0;
        }
        picture->pts = av_rescale_q(pts_interval,
            av_make_q(1, 1000000L),
            ctx_codec->time_base);
        if (picture->pts <= base_pts && frame_cnt > 0) {
            picture->pts = base_pts + 1;
        }
    }
    base_pts = picture->pts;

    /* Handle keyframe requests */
    if (keyframe_requested || (frame_cnt % gop_size == 0)) {
        picture->pict_type = AV_PICTURE_TYPE_I;
        keyframe_requested = false;
    } else {
        picture->pict_type = AV_PICTURE_TYPE_NONE;
    }

    /* Allocate packet */
    pkt = av_packet_alloc();
    if (pkt == nullptr) {
        return -1;
    }

    /* Determine which frame to send based on encoder type */
    AVFrame *send_frame = picture;

    if (codec_type == H264_CODEC_VAAPI) {
        /* VAAPI: upload YUV420P data to GPU surface */
        retcd = av_hwframe_get_buffer(ctx_codec->hw_frames_ctx, hw_frame, 0);
        if (retcd < 0) {
            av_strerror(retcd, errstr, sizeof(errstr));
            MOTION_LOG(ERR, TYPE_ENCODER, NO_ERRNO
                , _("H264 shared encoder: VAAPI get_buffer failed: %s"), errstr);
            av_packet_free(&pkt);
            pkt = nullptr;
            return -1;
        }
        /* Transfer YUV420P CPU frame → VAAPI GPU surface
         * (handles YUV420P → NV12 conversion automatically) */
        retcd = av_hwframe_transfer_data(hw_frame, picture, 0);
        if (retcd < 0) {
            av_strerror(retcd, errstr, sizeof(errstr));
            MOTION_LOG(ERR, TYPE_ENCODER, NO_ERRNO
                , _("H264 shared encoder: VAAPI transfer failed: %s"), errstr);
            av_frame_unref(hw_frame);
            av_packet_free(&pkt);
            pkt = nullptr;
            return -1;
        }
        hw_frame->pts = picture->pts;
        hw_frame->pict_type = picture->pict_type;
        send_frame = hw_frame;

    } else if (codec_type == H264_CODEC_QSV) {
        /* QSV: convert YUV420P → NV12 via swscale */
        if (sws_ctx == nullptr) {
            sws_ctx = sws_getContext(
                width, height, AV_PIX_FMT_YUV420P,
                width, height, AV_PIX_FMT_NV12,
                SWS_FAST_BILINEAR, NULL, NULL, NULL);
            if (sws_ctx == nullptr) {
                MOTION_LOG(ERR, TYPE_ENCODER, NO_ERRNO
                    , _("H264 shared encoder: QSV sws_getContext failed"));
                av_packet_free(&pkt);
                pkt = nullptr;
                return -1;
            }
        }
        retcd = av_frame_make_writable(nv12_frame);
        if (retcd < 0) {
            MOTION_LOG(ERR, TYPE_ENCODER, NO_ERRNO
                , _("H264 shared encoder: QSV frame not writable"));
            av_packet_free(&pkt);
            pkt = nullptr;
            return -1;
        }
        sws_scale(sws_ctx,
            (const uint8_t * const *)picture->data, picture->linesize,
            0, height,
            nv12_frame->data, nv12_frame->linesize);
        nv12_frame->pts = picture->pts;
        nv12_frame->pict_type = picture->pict_type;
        send_frame = nv12_frame;
    }

    /* Encode */
    retcd = avcodec_send_frame(ctx_codec, send_frame);

    /* Release VAAPI surface back to pool */
    if (codec_type == H264_CODEC_VAAPI) {
        av_frame_unref(hw_frame);
    }

    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        MOTION_LOG(ERR, TYPE_ENCODER, NO_ERRNO
            , _("H264 shared encoder: error sending frame: %s"), errstr);
        av_packet_free(&pkt);
        pkt = nullptr;
        return -1;
    }

    retcd = avcodec_receive_packet(ctx_codec, pkt);
    if (retcd == AVERROR(EAGAIN)) {
        /* Buffered - no output yet */
        av_packet_free(&pkt);
        pkt = nullptr;
        frame_cnt++;
        return 0;
    }
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        MOTION_LOG(ERR, TYPE_ENCODER, NO_ERRNO
            , _("H264 shared encoder: error receiving packet: %s"), errstr);
        av_packet_free(&pkt);
        pkt = nullptr;
        return -1;
    }

    /* Apply v4l2m2m NAL fixup if needed (only v4l2m2m has this quirk) */
    if (codec_type == H264_CODEC_V4L2M2M) {
        encode_nal();
    }

    /* Cache SPS/PPS from the first keyframe */
    if (frame_cnt == 0 && sps_pps.data == nullptr) {
        cache_sps_pps();
    }

    /* Store encoded data in h264_back */
    if (h264_back.nal_data) {
        free(h264_back.nal_data);
        h264_back.nal_data = nullptr;
    }

    h264_back.nal_sz = pkt->size;
    h264_back.nal_data = (u_char *)malloc((uint)pkt->size);
    if (h264_back.nal_data) {
        memcpy(h264_back.nal_data, pkt->data, (uint)pkt->size);
    } else {
        h264_back.nal_sz = 0;
    }
    h264_back.is_keyframe = (pkt->flags & AV_PKT_FLAG_KEY) != 0;
    h264_back.pts = pkt->pts;
    h264_back.dts = pkt->dts;

    av_packet_free(&pkt);
    pkt = nullptr;

    frame_cnt++;

    return 0;
}

/* Swap front and back buffers under mutex protection.
 * After this call, consumers can read from h264_front. */
void cls_h264_encoder::swap_buffers()
{
    pthread_mutex_lock(&h264_mutex);

    /* Swap the buffer contents */
    ctx_h264_data tmp = h264_front;
    h264_front = h264_back;
    h264_back = tmp;

    pthread_mutex_unlock(&h264_mutex);
}

/* Start the encoder in the specified state */
int cls_h264_encoder::start(h264_encoder_state new_state)
{
    int profile, gop;

    if (new_state == H264_STATE_IDLE) {
        return 0;
    }

    if (new_state == H264_STATE_RECORD_ONLY) {
        profile = MY_PROFILE_H264_HIGH;
        /* Use movie_quality GOP: fps/2, capped similar to cls_movie */
        int fps = cam->cfg->framerate;
        if (fps < 2) fps = 2;
        if (fps <= 5) {
            gop = 1;
        } else if (fps > 30) {
            gop = 15;
        } else {
            gop = fps / 2;
        }
    } else {
        /* WEBRTC_ONLY or BOTH: use WebRTC-compatible settings */
        profile = MY_PROFILE_H264_CONSTRAINED_BASELINE;
        gop = cam->cfg->webrtc_gop;
        if (gop <= 0) gop = 30;
    }

    int retcd = open_encoder(profile, gop);
    if (retcd != 0) {
        MOTION_LOG(ERR, TYPE_ENCODER, NO_ERRNO
            , _("H264 shared encoder: failed to start in state %d"), (int)new_state);
        return -1;
    }

    state = new_state;
    return 0;
}

/* Stop the encoder */
void cls_h264_encoder::stop()
{
    if (state == H264_STATE_IDLE) {
        return;
    }

    MOTION_LOG(NTC, TYPE_ENCODER, NO_ERRNO
        , _("H264 shared encoder stopped (was state %d)"), (int)state);

    close_encoder();
    state = H264_STATE_IDLE;
}

/* Transition the encoder to a new state.
 * May require closing and reopening the encoder if the profile changes. */
void cls_h264_encoder::transition(h264_encoder_state new_state)
{
    if (state == new_state) {
        return;
    }

    MOTION_LOG(NTC, TYPE_ENCODER, NO_ERRNO
        , _("H264 shared encoder: transition %d -> %d"), (int)state, (int)new_state);

    if (new_state == H264_STATE_IDLE) {
        stop();
        return;
    }

    if (state == H264_STATE_IDLE) {
        start(new_state);
        return;
    }

    /* Check if profile change is needed */
    bool old_is_baseline = (state == H264_STATE_WEBRTC_ONLY || state == H264_STATE_BOTH);
    bool new_is_baseline = (new_state == H264_STATE_WEBRTC_ONLY || new_state == H264_STATE_BOTH);

    if (old_is_baseline == new_is_baseline) {
        /* Same profile, just update state */
        state = new_state;
        return;
    }

    /* Profile change required: close and reopen encoder */
    close_encoder();
    start(new_state);
}

/* Called when a WebRTC peer connects */
void cls_h264_encoder::webrtc_peer_connected()
{
    int cnt = ++webrtc_cnct;

    MOTION_LOG(NTC, TYPE_ENCODER, NO_ERRNO
        , _("H264 shared encoder: WebRTC peer connected (total: %d)"), cnt);

    /* Cancel hysteresis if active */
    hysteresis_countdown = 0;

    if (state == H264_STATE_IDLE) {
        transition(H264_STATE_WEBRTC_ONLY);
    } else if (state == H264_STATE_RECORD_ONLY) {
        transition(H264_STATE_BOTH);
    }
    /* WEBRTC_ONLY or BOTH: already correct state */

    /* Request keyframe for the new peer */
    request_keyframe();
}

/* Called when a WebRTC peer disconnects */
void cls_h264_encoder::webrtc_peer_disconnected()
{
    int cnt = --webrtc_cnct;
    if (cnt < 0) {
        webrtc_cnct = 0;
        cnt = 0;
    }

    MOTION_LOG(NTC, TYPE_ENCODER, NO_ERRNO
        , _("H264 shared encoder: WebRTC peer disconnected (remaining: %d)"), cnt);

    if (cnt > 0) {
        return; /* Still have viewers */
    }

    if (state == H264_STATE_WEBRTC_ONLY) {
        transition(H264_STATE_IDLE);
    } else if (state == H264_STATE_BOTH) {
        /* Start hysteresis: stay at Constrained Baseline for 30s.
         * The hysteresis_countdown is decremented each encode_frame call.
         * At 15fps, 30s = 450 frames. */
        int fps = cam->cfg->framerate;
        if (fps < 2) fps = 2;
        hysteresis_countdown = fps * 30;
        /* For now, just transition immediately to RECORD_ONLY.
         * Full hysteresis with delayed profile switch is deferred
         * to avoid complexity in Phase 7b. */
        transition(H264_STATE_RECORD_ONLY);
    }
}

/* Called when movie recording starts */
void cls_h264_encoder::recording_started()
{
    recording = true;

    MOTION_LOG(NTC, TYPE_ENCODER, NO_ERRNO
        , _("H264 shared encoder: recording started (state: %d)"), (int)state);

    if (state == H264_STATE_IDLE) {
        transition(H264_STATE_RECORD_ONLY);
    } else if (state == H264_STATE_WEBRTC_ONLY) {
        /* Encoder already running at Baseline, no restart needed */
        state = H264_STATE_BOTH;
    }
    /* RECORD_ONLY or BOTH: already correct */
}

/* Called when movie recording stops */
void cls_h264_encoder::recording_stopped()
{
    recording = false;

    MOTION_LOG(NTC, TYPE_ENCODER, NO_ERRNO
        , _("H264 shared encoder: recording stopped (state: %d)"), (int)state);

    if (state == H264_STATE_RECORD_ONLY) {
        transition(H264_STATE_IDLE);
    } else if (state == H264_STATE_BOTH) {
        state = H264_STATE_WEBRTC_ONLY;
    }
    /* WEBRTC_ONLY or IDLE: already correct */
}

/* Request a keyframe on the next encode */
void cls_h264_encoder::request_keyframe()
{
    keyframe_requested = true;
}

#endif /* HAVE_WEBRTC */
