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
 * webu_json.cpp - JSON REST API Implementation
 *
 * This module implements the JSON REST API for configuration management,
 * camera control, status queries, and profile operations, serving as the
 * primary interface between the React frontend and Motion backend.
 *
 */

#include "motion.hpp"
#include "util.hpp"
#include "camera.hpp"
#include "conf.hpp"
#include "conf_profile.hpp"
#include "cam_detect.hpp"
#include "logger.hpp"
#include "webu.hpp"
#include "webu_ans.hpp"
#include "webu_auth.hpp"
#include "webu_json.hpp"
#include "dbse.hpp"
#include "libcam.hpp"
#include "netcam.hpp"
#include "video_v4l2.hpp"
#include "json_parse.hpp"
#include <map>
#include <algorithm>
#include <vector>
#include <thread>
#include <functional>
#include <unordered_map>
#include <sys/statvfs.h>
#include <dirent.h>
#include <set>

void cls_webu_json::api_cameras()
{
    int indx_cam;
    std::string strid;
    cls_camera *cam;

    webua->resp_page = "{\"cameras\":[";

    for (indx_cam=0; indx_cam<app->cam_cnt; indx_cam++) {
        cam = app->cam_list[indx_cam];
        strid = std::to_string(cam->cfg->device_id);

        if (indx_cam > 0) {
            webua->resp_page += ",";
        }

        webua->resp_page += "{";
        webua->resp_page += "\"id\":" + strid + ",";

        if (cam->cfg->device_name == "") {
            webua->resp_page += "\"name\":\"camera " + strid + "\",";
        } else {
            webua->resp_page += "\"name\":\"" + escstr(cam->cfg->device_name) + "\",";
        }

        webua->resp_page += "\"url\":\"" + webua->hostfull + "/" + strid + "/\"";
        webua->resp_page += "}";
    }

    webua->resp_page += "]}";
    webua->resp_type = WEBUI_RESP_JSON;
}

void cls_webu_json::api_camera_restart()
{
    webua->resp_type = WEBUI_RESP_JSON;

    if (!validate_csrf()) {
        return;
    }

    if (!check_action_permission("restart")) {
        return;
    }

    pthread_mutex_lock(&app->mutex_post);
    if (webua->device_id == 0) {
        MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO, _("Restarting all cameras"));
        for (int indx = 0; indx < app->cam_cnt; indx++) {
            app->cam_list[indx]->handler_stop = false;
            app->cam_list[indx]->restart = true;
        }
    } else {
        if (webua->camindx >= 0 && webua->camindx < app->cam_cnt) {
            MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO,
                _("Restarting camera %d"),
                app->cam_list[webua->camindx]->cfg->device_id);
            app->cam_list[webua->camindx]->handler_stop = false;
            app->cam_list[webua->camindx]->restart = true;
        } else {
            pthread_mutex_unlock(&app->mutex_post);
            webua->resp_page = "{\"error\":\"Invalid camera ID\"}";
            return;
        }
    }
    pthread_mutex_unlock(&app->mutex_post);

    webua->resp_page = "{\"status\":\"ok\"}";
}

void cls_webu_json::api_camera_snapshot()
{
    webua->resp_type = WEBUI_RESP_JSON;

    if (!validate_csrf()) {
        return;
    }

    if (!check_action_permission("snapshot")) {
        return;
    }

    pthread_mutex_lock(&app->mutex_post);
    if (webua->device_id == 0) {
        MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO, _("Snapshot requested for all cameras"));
        for (int indx = 0; indx < app->cam_cnt; indx++) {
            app->cam_list[indx]->action_snapshot = true;
        }
    } else {
        if (webua->camindx >= 0 && webua->camindx < app->cam_cnt) {
            MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO,
                _("Snapshot requested for camera %d"),
                app->cam_list[webua->camindx]->cfg->device_id);
            app->cam_list[webua->camindx]->action_snapshot = true;
        } else {
            pthread_mutex_unlock(&app->mutex_post);
            webua->resp_page = "{\"error\":\"Invalid camera ID\"}";
            return;
        }
    }
    pthread_mutex_unlock(&app->mutex_post);

    webua->resp_page = "{\"status\":\"ok\"}";
}

