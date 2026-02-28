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
 * util.hpp - Common Utilities and FFmpeg Compatibility
 *
 * Header file defining utility functions, FFmpeg API compatibility macros,
 * and common helper functions for string manipulation, time handling,
 * and cross-platform operations.
 *
 */

#ifndef _INCLUDE_UTIL_HPP_
#define _INCLUDE_UTIL_HPP_

#define MYFFVER (LIBAVFORMAT_VERSION_MAJOR * 1000)+LIBAVFORMAT_VERSION_MINOR

/* Modern FFmpeg (6.0+) - Pi 5 with 64-bit OS uses current FFmpeg APIs */
typedef const AVCodec myAVCodec;
typedef const uint8_t myuint;

/* Profile constants - handle FFmpeg rename from FF_PROFILE_* to AV_PROFILE_*
 * (FFmpeg 7+ uses AV_PROFILE_*, older versions use FF_PROFILE_*) */
#ifdef FF_PROFILE_H264_HIGH
    #define MY_PROFILE_H264_HIGH                   FF_PROFILE_H264_HIGH
    #define MY_PROFILE_H264_CONSTRAINED_BASELINE   FF_PROFILE_H264_CONSTRAINED_BASELINE
#else
    #define MY_PROFILE_H264_HIGH                   AV_PROFILE_H264_HIGH
    #define MY_PROFILE_H264_CONSTRAINED_BASELINE   AV_PROFILE_H264_CONSTRAINED_BASELINE
#endif

/* FFmpeg 5.0+ Compatibility Macros - Support FFmpeg 5.0 through 7.x */
/* Minimum version check - Motion requires FFmpeg 5.0 or newer */
#if (MYFFVER < 50000)
    #error "Motion requires FFmpeg 5.0 or newer. Please upgrade your FFmpeg installation."
#endif

/* Codec ID compatibility macros - Provides unified API across FFmpeg versions */
#define MY_CODEC_ID_NONE           AV_CODEC_ID_NONE
#define MY_CODEC_ID_H264           AV_CODEC_ID_H264
#define MY_CODEC_ID_HEVC           AV_CODEC_ID_HEVC
#define MY_CODEC_ID_MPEG2VIDEO     AV_CODEC_ID_MPEG2VIDEO
#define MY_CODEC_ID_MPEG4          AV_CODEC_ID_MPEG4
#define MY_CODEC_ID_VP8            AV_CODEC_ID_VP8
#define MY_CODEC_ID_VP9            AV_CODEC_ID_VP9
#define MY_CODEC_ID_FLV1           AV_CODEC_ID_FLV1
#define MY_CODEC_ID_MJPEG          AV_CODEC_ID_MJPEG
#define MY_CODEC_ID_MSMPEG4V2      AV_CODEC_ID_MSMPEG4V2

/* Pixel format compatibility macros */
#define MY_PIX_FMT_YUV420P         AV_PIX_FMT_YUV420P
#define MY_PIX_FMT_YUVJ420P        AV_PIX_FMT_YUVJ420P
#define MY_PIX_FMT_RGB24           AV_PIX_FMT_RGB24

/* Codec flags compatibility macros */
#define MY_CODEC_FLAG_GLOBAL_HEADER  AV_CODEC_FLAG_GLOBAL_HEADER
#define MY_CODEC_FLAG_QSCALE         AV_CODEC_FLAG_QSCALE

/* AVIO callback typedef - Handles FFmpeg 7.0+ const buffer changes */
/* FFmpeg 7.0+ (libavformat >= 61) changed write_packet to const uint8_t* */
#if LIBAVFORMAT_VERSION_MAJOR < 61
    typedef int (*my_avio_write_cb)(void *opaque, uint8_t *buf, int buf_size);
#else
    typedef int (*my_avio_write_cb)(void *opaque, const uint8_t *buf, int buf_size);
#endif


#ifdef HAVE_GETTEXT
    #include <libintl.h>
    extern int  _nl_msg_cat_cntr;    /* Required for changing the locale dynamically */
