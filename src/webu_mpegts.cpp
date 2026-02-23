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
 * webu_mpegts.cpp - MPEG-TS Streaming Implementation
 *
 * This module provides H.264 video streaming in MPEG Transport Stream
 * format over HTTP, delivering lower-latency video streams compared to
 * MJPEG for compatible browsers and applications.
 *
 */

#include "motion.hpp"
#include "util.hpp"
#include "camera.hpp"
#include "allcam.hpp"
#include "conf.hpp"
#include "logger.hpp"
#include "picture.hpp"
#include "webu.hpp"
#include "webu_ans.hpp"
#include "webu_stream.hpp"
#include "webu_mpegts.hpp"

/****** Callback functions for MHD ****************************************/

#if LIBAVFORMAT_VERSION_MAJOR < 61
/* FFmpeg 6.x and earlier - write_packet callback uses non-const uint8_t* */
static int webu_mpegts_avio_buf(void *opaque, uint8_t *buf, int buf_size)
#else
/* FFmpeg 7.0+ - write_packet callback uses const uint8_t* */
static int webu_mpegts_avio_buf(void *opaque, const uint8_t *buf, int buf_size)
#endif
{
    cls_webu_mpegts *webu_mpegts;
    webu_mpegts =(cls_webu_mpegts *)opaque;
    return webu_mpegts->avio_buf(buf, buf_size);
}

static ssize_t webu_mpegts_response(void *cls, uint64_t pos, char *buf, size_t max)
{
    cls_webu_mpegts *webu_mpegts;
    (void)pos;
    webu_mpegts =(cls_webu_mpegts *)cls;
    return webu_mpegts->response(buf, max);
}

/********Class Functions ****************************************************/

void cls_webu_mpegts::free_nal()
{
    if (nal_info) {
        free(nal_info);
        nal_info = nullptr;
        nal_info_len = 0;
    }
}

void cls_webu_mpegts::encode_nal(AVPacket *pkt)
{
    // h264_v4l2m2m separates SPS/PPS NAL units from the first frame as a
    // non-KEY packet. In movie.cpp this packet has pts=0, but in streaming
    // the first PTS is a real-time value. Use nal_info==nullptr to detect
    // the first non-KEY packet instead of checking pts==0.
    if ((nal_info == nullptr) && (!(pkt->flags & AV_PKT_FLAG_KEY))) {
        nal_info_len = pkt->size;
        nal_info = (char*)mymalloc((uint)nal_info_len);
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
        free_nal();
    }
}

/*Special allocation of video buffer for v4l2m2m codec*/
int cls_webu_mpegts::alloc_video_buffer(AVFrame *frame, int align)
{
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get((enum AVPixelFormat)frame->format);
    int ret, i, padded_height;
    int plane_padding = FFMAX(16 + 16/*STRIDE_ALIGN*/, align);

    if (!desc) {
        return AVERROR(EINVAL);
    }

    if ((ret = av_image_check_size(
            (uint)frame->width, (uint)frame->height
            , 0, nullptr)) < 0) {
        return ret;
    }

    if (!frame->linesize[0]) {
        if (align <= 0) {
            align = 32; /* STRIDE_ALIGN. Should be av_cpu_max_align() */
        }

        for(i=1; i<=align; i+=i) {
            ret = av_image_fill_linesizes(frame->linesize,(enum AVPixelFormat) frame->format,
                                          FFALIGN(frame->width, i));
            if (ret < 0) {
                return ret;
            }
            if (!(frame->linesize[0] & (align-1))) {
                break;
            }
        }

        for (i = 0; i < 4 && frame->linesize[i]; i++)
            frame->linesize[i] = FFALIGN(frame->linesize[i], align);
    }

    padded_height = FFALIGN(frame->height, 32);
    ret = av_image_fill_pointers(frame->data
            ,(enum AVPixelFormat) frame->format
            , padded_height
            , nullptr
            , frame->linesize);
    if (ret < 0) {
        return ret;
    }

    frame->buf[0] = av_buffer_alloc((uint)(ret + 4*plane_padding));
    if (!frame->buf[0]) {
        ret = AVERROR(ENOMEM);
        av_frame_unref(frame);
        return ret;
    }
    frame->buf[1] = av_buffer_alloc((uint)(ret + 4*plane_padding));
    if (!frame->buf[1]) {
        ret = AVERROR(ENOMEM);
        av_frame_unref(frame);
        return ret;
    }

    frame->data[0] = frame->buf[0]->data;
    frame->data[1] = frame->buf[1]->data;
    frame->data[2] = frame->data[1] + ((frame->width * padded_height) / 4);

    frame->extended_data = frame->data;

    return 0;
}