void cls_webu_json::api_camera_pause()
{
    webua->resp_type = WEBUI_RESP_JSON;

    if (!validate_csrf()) {
        return;
    }

    if (!check_action_permission("pause")) {
        return;
    }

    /* Parse JSON body for action */
    std::string action = "on";  /* Default to pause on */
    if (!webua->raw_body.empty()) {
        JsonParser parser;
        if (parser.parse(webua->raw_body)) {
            std::string parsed_action = parser.getString("action");
            if (!parsed_action.empty()) {
                action = parsed_action;
            }
        }
    }

    /* Validate action value */
    if (action != "on" && action != "off" && action != "schedule") {
        webua->resp_page = "{\"error\":\"Invalid action. Use 'on', 'off', or 'schedule'\"}";
        return;
    }

    pthread_mutex_lock(&app->mutex_post);
    if (webua->device_id == 0) {
        MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO,
            _("Pause %s requested for all cameras"), action.c_str());
        for (int indx = 0; indx < app->cam_cnt; indx++) {
            app->cam_list[indx]->user_pause = action;
        }
    } else {
        if (webua->camindx >= 0 && webua->camindx < app->cam_cnt) {
            MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO,
                _("Pause %s requested for camera %d"), action.c_str(),
                app->cam_list[webua->camindx]->cfg->device_id);
            app->cam_list[webua->camindx]->user_pause = action;
        } else {
            pthread_mutex_unlock(&app->mutex_post);
            webua->resp_page = "{\"error\":\"Invalid camera ID\"}";
            return;
        }
    }
    pthread_mutex_unlock(&app->mutex_post);

    webua->resp_page = "{\"status\":\"ok\",\"action\":\"" + action + "\"}";
}

void cls_webu_json::api_camera_stop()
{
    webua->resp_type = WEBUI_RESP_JSON;

    if (!validate_csrf()) {
        return;
    }

    if (!check_action_permission("stop")) {
        return;
    }

    pthread_mutex_lock(&app->mutex_post);
    if (webua->device_id == 0) {
        for (int indx = 0; indx < app->cam_cnt; indx++) {
            MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO,
                _("Stopping camera %d"),
                app->cam_list[indx]->cfg->device_id);
            app->cam_list[indx]->restart = false;
            app->cam_list[indx]->event_stop = true;
            app->cam_list[indx]->event_user = false;
            app->cam_list[indx]->handler_stop = true;
        }
    } else {
        if (webua->camindx >= 0 && webua->camindx < app->cam_cnt) {
            MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO,
                _("Stopping camera %d"),
                app->cam_list[webua->camindx]->cfg->device_id);
            app->cam_list[webua->camindx]->restart = false;
            app->cam_list[webua->camindx]->event_stop = true;
            app->cam_list[webua->camindx]->event_user = false;
            app->cam_list[webua->camindx]->handler_stop = true;
        } else {
            pthread_mutex_unlock(&app->mutex_post);
            webua->resp_page = "{\"error\":\"Invalid camera ID\"}";
            return;
        }
    }
    pthread_mutex_unlock(&app->mutex_post);

    webua->resp_page = "{\"status\":\"ok\"}";
}

void cls_webu_json::api_camera_event_start()
{
    webua->resp_type = WEBUI_RESP_JSON;

    if (!validate_csrf()) {
        return;
    }

    if (!check_action_permission("event")) {
        return;
    }

    pthread_mutex_lock(&app->mutex_post);
    if (webua->device_id == 0) {
        MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO, _("Event start triggered for all cameras"));
        for (int indx = 0; indx < app->cam_cnt; indx++) {
            app->cam_list[indx]->event_user = true;
        }
    } else {
        if (webua->camindx >= 0 && webua->camindx < app->cam_cnt) {
            MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO,
                _("Event start triggered for camera %d"),
                app->cam_list[webua->camindx]->cfg->device_id);
            app->cam_list[webua->camindx]->event_user = true;
        } else {
            pthread_mutex_unlock(&app->mutex_post);
            webua->resp_page = "{\"error\":\"Invalid camera ID\"}";
            return;
        }
    }
    pthread_mutex_unlock(&app->mutex_post);

    webua->resp_page = "{\"status\":\"ok\"}";
}

