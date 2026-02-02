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
 * cam_detect.cpp - Camera Auto-Detection Implementation
 *
 * Implements intelligent camera detection for Pi cameras (CSI/libcamera),
 * USB/V4L2 cameras with device info extraction, blacklist filtering,
 * and sensor-aware default configuration.
 *
 */

#include "motion.hpp"
#include "cam_detect.hpp"
#include "camera.hpp"
#include "logger.hpp"
#include "conf.hpp"
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>

#ifdef HAVE_V4L2
    #include <linux/videodev2.h>
    #include <sys/ioctl.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

#ifdef HAVE_LIBCAM
    #include <libcamera/libcamera.h>
    using namespace libcamera;
#endif

cls_cam_detect::cls_cam_detect(cls_motapp *p_app)
{
    app = p_app;
}

cls_cam_detect::~cls_cam_detect()
{
}

/* Read platform model from device tree */
std::string cls_cam_detect::get_pi_model()
{
    std::ifstream file("/proc/device-tree/model");
    std::string model;

    if (file.is_open()) {
        std::getline(file, model);
        file.close();

        /* Parse out Pi version: "Raspberry Pi 5 Model B Rev 1.0" -> "Pi 5" */
        if (model.find("Raspberry Pi") != std::string::npos) {
            size_t pos = model.find("Raspberry Pi");
            std::string version = model.substr(pos + 13);

            /* Extract just the number/version */
            size_t space = version.find(' ');
            if (space != std::string::npos) {
                version = version.substr(0, space);
                return "Pi " + version;
            }
        }
    }

    return "";
}

/* Check if running on Raspberry Pi */
bool cls_cam_detect::is_raspberry_pi()
{
    struct stat buffer;
    return (stat("/proc/device-tree/model", &buffer) == 0);
}

/* Check if libcamera is available */
bool cls_cam_detect::has_libcamera_support()
{
#ifdef HAVE_LIBCAM
    return true;
#else
    return false;
#endif
}

/* Check if V4L2 is available */
bool cls_cam_detect::has_v4l2_support()
{
#ifdef HAVE_V4L2
    return true;
#else
    return false;
#endif
}

/* Get platform information */
ctx_platform_info cls_cam_detect::get_platform_info()
{
    ctx_platform_info info;

    info.is_raspberry_pi = is_raspberry_pi();
    info.pi_model = info.is_raspberry_pi ? get_pi_model() : "";
    info.has_libcamera = has_libcamera_support();
    info.has_v4l2 = has_v4l2_support();

    MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO
        , "Platform: %s%s, libcamera: %s, V4L2: %s"
        , info.is_raspberry_pi ? info.pi_model.c_str() : "Generic"
        , info.is_raspberry_pi ? "" : " Linux"
        , info.has_libcamera ? "yes" : "no"
        , info.has_v4l2 ? "yes" : "no");

    return info;
}

/* Apply sensor-specific defaults */
void cls_cam_detect::apply_sensor_defaults(ctx_detected_cam &cam)
{
    /* Sensor-aware defaults based on known Pi camera modules */
    if (cam.sensor_model == "imx708") {
        /* Pi Camera v3 (12MP) - optimized defaults */
        cam.default_width = 1920;
        cam.default_height = 1080;
        cam.default_fps = 30;
    } else if (cam.sensor_model == "imx219") {
        /* Pi Camera v2 (8MP) - optimized defaults */
        cam.default_width = 1640;
        cam.default_height = 1232;
        cam.default_fps = 30;
    } else if (cam.sensor_model == "imx477") {
        /* Pi HQ Camera (12.3MP) - optimized defaults */
        cam.default_width = 1920;
        cam.default_height = 1080;
        cam.default_fps = 30;
    } else if (cam.sensor_model == "imx296") {
        /* Pi GS Camera (1.6MP) */
        cam.default_width = 1456;
        cam.default_height = 1088;
        cam.default_fps = 60;
    } else {
        /* Generic defaults */
        cam.default_width = 1280;
        cam.default_height = 720;
        cam.default_fps = 15;
    }
}