int cls_webu_mpegts::pic_send(unsigned char *img)
{
    int retcd;
    char errstr[128];
    struct timespec curr_ts;
    int64_t pts_interval;

    if (picture == NULL) {
        picture = av_frame_alloc();
        picture->format = ctx_codec->pix_fmt;
        picture->width  = ctx_codec->width;
        picture->height = ctx_codec->height;

        if (encoder_name == "h264_v4l2m2m") {
            retcd = alloc_video_buffer(picture, 32);
            if (retcd < 0) {
                av_strerror(retcd, errstr, sizeof(errstr));
                MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
                    ,_("Failed to allocate v4l2m2m aligned buffer: %s"), errstr);
                av_frame_free(&picture);
                picture = NULL;
                return -1;
            }
        } else {
            picture->linesize[0] = ctx_codec->width;
            picture->linesize[1] = ctx_codec->width / 2;
            picture->linesize[2] = ctx_codec->width / 2;
        }

        picture->pict_type = AV_PICTURE_TYPE_I;
        myframe_key(picture);
        picture->pts = 1;
    }

    if (encoder_name == "h264_v4l2m2m") {
        int y;
        int src_stride = ctx_codec->width;
        int dst_stride = picture->linesize[0];
        int src_uv_stride = ctx_codec->width / 2;
        int dst_uv_stride = picture->linesize[1];
        unsigned char *src_u = img + (ctx_codec->width * ctx_codec->height);
        unsigned char *src_v = src_u + (ctx_codec->width * ctx_codec->height / 4);

        for (y = 0; y < ctx_codec->height; y++)
            memcpy(picture->data[0] + y * dst_stride, img + y * src_stride, src_stride);
        for (y = 0; y < ctx_codec->height / 2; y++) {
            memcpy(picture->data[1] + y * dst_uv_stride, src_u + y * src_uv_stride, src_uv_stride);
            memcpy(picture->data[2] + y * dst_uv_stride, src_v + y * src_uv_stride, src_uv_stride);
        }
    } else {
        picture->data[0] = img;
        picture->data[1] = picture->data[0] +
            (ctx_codec->width * ctx_codec->height);
        picture->data[2] = picture->data[1] +
            ((ctx_codec->width * ctx_codec->height) / 4);
    }

    clock_gettime(CLOCK_REALTIME, &curr_ts);
    pts_interval = ((1000000L * (curr_ts.tv_sec - start_time.tv_sec)) +
        (curr_ts.tv_nsec/1000) - (start_time.tv_nsec/1000));
    picture->pts = av_rescale_q(pts_interval
        ,av_make_q(1,1000000L), ctx_codec->time_base);

    retcd = avcodec_send_frame(ctx_codec, picture);
    if (retcd < 0 ) {
        av_strerror(retcd, errstr, sizeof(errstr));
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            , _("Error sending frame for encoding:%s"), errstr);
        av_frame_free(&picture);
        picture = NULL;
        return -1;
    }

    return 0;
}

int cls_webu_mpegts::pic_get()
{
    int retcd;
    char errstr[128];
    AVPacket *pkt;

    pkt = NULL;
    pkt = mypacket_alloc(pkt);

    retcd = avcodec_receive_packet(ctx_codec, pkt);
    if (retcd == AVERROR(EAGAIN)) {
        av_packet_free(&pkt);
        pkt = NULL;
        return -1;
    }
    if (retcd < 0 ) {
        av_strerror(retcd, errstr, sizeof(errstr));
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            ,_("Error receiving encoded packet video:%s"), errstr);
        //Packet is freed upon failure of encoding
        return -1;
    }

    if (encoder_name == "h264_v4l2m2m") {
        encode_nal(pkt);
    }

    pkt->pts = picture->pts;

    retcd =  av_interleaved_write_frame(fmtctx, pkt);
    if (retcd < 0 ) {
        av_strerror(retcd, errstr, sizeof(errstr));
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            ,_("Error while writing video frame. %s"), errstr);
        return -1;
    }

    av_packet_free(&pkt);
    pkt = NULL;

    return 0;
}

void cls_webu_mpegts::resetpos()
{
    stream_pos = 0;
    webus->resp_used = 0;
}