void cls_webu_json::api_camera_event_end()
{
    webua->resp_type = WEBUI_RESP_JSON;

    if (!validate_csrf()) {
        return;
    }

    if (!check_action_permission("event")) {
        return;
    }

    pthread_mutex_lock(&app->mutex_post);
    if (webua->device_id == 0) {
        MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO, _("Event end triggered for all cameras"));
        for (int indx = 0; indx < app->cam_cnt; indx++) {
            app->cam_list[indx]->event_stop = true;
        }
    } else {
        if (webua->camindx >= 0 && webua->camindx < app->cam_cnt) {
            MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO,
                _("Event end triggered for camera %d"),
                app->cam_list[webua->camindx]->cfg->device_id);
            app->cam_list[webua->camindx]->event_stop = true;
        } else {
            pthread_mutex_unlock(&app->mutex_post);
            webua->resp_page = "{\"error\":\"Invalid camera ID\"}";
            return;
        }
    }
    pthread_mutex_unlock(&app->mutex_post);

    webua->resp_page = "{\"status\":\"ok\"}";
}

void cls_webu_json::api_camera_ptz()
{
    webua->resp_type = WEBUI_RESP_JSON;

    if (!validate_csrf()) {
        return;
    }

    if (!check_action_permission("ptz")) {
        return;
    }

    /* PTZ requires a specific camera */
    if (webua->camindx < 0 || webua->camindx >= app->cam_cnt) {
        webua->resp_page = "{\"error\":\"PTZ requires a specific camera ID\"}";
        return;
    }

    /* Parse JSON body for action */
    if (webua->raw_body.empty()) {
        webua->resp_page = "{\"error\":\"Missing request body with action\"}";
        return;
    }

    JsonParser parser;
    if (!parser.parse(webua->raw_body)) {
        webua->resp_page = "{\"error\":\"Invalid JSON: " + parser.getError() + "\"}";
        return;
    }

    std::string action = parser.getString("action");
    if (action.empty()) {
        webua->resp_page = "{\"error\":\"Missing 'action' field\"}";
        return;
    }

    cls_camera *cam = app->cam_list[webua->camindx];
    std::string ptz_cmd;

    /* Map action to PTZ command */
    if (action == "pan_left" && !cam->cfg->ptz_pan_left.empty()) {
        ptz_cmd = cam->cfg->ptz_pan_left;
    } else if (action == "pan_right" && !cam->cfg->ptz_pan_right.empty()) {
        ptz_cmd = cam->cfg->ptz_pan_right;
    } else if (action == "tilt_up" && !cam->cfg->ptz_tilt_up.empty()) {
        ptz_cmd = cam->cfg->ptz_tilt_up;
    } else if (action == "tilt_down" && !cam->cfg->ptz_tilt_down.empty()) {
        ptz_cmd = cam->cfg->ptz_tilt_down;
    } else if (action == "zoom_in" && !cam->cfg->ptz_zoom_in.empty()) {
        ptz_cmd = cam->cfg->ptz_zoom_in;
    } else if (action == "zoom_out" && !cam->cfg->ptz_zoom_out.empty()) {
        ptz_cmd = cam->cfg->ptz_zoom_out;
    } else {
        webua->resp_page = "{\"error\":\"Invalid or unconfigured PTZ action: " + action + "\"}";
        return;
    }

    pthread_mutex_lock(&app->mutex_post);
    cam->frame_skip = cam->cfg->ptz_wait;
    util_exec_command(cam, ptz_cmd);
    pthread_mutex_unlock(&app->mutex_post);

    MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO,
        _("PTZ %s executed for camera %d"), action.c_str(), cam->cfg->device_id);

    webua->resp_page = "{\"status\":\"ok\",\"action\":\"" + action + "\"}";
}