/* Check if device is already configured */
bool cls_cam_detect::is_device_configured(const std::string &device_id,
                                          const std::string &device_path)
{
    for (int i = 0; i < app->cam_cnt; i++) {
        cls_camera *cam = app->cam_list[i];

        /* Check libcam_device */
        std::string cfg_libcam = cam->cfg->libcam_device;
        if (!cfg_libcam.empty() && cfg_libcam == device_path) {
            return true;
        }

        /* Check v4l2_device (can be /dev/video0 or /dev/v4l/by-id/...) */
        std::string cfg_v4l2 = cam->cfg->v4l2_device;
        if (!cfg_v4l2.empty()) {
            if (cfg_v4l2 == device_path || cfg_v4l2 == device_id) {
                return true;
            }
        }
    }

    return false;
}

#ifdef HAVE_LIBCAM
/* Detect Pi cameras using libcamera */
std::vector<ctx_detected_cam> cls_cam_detect::detect_libcam()
{
    std::vector<ctx_detected_cam> cameras;

    try {
        /* Check if any camera is already using libcamera.
         * libcamera does not support multiple CameraManager instances in the
         * same process — creating a second one while cameras are active will
         * crash (SIGSEGV).  Skip detection when libcamera is already in use. */
        pthread_mutex_lock(&app->mutex_camlst);
        bool libcam_active = false;
        for (int i = 0; i < app->cam_cnt; i++) {
            if (app->cam_list[i] != nullptr &&
                app->cam_list[i]->has_libcam()) {
                MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO
                    , "libcamera already active on camera %d, skipping detection"
                    , i + 1);
                libcam_active = true;
                break;
            }
        }
        pthread_mutex_unlock(&app->mutex_camlst);
        if (libcam_active) {
            return cameras;
        }

        /* No active libcamera session — safe to create our own */
        std::unique_ptr<CameraManager> cam_mgr = std::make_unique<CameraManager>();

        int retcd = cam_mgr->start();
        if (retcd < 0) {
            MOTION_LOG(INF, TYPE_ALL, NO_ERRNO
                , "Camera manager not available, skipping libcamera detection");
            return cameras;
        }

        MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO
            , "cam_mgr started. Total cameras available: %d"
            , (int)cam_mgr->cameras().size());

        /* Filter for Pi cameras (exclude USB/UVC) */
        for (const auto& cam_item : cam_mgr->cameras()) {
            std::string id = cam_item->id();

            /* Filter out USB cameras (UVC devices)
             * Pi cameras have IDs like: /base/axi/pcie@120000/rp1/i2c@88000/imx708@1a
             * USB cameras have IDs containing "usb" or are UVC devices
             */
            std::string id_lower = id;
            std::transform(id_lower.begin(), id_lower.end(), id_lower.begin(), ::tolower);

            if (id_lower.find("usb") != std::string::npos ||
                id_lower.find("uvc") != std::string::npos) {
                MOTION_LOG(DBG, TYPE_ALL, NO_ERRNO
                    , "Skipping USB camera: %s", id.c_str());
                continue;
            }

            ctx_detected_cam detected;
            detected.type = CAM_DETECT_LIBCAM;
            detected.device_path = id;
            detected.device_id = detected.device_path; /* For libcam, ID == path */

            /* Extract sensor model from camera ID */
            /* Format: /base/axi/pcie@120000/rp1/i2c@88000/imx708@1a */
            size_t sensor_pos = detected.device_path.rfind('/');
            if (sensor_pos != std::string::npos) {
                std::string sensor_part = detected.device_path.substr(sensor_pos + 1);
                size_t at_pos = sensor_part.find('@');
                if (at_pos != std::string::npos) {
                    detected.sensor_model = sensor_part.substr(0, at_pos);
                }
            }

            /* Generate friendly name */
            if (!detected.sensor_model.empty()) {
                detected.device_name = "Pi Camera (" + detected.sensor_model + ")";
            } else {
                detected.device_name = "Pi Camera";
            }

            /* Apply sensor-specific defaults */
            apply_sensor_defaults(detected);

            /* Check if already configured */
            detected.already_configured = is_device_configured(
                detected.device_id, detected.device_path);

            cameras.push_back(detected);

            MOTION_LOG(INF, TYPE_ALL, NO_ERRNO
                , "Detected libcamera: %s [%s] %dx%d@%dfps%s"
                , detected.device_name.c_str()
                , detected.sensor_model.c_str()
                , detected.default_width
                , detected.default_height
                , detected.default_fps
                , detected.already_configured ? " (configured)" : "");
        }

        cam_mgr->stop();

    } catch (const std::exception &e) {
        MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO
            , "Error detecting libcamera devices: %s", e.what());
    }

    return cameras;
}
#else
std::vector<ctx_detected_cam> cls_cam_detect::detect_libcam()
{
    return std::vector<ctx_detected_cam>();
}
#endif