int cls_webu_mpegts::getimg()
{
    ctx_stream_data *strm;
    struct timespec curr_ts;
    unsigned char *img_data;
    int img_sz;

    if (webus->check_finish() == true) {
        resetpos();
        return 0;
    }

    clock_gettime(CLOCK_REALTIME, &curr_ts);

    memset(webus->resp_image, '\0', webus->resp_size);
    webus->resp_used = 0;

    if (webua->device_id > 0) {
        /* Assign to a local pointer the stream we want */
        if (webua->cnct_type == WEBUI_CNCT_TS_FULL) {
            strm = &webua->cam->stream.norm;
        } else if (webua->cnct_type == WEBUI_CNCT_TS_SUB) {
            strm = &webua->cam->stream.sub;
        } else if (webua->cnct_type == WEBUI_CNCT_TS_MOTION) {
            strm = &webua->cam->stream.motion;
        } else if (webua->cnct_type == WEBUI_CNCT_TS_SOURCE) {
            strm = &webua->cam->stream.source;
        } else if (webua->cnct_type == WEBUI_CNCT_TS_SECONDARY) {
            strm = &webua->cam->stream.secondary;
        } else {
            return 0;
        }
        img_sz = (ctx_codec->width * ctx_codec->height * 3)/2;
        img_data = (unsigned char*) mymalloc((uint)img_sz);
        pthread_mutex_lock(&webua->cam->stream.mutex);
            if (strm->img_data == NULL) {
                memset(img_data, 0x00, (uint)img_sz);
            } else {
                memcpy(img_data, strm->img_data, (uint)img_sz);
                strm->consumed = true;
            }
        pthread_mutex_unlock(&webua->cam->stream.mutex);
    } else {
        if (webua->cnct_type == WEBUI_CNCT_TS_FULL) {
            strm = &webua->app->allcam->stream.norm;
        } else if (webua->cnct_type == WEBUI_CNCT_TS_SUB) {
            strm = &webua->app->allcam->stream.sub;
        } else if (webua->cnct_type == WEBUI_CNCT_TS_MOTION) {
            strm = &webua->app->allcam->stream.motion;
        } else if (webua->cnct_type == WEBUI_CNCT_TS_SOURCE) {
            strm = &webua->app->allcam->stream.source;
        } else if (webua->cnct_type == WEBUI_CNCT_TS_SECONDARY) {
            strm = &webua->app->allcam->stream.secondary;
        } else {
            return 0;
        }
        img_sz = app->allcam->all_sizes.dst_sz;
        img_data = (unsigned char*) mymalloc((uint)img_sz);
        pthread_mutex_lock(&webua->app->allcam->stream.mutex);
            if (strm->img_data == nullptr) {
                memset(img_data, 0x00, (uint)img_sz);
            } else {
                memcpy(img_data, strm->img_data, (uint)img_sz);
                strm->consumed = true;
            }
        pthread_mutex_unlock(&webua->app->allcam->stream.mutex);

    }

    if (pic_send(img_data) < 0) {
        myfree(img_data);
        return -1;
    }
    myfree(img_data);

    if (pic_get() < 0) {
        return -1;
    }

    return 0;
}

#if LIBAVFORMAT_VERSION_MAJOR < 61
/* FFmpeg 6.x and earlier - write_packet callback uses non-const uint8_t* */
int cls_webu_mpegts::avio_buf(uint8_t *buf, int buf_size)
#else
/* FFmpeg 7.0+ - write_packet callback uses const uint8_t* */
int cls_webu_mpegts::avio_buf(const uint8_t *buf, int buf_size)
#endif
{
    if (webus->resp_size < (size_t)buf_size + webus->resp_used) {
        webus->resp_size = (size_t)buf_size + webus->resp_used;
        webus->resp_image = (unsigned char*)realloc(
            webus->resp_image, webus->resp_size);
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            ,_("resp_image reallocated %d %d %d")
            ,webus->resp_size
            ,webus->resp_used
            ,buf_size);
    }

    memcpy(webus->resp_image + webus->resp_used, buf, (uint)buf_size);
    webus->resp_used += (uint)buf_size;

    return buf_size;
}