void cls_webu_json::api_cameras_platform()
{
    webua->resp_type = WEBUI_RESP_JSON;

    auto platform_info = app->cam_detect->get_platform_info();

    webua->resp_page = "{";
    webua->resp_page += "\"is_raspberry_pi\":";
    webua->resp_page += platform_info.is_raspberry_pi ? "true" : "false";
    webua->resp_page += ",\"pi_model\":\"" + escstr(platform_info.pi_model) + "\"";
    webua->resp_page += ",\"has_libcamera\":";
    webua->resp_page += platform_info.has_libcamera ? "true" : "false";
    webua->resp_page += ",\"has_v4l2\":";
    webua->resp_page += platform_info.has_v4l2 ? "true" : "false";
    webua->resp_page += "}";
}

void cls_webu_json::api_cameras_detected()
{
    webua->resp_type = WEBUI_RESP_JSON;

    auto cameras = app->cam_detect->detect_cameras();

    webua->resp_page = "{\"cameras\":[";

    bool first = true;
    for (const auto &cam : cameras) {
        /* Only return unconfigured cameras */
        if (cam.already_configured) {
            continue;
        }

        if (!first) {
            webua->resp_page += ",";
        }
        first = false;

        webua->resp_page += "{";
        webua->resp_page += "\"type\":\"";
        if (cam.type == CAM_DETECT_LIBCAM) {
            webua->resp_page += "libcam";
        } else if (cam.type == CAM_DETECT_V4L2) {
            webua->resp_page += "v4l2";
        } else {
            webua->resp_page += "netcam";
        }
        webua->resp_page += "\"";
        webua->resp_page += ",\"device_id\":\"" + escstr(cam.device_id) + "\"";
        webua->resp_page += ",\"device_path\":\"" + escstr(cam.device_path) + "\"";
        webua->resp_page += ",\"device_name\":\"" + escstr(cam.device_name) + "\"";
        webua->resp_page += ",\"sensor_model\":\"" + escstr(cam.sensor_model) + "\"";
        webua->resp_page += ",\"default_width\":" + std::to_string(cam.default_width);
        webua->resp_page += ",\"default_height\":" + std::to_string(cam.default_height);
        webua->resp_page += ",\"default_fps\":" + std::to_string(cam.default_fps);

        /* Add available resolutions */
        webua->resp_page += ",\"resolutions\":[";
        for (size_t i = 0; i < cam.resolutions.size(); i++) {
            if (i > 0) {
                webua->resp_page += ",";
            }
            webua->resp_page += "[" + std::to_string(cam.resolutions[i].first);
            webua->resp_page += "," + std::to_string(cam.resolutions[i].second) + "]";
        }
        webua->resp_page += "]";

        webua->resp_page += "}";
    }

    webua->resp_page += "]}";
}

void cls_webu_json::api_cameras_add()
{
    webua->resp_type = WEBUI_RESP_JSON;

    if (!validate_csrf()) {
        return;
    }

    /* Parse JSON body */
    JsonParser parser;
    if (!parser.parse(webua->raw_body)) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("JSON parse error: %s"), parser.getError().c_str());
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Invalid JSON\"}";
        return;
    }

    /* Get camera details from JSON */
    std::string type = parser.getString("type", "");
    std::string device_id = parser.getString("device_id", "");
    std::string device_path = parser.getString("device_path", "");
    std::string device_name = parser.getString("device_name", "");
    std::string sensor_model = parser.getString("sensor_model", "");
    int width = static_cast<int>(parser.getNumber("width", 0));
    int height = static_cast<int>(parser.getNumber("height", 0));
    int fps = static_cast<int>(parser.getNumber("fps", 0));

    if (type.empty() || device_path.empty()) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Missing required fields\"}";
        return;
    }

    /* Create detected camera struct */
    ctx_detected_cam detected;
    if (type == "libcam") {
        detected.type = CAM_DETECT_LIBCAM;
    } else if (type == "v4l2") {
        detected.type = CAM_DETECT_V4L2;
    } else if (type == "netcam") {
        detected.type = CAM_DETECT_NETCAM;
    } else {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Invalid camera type\"}";
        return;
    }

    detected.device_id = device_id;
    detected.device_path = device_path;
    detected.device_name = device_name;
    detected.sensor_model = sensor_model;
    detected.default_width = width;
    detected.default_height = height;
    detected.default_fps = fps;

    /* Add camera to configuration */
    pthread_mutex_lock(&app->mutex_camlst);
    app->conf_src->camera_add_from_detection(detected);
    pthread_mutex_unlock(&app->mutex_camlst);

    MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO,
        "Camera added via API: %s [%s]", device_name.c_str(), device_path.c_str());

    webua->resp_page = "{\"status\":\"ok\",\"message\":\"Camera added successfully\"}";
}