#ifdef HAVE_V4L2
/* Check if device name matches blacklist */
bool cls_cam_detect::is_v4l2_blacklisted(const std::string &device_name)
{
    for (const auto &blacklisted : v4l2_blacklist) {
        if (device_name.find(blacklisted) != std::string::npos) {
            return true;
        }
    }
    return false;
}

/* Get V4L2 device friendly name */
std::string cls_cam_detect::get_v4l2_device_name(const std::string &device_path)
{
    int fd = open(device_path.c_str(), O_RDONLY);
    if (fd < 0) {
        return "";
    }

    struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
        close(fd);
        return std::string(reinterpret_cast<char*>(cap.card));
    }

    close(fd);
    return "";
}

/* Get persistent device ID from /dev/v4l/by-id/ */
std::string cls_cam_detect::get_v4l2_persistent_id(const std::string &device_path)
{
    /* Look for symlink in /dev/v4l/by-id/ pointing to this device */
    DIR *dir = opendir("/dev/v4l/by-id");
    if (!dir) {
        return device_path; /* Fallback to device_path */
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        std::string link_path = std::string("/dev/v4l/by-id/") + entry->d_name;
        char target[PATH_MAX];
        ssize_t len = readlink(link_path.c_str(), target, sizeof(target) - 1);

        if (len > 0) {
            target[len] = '\0';

            /* Resolve relative path */
            char resolved_target[PATH_MAX];
            if (realpath(link_path.c_str(), resolved_target) != NULL) {
                if (device_path == resolved_target) {
                    closedir(dir);
                    return link_path;
                }
            }
        }
    }

    closedir(dir);
    return device_path; /* No persistent ID found, use device path */
}

/* Get available resolutions for device */
std::vector<std::pair<int,int>> cls_cam_detect::get_device_resolutions(const std::string &device_path)
{
    std::vector<std::pair<int,int>> resolutions;

    int fd = open(device_path.c_str(), O_RDONLY);
    if (fd < 0) {
        return resolutions;
    }

    /* Enumerate frame sizes */
    struct v4l2_frmsizeenum frmsize;
    memset(&frmsize, 0, sizeof(frmsize));
    frmsize.pixel_format = V4L2_PIX_FMT_YUYV; /* Common format */

    int index = 0;
    while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {
        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
            resolutions.push_back(std::make_pair(
                frmsize.discrete.width, frmsize.discrete.height));
        }
        frmsize.index = ++index;
    }

    close(fd);
    return resolutions;
}

/* Detect V4L2/USB cameras */
std::vector<ctx_detected_cam> cls_cam_detect::detect_v4l2()
{
    std::vector<ctx_detected_cam> cameras;

    /* Enumerate /dev/video* devices */
    for (int i = 0; i < 10; i++) {
        std::string device_path = "/dev/video" + std::to_string(i);

        int fd = open(device_path.c_str(), O_RDONLY);
        if (fd < 0) {
            continue; /* Device doesn't exist */
        }

        struct v4l2_capability cap;
        if (ioctl(fd, VIDIOC_QUERYCAP, &cap) != 0) {
            close(fd);
            continue;
        }

        std::string device_name = std::string(reinterpret_cast<char*>(cap.card));
        close(fd);

        /* Skip blacklisted devices (ISP devices, not cameras) */
        if (is_v4l2_blacklisted(device_name)) {
            MOTION_LOG(DBG, TYPE_ALL, NO_ERRNO
                , "Skipping blacklisted device: %s (%s)"
                , device_path.c_str(), device_name.c_str());
            continue;
        }

        /* Skip non-capture devices */
        if (!(cap.device_caps & V4L2_CAP_VIDEO_CAPTURE)) {
            continue;
        }

        ctx_detected_cam detected;
        detected.type = CAM_DETECT_V4L2;
        detected.device_path = device_path;
        detected.device_id = get_v4l2_persistent_id(device_path);
        detected.device_name = device_name.empty() ? "USB Camera" : device_name;
        detected.sensor_model = ""; /* V4L2 doesn't expose sensor model */

        /* Get available resolutions */
        detected.resolutions = get_device_resolutions(device_path);

        /* Apply generic defaults */
        apply_sensor_defaults(detected);

        /* Check if already configured */
        detected.already_configured = is_device_configured(
            detected.device_id, detected.device_path);

        cameras.push_back(detected);

        MOTION_LOG(INF, TYPE_ALL, NO_ERRNO
            , "Detected V4L2: %s [%s] %dx%d@%dfps%s"
            , detected.device_name.c_str()
            , detected.device_path.c_str()
            , detected.default_width
            , detected.default_height
            , detected.default_fps
            , detected.already_configured ? " (configured)" : "");
    }

    return cameras;
}
#else
std::vector<ctx_detected_cam> cls_cam_detect::detect_v4l2()
{
    return std::vector<ctx_detected_cam>();
}
#endif