ssize_t cls_webu_mpegts::response(char *buf, size_t max)
{
    size_t sent_bytes;

    if (webus->check_finish()) {
        return -1;
    }

    if (ctx_codec != nullptr) {
        if ((webua->device_id == 0) &&
            ((webua->app->allcam->all_sizes.dst_h != ctx_codec->height ) ||
             (webua->app->allcam->all_sizes.dst_w != ctx_codec->width))) {
            return -1;
        }
    }

    if (stream_pos == 0) {
        if ((webus->time_last.tv_sec - st_mono_time.tv_sec) < 2) {
            webus->stream_fps = 30;
        } else {
            webus->set_fps();
        }
        webus->delay();
        resetpos();
        if (getimg() < 0) {
            return 0;
        }
    }

    /* If we don't have anything in the avio buffer at this point bail out */
    if (webus->resp_used == 0) {
        resetpos();
        return 0;
    }

    if ((webus->resp_used - stream_pos) > max) {
        sent_bytes = max;
    } else {
        sent_bytes = webus->resp_used - stream_pos;
    }

    memcpy(buf, webus->resp_image + stream_pos, (uint)sent_bytes);

    stream_pos = stream_pos + sent_bytes;
    if (stream_pos >= webus->resp_used) {
        stream_pos = 0;
    }

    return (ssize_t)sent_bytes;
}

int cls_webu_mpegts::open_mpegts()
{
    int retcd, img_w, img_h;
    char errstr[128];
    unsigned char   *buf_image;
    AVStream        *strm;
    const AVCodec   *codec;
    AVDictionary    *opts;
    size_t          aviobuf_sz;

    opts = NULL;
    webus->stream_fps = 30;
    aviobuf_sz = 4096;
    clock_gettime(CLOCK_REALTIME, &start_time);
    clock_gettime(CLOCK_MONOTONIC, &st_mono_time);

    fmtctx = avformat_alloc_context();
    fmtctx->oformat = av_guess_format("mpegts", NULL, NULL);
    fmtctx->video_codec_id = MY_CODEC_ID_H264;

    codec = avcodec_find_encoder_by_name("h264_v4l2m2m");
    if (codec != nullptr) {
        encoder_name = "h264_v4l2m2m";
        MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO
            ,_("Using hardware H.264 encoder (h264_v4l2m2m)"));
    } else {
        codec = mycodec_find_encoder(MY_CODEC_ID_H264);
        encoder_name = codec ? codec->name : "";
        MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO
            ,_("Using software H.264 encoder (%s)"), encoder_name.c_str());
    }
    strm = avformat_new_stream(fmtctx, codec);

    if (webua->device_id > 0) {
        if ((webua->cnct_type == WEBUI_CNCT_TS_SUB) &&
            ((webua->cam->imgs.width  % 16) == 0) &&
            ((webua->cam->imgs.height % 16) == 0)) {
            img_w = (webua->cam->imgs.width/2);
            img_h = (webua->cam->imgs.height/2);
        } else {
            img_w = webua->cam->imgs.width;
            img_h = webua->cam->imgs.height;
        }
    } else {
        if (webus->all_ready() == false) {
            return -1;
        }
        img_w = app->allcam->all_sizes.dst_w;
        img_h = app->allcam->all_sizes.dst_h;
    }

    ctx_codec = avcodec_alloc_context3(codec);
    ctx_codec->gop_size      = 15;
    ctx_codec->codec_id      = MY_CODEC_ID_H264;
    ctx_codec->codec_type    = AVMEDIA_TYPE_VIDEO;
    ctx_codec->width         = img_w;
    ctx_codec->height        = img_h;
    ctx_codec->time_base.num = 1;
    ctx_codec->time_base.den = 90000;
    ctx_codec->pix_fmt       = AV_PIX_FMT_YUV420P;
    ctx_codec->framerate.num  = 1;
    ctx_codec->framerate.den  = 1;

    if (encoder_name == "h264_v4l2m2m") {
        /* v4l2m2m does not properly populate extradata with GLOBAL_HEADER,
         * so we omit it. SPS/PPS are delivered inline via encode_nal(). */
        ctx_codec->bit_rate      = webua->cam->cfg->stream_h264_bitrate;
        ctx_codec->max_b_frames  = 0;
        ctx_codec->profile       = MY_PROFILE_H264_HIGH;
        av_dict_set(&opts, "preset", "superfast", 0);
        av_dict_set(&opts, "tune", "zerolatency", 0);
    } else {
        char crf[10];
        ctx_codec->flags         |= AV_CODEC_FLAG_GLOBAL_HEADER;
        snprintf(crf, 10, "%d", webua->cam->cfg->stream_h264_quality);
        ctx_codec->bit_rate      = webua->cam->cfg->stream_h264_bitrate;
        ctx_codec->max_b_frames  = 0;
        av_opt_set(ctx_codec->priv_data, "profile", "main", 0);
        av_opt_set(ctx_codec->priv_data, "crf", crf, 0);
        av_opt_set(ctx_codec->priv_data, "tune", "zerolatency", 0);
        av_opt_set(ctx_codec->priv_data, "preset",
            webua->cam->cfg->stream_h264_preset.c_str(), 0);
    }
    av_dict_set(&opts, "movflags", "empty_moov", 0);

    retcd = avcodec_open2(ctx_codec, codec, &opts);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            ,_("Failed to open codec context for %dx%d transport stream: %s")
            , img_w, img_h, errstr);
        av_dict_free(&opts);
        return -1;
    }

    retcd = avcodec_parameters_from_context(strm->codecpar, ctx_codec);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            ,_("Failed to copy decoder parameters!: %s"), errstr);
        av_dict_free(&opts);
        return -1;
    }

    if (webua->device_id == 0) {
        webus->all_buffer();
    } else {
        webus->one_buffer();
    }


    buf_image = (unsigned char*)av_malloc(aviobuf_sz);
    fmtctx->pb = avio_alloc_context(
        buf_image, (int)aviobuf_sz, 1, this
        , NULL, &webu_mpegts_avio_buf, NULL);
    fmtctx->flags = AVFMT_FLAG_CUSTOM_IO;

    retcd = avformat_write_header(fmtctx, &opts);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO
            ,_("Failed to write header!: %s"), errstr);
        av_dict_free(&opts);
        return -1;
    }

    stream_pos = 0;
    webus->resp_used = 0;

    av_dict_free(&opts);

    return 0;
}