void cls_webu_json::api_cameras_delete()
{
    int indx, maxcnt;

    webua->resp_type = WEBUI_RESP_JSON;

    if (!validate_csrf()) {
        return;
    }

    if (webua->camindx < 0 || webua->camindx >= app->cam_cnt) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Invalid camera ID\"}";
        return;
    }

    MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO,
        "Camera delete requested via API: camera %d", webua->device_id);

    pthread_mutex_lock(&app->mutex_post);
    app->cam_delete = webua->camindx;
    pthread_mutex_unlock(&app->mutex_post);

    /* Wait for main loop to complete the deletion */
    maxcnt = 100;
    indx = 0;
    while ((app->cam_delete != -1) && (indx < maxcnt)) {
        SLEEP(0, 50000000)
        indx++;
    }

    if (indx == maxcnt) {
        MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO, "Error deleting camera. Timed out shutting down");
        app->cam_delete = -1;
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Camera deletion timed out\"}";
        return;
    }

    webua->resp_page = "{\"status\":\"ok\",\"message\":\"Camera removed successfully\"}";
}

void cls_webu_json::api_cameras_test_netcam()
{
    webua->resp_type = WEBUI_RESP_JSON;

    /* Parse JSON body */
    JsonParser parser;
    if (!parser.parse(webua->raw_body)) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Invalid JSON\"}";
        return;
    }

    std::string url = parser.getString("url", "");
    std::string user = parser.getString("user", "");
    std::string pass = parser.getString("pass", "");
    int timeout = static_cast<int>(parser.getNumber("timeout", 5));

    if (url.empty()) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"URL is required\"}";
        return;
    }

    bool success = app->cam_detect->test_netcam(url, user, pass, timeout);

    if (success) {
        webua->resp_page = "{\"status\":\"ok\",\"message\":\"Connection successful\"}";
    } else {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Connection failed\"}";
    }
}

/* Serialize one ctx_stream_data as a pair of "mjpeg" and "mpegts" JSON objects.
 * Called under stream.mutex.
 */
static std::string stream_stats_one(const ctx_stream_data &sd)
{
    char buf[256];

    /* bandwidth = fps * frame_size * 8 / 1000 */
    double bw = sd.stats.fps_actual * (double)sd.stats.frame_size_bytes * 8.0 / 1000.0;

    snprintf(buf, sizeof(buf),
        "{\"clients\":%d"
        ",\"fps_actual\":%.2f"
        ",\"encode_time_ms\":%.2f"
        ",\"frame_size_bytes\":%d"
        ",\"bandwidth_kbps\":%.1f"
        ",\"frames_served\":%llu"
        ",\"frames_dropped\":%llu"
        "}",
        sd.jpg_cnct,
        sd.stats.fps_actual,
        sd.stats.encode_time_ms,
        sd.stats.frame_size_bytes,
        bw,
        (unsigned long long)sd.stats.frames_served.load(std::memory_order_relaxed),
        (unsigned long long)sd.stats.frames_dropped);

    std::string mjpeg(buf);

    /* MPEG-TS encoding is not yet instrumented; report client count only */
    snprintf(buf, sizeof(buf),
        "{\"clients\":%d"
        ",\"fps_actual\":0,\"encode_time_ms\":0,\"frame_size_bytes\":0"
        ",\"bandwidth_kbps\":0,\"frames_served\":0,\"frames_dropped\":0}",
        sd.ts_cnct);

    return "\"mjpeg\":" + mjpeg + ",\"mpegts\":" + std::string(buf);
}