/* Detect all cameras (libcam + V4L2) */
std::vector<ctx_detected_cam> cls_cam_detect::detect_cameras()
{
    std::vector<ctx_detected_cam> all_cameras;

    MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO, "Starting camera detection...");

    /* Detect Pi cameras via libcamera */
    auto libcam_cameras = detect_libcam();
    all_cameras.insert(all_cameras.end(), libcam_cameras.begin(), libcam_cameras.end());

    /* Detect USB/V4L2 cameras */
    auto v4l2_cameras = detect_v4l2();
    all_cameras.insert(all_cameras.end(), v4l2_cameras.begin(), v4l2_cameras.end());

    MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO
        , "Camera detection complete: %d total, %d unconfigured"
        , (int)all_cameras.size()
        , (int)std::count_if(all_cameras.begin(), all_cameras.end(),
            [](const ctx_detected_cam &c) { return !c.already_configured; }));

    return all_cameras;
}

/* Test network camera connection */
bool cls_cam_detect::test_netcam(const std::string &url, const std::string &user,
                                const std::string &pass, int timeout_sec)
{
    AVFormatContext *fmt_ctx = nullptr;
    AVDictionary *opts = nullptr;
    int retcd;
    std::string full_url;

    MOTION_LOG(INF, TYPE_ALL, NO_ERRNO
        , "Testing netcam connection: %s (timeout: %ds)"
        , url.c_str(), timeout_sec);

    /* Build URL with credentials if provided */
    if (!user.empty()) {
        /* Insert user:pass into URL after the scheme:// */
        size_t pos = url.find("://");
        if (pos != std::string::npos) {
            full_url = url.substr(0, pos + 3) + user;
            if (!pass.empty()) {
                full_url += ":" + pass;
            }
            full_url += "@" + url.substr(pos + 3);
        } else {
            full_url = url;
        }
    } else {
        full_url = url;
    }

    /* Set timeout options */
    std::string timeout_us = std::to_string(timeout_sec * 1000000);
    av_dict_set(&opts, "timeout", timeout_us.c_str(), 0);
    av_dict_set(&opts, "stimeout", timeout_us.c_str(), 0);
    av_dict_set(&opts, "analyzeduration", "1000000", 0);
    av_dict_set(&opts, "probesize", "100000", 0);

    /* Detect protocol and set appropriate options */
    if (full_url.find("rtsp") == 0) {
        av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    } else if (full_url.find("http") == 0) {
        av_dict_set(&opts, "input_format", "mjpeg", 0);
    }

    fmt_ctx = avformat_alloc_context();
    if (!fmt_ctx) {
        MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO, "Failed to allocate format context");
        av_dict_free(&opts);
        return false;
    }

    retcd = avformat_open_input(&fmt_ctx, full_url.c_str(), nullptr, &opts);
    av_dict_free(&opts);

    if (retcd < 0) {
        char errstr[128];
        av_strerror(retcd, errstr, sizeof(errstr));
        MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO
            , "Netcam test failed for %s: %s"
            , url.c_str(), errstr);
        /* fmt_ctx is freed by avformat_open_input on failure */
        return false;
    }

    MOTION_LOG(INF, TYPE_ALL, NO_ERRNO
        , "Netcam test successful for %s", url.c_str());

    avformat_close_input(&fmt_ctx);
    return true;
}