#endif

#define _(STRING) mytranslate_text(STRING, 2)

#define SLEEP(seconds, nanoseconds) {              \
                struct timespec ts1;                \
                ts1.tv_sec = seconds;             \
                ts1.tv_nsec = (long)nanoseconds;        \
                while (nanosleep(&ts1, &ts1) == -1); \
        }
#define myfree(x)   {if(x!=nullptr) {free(x);  x=nullptr;}}
#define mydelete(x) {if(x!=nullptr) {delete x; x=nullptr;}}

#if MHD_VERSION >= 0x00097002
    typedef enum MHD_Result mhdrslt; /* Version independent return result from MHD */
#else
    typedef int             mhdrslt; /* Version independent return result from MHD */
#endif

struct ctx_params_item {
    std::string     param_name;       /* The name or description of the ID as requested by user*/
    std::string     param_value;      /* The value that the user wants the control set to*/
};
typedef std::vector<ctx_params_item> vec_params;
struct ctx_params {
    vec_params  params_array;
    int         params_cnt;
    std::string params_desc;
};

    void *mymalloc(size_t nbytes);

    void *myrealloc(void *ptr, size_t size, const char *desc);
    int mycreate_path(const char *path);
    FILE *myfopen(const char *path, const char *mode);
    int myfclose(FILE *fh);
    void mystrftime(cls_camera *cam, char *s, size_t mx_sz
        , const char *usrfmt, const char *fname);
    void mystrftime(cls_camera *cam, std::string &rslt
        , std::string usrfmt, std::string fname);
    void mystrftime(cls_sound *snd, std::string &dst, std::string fmt);
    std::string util_sanitize_shell_chars(const std::string &input);
    void util_exec_command(cls_camera *cam, const char *command, const char *filename);
    void util_exec_command(cls_sound *snd, std::string cmd);
    void util_exec_command(cls_camera *cam, std::string cmd);

    void mythreadname_set(const char *abbr, int threadnbr, const char *threadname);
    void mythreadname_get(char *threadname);
    void mythreadname_get(std::string &threadname);

    char* mytranslate_text(const char *msgid, int setnls);
    void mytranslate_init(void);

    int mystrceq(const char* var1, const char* var2);
    int mystrcne(const char* var1, const char* var2);
    int mystreq(const char* var1, const char* var2);
    int mystrne(const char* var1, const char* var2);
    void mylower(std::string &parm);
    void myltrim(std::string &parm);
    void myrtrim(std::string &parm);
    void mytrim(std::string &parm);
    void myunquote(std::string &parm);

    void myframe_key(AVFrame *frame);
    void myframe_nonkey(AVFrame *frame);
    AVPacket *mypacket_alloc(AVPacket *pkt);

    /* FFmpeg initialization wrapper - Handles av_register_all() for old versions */
    void myffmpeg_init();

    /* Codec finding wrappers - Handles const changes across FFmpeg versions */
    myAVCodec *mycodec_find_encoder(enum AVCodecID id);
    myAVCodec *mycodec_find_decoder(enum AVCodecID id);

    void util_parms_parse(ctx_params *params, std::string parm_desc, std::string confline);
    void util_parms_add_default(ctx_params *params, std::string parm_nm, std::string parm_vl);
    void util_parms_add_default(ctx_params *params, std::string parm_nm, int parm_vl);
    void util_parms_add(ctx_params *params, std::string parm_nm, std::string parm_val);
    void util_parms_update(ctx_params *params, std::string &confline);

    int mtoi(std::string parm);
    int mtoi(char *parm);
    float mtof(char *parm);
    float mtof(std::string parm);
    bool mtob(std::string parm);
    bool mtob(char *parm);
    long mtol(std::string parm);
    long mtol(char *parm);
    std::string mtok(std::string &parm, std::string tok);

    void util_resize(uint8_t *src, int src_w, int src_h
        , uint8_t *dst, int dst_w, int dst_h);

#endif /* _INCLUDE_UTIL_HPP_ */
