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

#ifndef _INCLUDE_LIBCAM_HPP_
#define _INCLUDE_LIBCAM_HPP_
    #ifdef HAVE_LIBCAM
        #include <queue>
        #include <vector>
        #include <algorithm>
        #include <atomic>
        #include <sys/mman.h>
        #include <libcamera/libcamera.h>

        #define LIBCAMVER (LIBCAMERA_VERSION_MAJOR * 1000000)+(LIBCAMERA_VERSION_MINOR* 1000) + LIBCAMERA_VERSION_PATCH

        /* Buffers and sizes for planes of image*/
        struct ctx_imgmap {
            uint8_t *buf;
            int     bufsz;
        };

        /* Request with associated buffer index for multi-buffer support */
        struct ctx_reqinfo {
            libcamera::Request *request;
            int buffer_idx;
        };

        /* Pending runtime control updates (hot-reload brightness/contrast/ISO/AWB) */
        struct ctx_pending_controls {
            float brightness = 0.0f;
            float contrast = 1.0f;
            float iso = 100.0f;  // ISO 100-6400 (converted to AnalogueGain)
            // AWB controls
            bool awb_enable = true;
            int awb_mode = 0;           // libcamera native: 0=Auto
            bool awb_locked = false;
            int colour_temp = 0;        // 0 = disabled
            float colour_gain_r = 0.0f; // 0 = auto
            float colour_gain_b = 0.0f; // 0 = auto
            std::atomic<bool> dirty{false};  /* Must use brace initialization for atomics */
        };

        class cls_libcam {
            public:
                cls_libcam(cls_camera *p_cam);
                ~cls_libcam();
                int next(ctx_image_data *img_data);
                void noimage();
                void set_brightness(float value);
                void set_contrast(float value);
                void set_iso(float value);
                void set_awb_enable(bool value);
                void set_awb_mode(int value);
                void set_awb_locked(bool value);
                void set_colour_temp(int value);
                void set_colour_gains(float red, float blue);
            private:
                cls_camera  *cam;
                ctx_params  *params;

                std::unique_ptr<libcamera::CameraManager>          cam_mgr;
                std::shared_ptr<libcamera::Camera>                 camera;
                std::unique_ptr<libcamera::CameraConfiguration>    config;
                std::unique_ptr<libcamera::FrameBufferAllocator>   frmbuf;
                std::vector<std::unique_ptr<libcamera::Request>>   requests;

                std::queue<ctx_reqinfo>            req_queue;      /* Changed: now includes buffer index */
                libcamera::ControlList             controls;
                ctx_imgmap                         membuf;         /* Legacy single buffer (kept for compatibility) */
                std::vector<ctx_imgmap>            membuf_pool;    /* NEW: Multi-buffer pool */
                ctx_pending_controls               pending_ctrls;  /* Hot-reload brightness/contrast */
                bool    started_cam;
                bool    started_mgr;
                bool    started_aqr;
                bool    started_req;
                int     reconnect_count;
                void log_orientation();
                void log_controls();
                void log_draft();

                int libcam_start();
                void libcam_stop();

                void start_params();
                int start_mgr();
                int start_config();
                int start_req();
                int start_capture();
                void config_orientation();
                void config_controls();
                void config_control_item(std::string pname, std::string pvalue);
                void req_complete(libcamera::Request *request);
                int req_add(libcamera::Request *request);
                void apply_pending_controls();

                /* NEW: Pi 5 camera filtering and selection */
                std::vector<std::shared_ptr<libcamera::Camera>> get_pi_cameras();
                std::shared_ptr<libcamera::Camera> find_camera_by_model(const std::string &model);

        };
    #else
        #define LIBCAMVER 0
        class cls_libcam {
            public:
                cls_libcam(cls_camera *p_cam);
                ~cls_libcam();
                int next(ctx_image_data *img_data);
                void noimage();
            private:
                cls_camera  *cam;
        };
    #endif

#endif /* _INCLUDE_LIBCAM_HPP_ */