mhdrslt cls_webu_mpegts::main()
{
    mhdrslt retcd;
    struct MHD_Response *response;
    int indx;

    if (webua->device_id == 0) {
        if (webus->all_ready() == false) {
            return MHD_NO;
        }
    }

    if (open_mpegts() < 0 ) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO, _("Unable to open mpegts"));
        return MHD_NO;
    }

    clock_gettime(CLOCK_MONOTONIC, &webus->time_last);

    response = MHD_create_response_from_callback (MHD_SIZE_UNKNOWN, 16384
        ,&webu_mpegts_response, this, NULL);
    if (!response) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO, _("Invalid response"));
        return MHD_NO;
    }

    if (webu->wb_headers->params_cnt > 0) {
        for (indx=0;indx<webu->wb_headers->params_cnt;indx++) {
            MHD_add_response_header (response
                , webu->wb_headers->params_array[indx].param_name.c_str()
                , webu->wb_headers->params_array[indx].param_value.c_str());
        }
    }

    MHD_add_response_header(response, "Content-Type", "video/mp2t");
    MHD_add_response_header(response, "Cache-Control",
        "no-store, no-cache, must-revalidate, max-age=0");
    MHD_add_response_header(response, "Pragma", "no-cache");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");

    retcd = MHD_queue_response (webua->connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);

    return retcd;
}

cls_webu_mpegts::cls_webu_mpegts(cls_webu_ans *p_webua, cls_webu_stream *p_webus)
{
    app    = p_webua->app;
    webu   = p_webua->webu;
    webua  = p_webua;
    webus  = p_webus;

    stream_pos    = 0;
    picture = nullptr;
    ctx_codec = nullptr;
    fmtctx = nullptr;
    nal_info = nullptr;
    nal_info_len = 0;
}

cls_webu_mpegts::~cls_webu_mpegts()
{
    app    = nullptr;
    webu   = nullptr;
    webua  = nullptr;
    free_nal();
    if (picture != nullptr) {
        av_frame_free(&picture);
        picture = nullptr;
    }
    if (ctx_codec != nullptr) {
        avcodec_free_context(&ctx_codec);
        ctx_codec = nullptr;
    }
    if (fmtctx != nullptr) {
        if (fmtctx->pb != nullptr) {
            if (fmtctx->pb->buffer != nullptr) {
                av_free(fmtctx->pb->buffer);
                fmtctx->pb->buffer = nullptr;
            }
            avio_context_free(&fmtctx->pb);
            fmtctx->pb = nullptr;
        }
        avformat_free_context(fmtctx);
        fmtctx = nullptr;
    }
}
