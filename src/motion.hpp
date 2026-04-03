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
 * motion.hpp - Main Application Header
 *
 * Primary header file containing global definitions, structures, and
 * includes for the Motion application, defining core data types and
 * constants used throughout the system.
 *
 */

#ifndef _INCLUDE_MOTION_HPP_
#define _INCLUDE_MOTION_HPP_

#include "config.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <stdint.h>
#include <pthread.h>
#include <microhttpd.h>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <regex.h>
#include <dirent.h>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include "zlib.h"

#if defined(HAVE_PTHREAD_NP_H)
    #include <pthread_np.h>
#endif

#ifdef HAVE_SYSTEMD
    #include <systemd/sd-daemon.h>
#endif

#pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wconversion"
    extern "C" {
        #include <libavformat/avformat.h>
        #include <libavutil/imgutils.h>
        #include <libavutil/mathematics.h>
        #include <libavdevice/avdevice.h>
        #include <libavcodec/avcodec.h>
        #include <libavformat/avio.h>
        #include <libswscale/swscale.h>
        #include <libavutil/avutil.h>
        #include "libavutil/buffer.h"
        #include "libavutil/error.h"
        #include "libavutil/hwcontext.h"
        #include "libavutil/mem.h"
    }
#pragma GCC diagnostic pop

#ifdef HAVE_V4L2
    #if defined(HAVE_LINUX_VIDEODEV2_H)
        #include <linux/videodev2.h>
    #else
        #include <sys/videoio.h>
    #endif
#endif

class cls_motapp;
class cls_camera;
class cls_allcam;
class cls_schedule;
class cls_sound;
class cls_algsec;
class cls_alg;
class cls_config;
class cls_config_profile;
class cls_cam_detect;
class cls_dbse;
class cls_draw;
class cls_log;
class cls_movie;
class cls_netcam;
class cls_picture;
class cls_rotate;
class cls_v4l2cam;
class cls_convert;
class cls_libcam;
class cls_webu;
class cls_webu_ans;
class cls_webu_file;
class cls_webu_json;
class cls_webu_common;
class cls_webu_stream;
class cls_thumbnail;

enum MOTION_SIGNAL {
    MOTION_SIGNAL_NONE,
    MOTION_SIGNAL_ALARM,
    MOTION_SIGNAL_USR1,
    MOTION_SIGNAL_SIGHUP,
    MOTION_SIGNAL_SIGTERM
};

enum DEVICE_STATUS {
    STATUS_CLOSED,   /* Device is closed */
    STATUS_INIT,     /* First time initialize */
    STATUS_OPENED    /* Successfully started the device */
};

struct ctx_all_loc {
    int     row;
    int     col;
    int     offset_row;
    int     offset_col;
    int     offset_user_row;
    int     offset_user_col;
    int     scale;
    int     xpct_st;    /*Starting x location of image on percentage basis*/
    int     xpct_en;    /*Ending x location of image on percentage basis*/
    int     ypct_st;    /*Starting y location of image on percentage basis*/
    int     ypct_en;    /*Ending y location of image on percentage basis*/
};

struct ctx_all_sizes {
    int     src_w;
    int     src_h;
    int     src_sz;
    int     dst_w;
    int     dst_h;
    int     dst_sz;
    bool    reset;
};

struct ctx_stream_data {
    u_char  *jpg_data;  /* Image compressed as JPG */
    int     jpg_sz;     /* The number of bytes for jpg */
    int     consumed;   /* Bool for whether the jpeg data was consumed*/
    u_char  *img_data;  /* The base data used for image */
    int     jpg_cnct;   /* Counter of the number of jpg connections*/
    int     all_cnct;   /* Counter of the number of all camera connections */
    struct timespec last_encode_time;  /* Timestamp of last encode */
    float           encode_fps;        /* Running average FPS */
};

struct ctx_stream {
    pthread_mutex_t  mutex;
    ctx_stream_data  norm;       /* Copy of the image to use for web stream*/
    ctx_stream_data  sub;        /* Copy of the image to use for web stream*/
    ctx_stream_data  motion;     /* Copy of the image to use for web stream*/
    ctx_stream_data  source;     /* Copy of the image to use for web stream*/
    ctx_stream_data  secondary;  /* Copy of the image to use for web stream*/
};

/* Forward declaration for delete progress tracking (defined in webu.hpp) */
struct ctx_delete_progress;

/* Hardware encoder availability (probed at startup, cached) */
struct ctx_hw_encoders {
    bool h264_v4l2m2m;  /* true if v4l2m2m H.264 hardware encoder is available */
    bool h264_nvenc;    /* true if NVIDIA NVENC H.264 encoder is available */
    bool h264_vaapi;    /* true if VAAPI H.264 encoder is available (Intel/AMD) */
    bool h264_qsv;      /* true if Intel Quick Sync H.264 encoder is available */
    bool probed;        /* true after startup probe completes */
};

class cls_motapp {
    public:
        cls_motapp();
        ~cls_motapp();

        std::vector<cls_camera*>    cam_list;
#ifndef HAVE_IPCAM
        std::vector<cls_sound*>     snd_list;
#endif

        bool    reload_all;
        bool    cam_add;
        int     cam_delete;
        int     cam_cnt;
#ifndef HAVE_IPCAM
        int     snd_cnt;
#endif

        int     argc;
        char    **argv;
        std::string user_pause;

        cls_config          *conf_src;
        cls_config          *cfg;
        cls_webu            *webu;
#ifndef HAVE_IPCAM
        cls_dbse            *dbse;
        cls_config_profile  *profiles;
        cls_allcam          *allcam;
        cls_schedule        *schedule;
        cls_thumbnail       *thumbnail;
        cls_cam_detect      *cam_detect;
#endif

        pthread_mutex_t     mutex_camlst;       /* Lock the list of cams while adding/removing */
        pthread_mutex_t     mutex_post;         /* mutex to allow for processing of post actions*/

        /* Delete progress tracking */
        std::map<std::string, ctx_delete_progress> delete_progress_map;
        pthread_mutex_t     mutex_delete_progress;

        /* Hardware encoder availability (probed once at startup) */
        ctx_hw_encoders     hw_encoders;

        void signal_process();
        void cleanup_delete_progress();
        bool check_devices();
        void check_restart();
        void init(int p_argc, char *p_argv[]);
        void deinit();
        void camera_add();
        void camera_delete();

    private:
        void pid_write();
        void pid_remove();
        void daemon();
        void av_init();
        void av_deinit();
        void ntc();
        void watchdog(uint camindx);
};

#endif /* _INCLUDE_MOTION_HPP_ */