/* GET /{camId}/api/stream/stats
 * Returns real-time MJPEG/MPEG-TS streaming metrics for the given camera.
 */
void cls_webu_json::api_stream_stats()
{
    webua->resp_type = WEBUI_RESP_JSON;

    if (webua->cam == NULL) {
        webua->resp_page = "{\"error\":\"camId 0 not supported; specify a camera ID\"}";
        return;
    }

    cls_camera *cam = webua->cam;
    char buf[256];

    std::string json = "{";
    snprintf(buf, sizeof(buf), "%d", cam->cfg->device_id);
    json += "\"camera_id\":";  json += buf;
    snprintf(buf, sizeof(buf), "%lld", (long long)time(NULL));
    json += ",\"timestamp\":"; json += buf;

    json += ",\"streams\":{";

    pthread_mutex_lock(&cam->stream.mutex);

    json += "\"norm\":{";   json += stream_stats_one(cam->stream.norm);      json += "}";
    json += ",\"sub\":{";   json += stream_stats_one(cam->stream.sub);       json += "}";
    json += ",\"motion\":{"; json += stream_stats_one(cam->stream.motion);   json += "}";
    json += ",\"source\":{"; json += stream_stats_one(cam->stream.source);   json += "}";
    json += ",\"secondary\":{"; json += stream_stats_one(cam->stream.secondary); json += "}";

    /* Compute totals while still under mutex */
    int total_clients =
        cam->stream.norm.jpg_cnct + cam->stream.norm.ts_cnct +
        cam->stream.sub.jpg_cnct  + cam->stream.sub.ts_cnct  +
        cam->stream.motion.jpg_cnct + cam->stream.motion.ts_cnct +
        cam->stream.source.jpg_cnct + cam->stream.source.ts_cnct +
        cam->stream.secondary.jpg_cnct + cam->stream.secondary.ts_cnct;

    double total_bw =
        (cam->stream.norm.stats.fps_actual * (double)cam->stream.norm.stats.frame_size_bytes +
         cam->stream.sub.stats.fps_actual  * (double)cam->stream.sub.stats.frame_size_bytes  +
         cam->stream.motion.stats.fps_actual * (double)cam->stream.motion.stats.frame_size_bytes +
         cam->stream.source.stats.fps_actual * (double)cam->stream.source.stats.frame_size_bytes +
         cam->stream.secondary.stats.fps_actual * (double)cam->stream.secondary.stats.frame_size_bytes
        ) * 8.0 / 1000.0;

    uint64_t total_served =
        cam->stream.norm.stats.frames_served.load(std::memory_order_relaxed) +
        cam->stream.sub.stats.frames_served.load(std::memory_order_relaxed)  +
        cam->stream.motion.stats.frames_served.load(std::memory_order_relaxed) +
        cam->stream.source.stats.frames_served.load(std::memory_order_relaxed) +
        cam->stream.secondary.stats.frames_served.load(std::memory_order_relaxed);

    uint64_t total_dropped =
        cam->stream.norm.stats.frames_dropped +
        cam->stream.sub.stats.frames_dropped  +
        cam->stream.motion.stats.frames_dropped +
        cam->stream.source.stats.frames_dropped +
        cam->stream.secondary.stats.frames_dropped;

    pthread_mutex_unlock(&cam->stream.mutex);

    json += "}";  /* close streams */

    snprintf(buf, sizeof(buf),
        ",\"totals\":{"
        "\"total_clients\":%d"
        ",\"total_bandwidth_kbps\":%.1f"
        ",\"total_frames_served\":%llu"
        ",\"total_frames_dropped\":%llu"
        "}",
        total_clients, total_bw,
        (unsigned long long)total_served, (unsigned long long)total_dropped);

    json += buf;
    json += "}";

    webua->resp_page = json;
}
