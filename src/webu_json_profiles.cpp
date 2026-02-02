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

void cls_webu_json::api_profiles_list()
{
    webua->resp_type = WEBUI_RESP_JSON;

    /* Get camera_id from query params (default to 0) */
    int camera_id = 0;
    const char* cam_id_str = MHD_lookup_connection_value(
        webua->connection, MHD_GET_ARGUMENT_KIND, "camera_id");
    if (cam_id_str != nullptr) {
        camera_id = atoi(cam_id_str);
    }

    /* Get profiles from database */
    if (!app->profiles || !app->profiles->enabled) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Profile system not available\",\"profiles\":[]}";
        return;
    }

    std::vector<ctx_profile_info> profiles = app->profiles->list_profiles(camera_id);

    /* Build JSON response */
    webua->resp_page = "{\"status\":\"ok\",\"profiles\":[";
    bool first = true;
    for (const auto &prof : profiles) {
        if (!first) {
            webua->resp_page += ",";
        }
        first = false;

        webua->resp_page += "{";
        webua->resp_page += "\"profile_id\":" + std::to_string(prof.profile_id) + ",";
        webua->resp_page += "\"camera_id\":" + std::to_string(prof.camera_id) + ",";
        webua->resp_page += "\"name\":\"" + escstr(prof.name) + "\",";
        webua->resp_page += "\"description\":\"" + escstr(prof.description) + "\",";
        webua->resp_page += "\"is_default\":" + std::string(prof.is_default ? "true" : "false") + ",";
        webua->resp_page += "\"created_at\":" + std::to_string((int64_t)prof.created_at) + ",";
        webua->resp_page += "\"updated_at\":" + std::to_string((int64_t)prof.updated_at) + ",";
        webua->resp_page += "\"param_count\":" + std::to_string(prof.param_count);
        webua->resp_page += "}";
    }
    webua->resp_page += "]}";
}

void cls_webu_json::api_profiles_get()
{
    webua->resp_type = WEBUI_RESP_JSON;

    /* Parse profile_id from URI */
    int profile_id = atoi(webua->uri_cmd3.c_str());
    if (profile_id <= 0) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Invalid profile ID\"}";
        return;
    }

    if (!app->profiles || !app->profiles->enabled) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Profile system not available\"}";
        return;
    }

    /* Get profile info */
    ctx_profile_info info;
    if (!app->profiles->get_profile_info(profile_id, info)) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Profile not found\"}";
        return;
    }

    /* Load profile parameters */
    std::map<std::string, std::string> params;
    if (app->profiles->load_profile(profile_id, params) != 0) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Failed to load profile parameters\"}";
        return;
    }

    /* Build JSON response with metadata + params */
    webua->resp_page = "{\"status\":\"ok\",";
    webua->resp_page += "\"profile_id\":" + std::to_string(info.profile_id) + ",";
    webua->resp_page += "\"camera_id\":" + std::to_string(info.camera_id) + ",";
    webua->resp_page += "\"name\":\"" + escstr(info.name) + "\",";
    webua->resp_page += "\"description\":\"" + escstr(info.description) + "\",";
    webua->resp_page += "\"is_default\":" + std::string(info.is_default ? "true" : "false") + ",";
    webua->resp_page += "\"created_at\":" + std::to_string((int64_t)info.created_at) + ",";
    webua->resp_page += "\"updated_at\":" + std::to_string((int64_t)info.updated_at) + ",";
    webua->resp_page += "\"params\":{";

    bool first = true;
    for (const auto &kv : params) {
        if (!first) {
            webua->resp_page += ",";
        }
        first = false;
        webua->resp_page += "\"" + escstr(kv.first) + "\":\"" + escstr(kv.second) + "\"";
    }
    webua->resp_page += "}}";
}

void cls_webu_json::api_profiles_create()
{
    webua->resp_type = WEBUI_RESP_JSON;

    /* Validate CSRF token (supports both session and global tokens) */
    const char* csrf_token = MHD_lookup_connection_value(
        webua->connection, MHD_HEADER_KIND, "X-CSRF-Token");
    if (!webu->csrf_validate_request(csrf_token ? std::string(csrf_token) : "", webua->session_token)) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("CSRF token validation failed for profile create from %s"), webua->clientip.c_str());
        webua->resp_page = "{\"status\":\"error\",\"message\":\"CSRF validation failed\"}";
        return;
    }

    if (!app->profiles || !app->profiles->enabled) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Profile system not available\"}";
        return;
    }

    /* Parse JSON body */
    JsonParser parser;
    if (!parser.parse(webua->raw_body)) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("JSON parse error: %s"), parser.getError().c_str());
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Invalid JSON: " +
                          parser.getError() + "\"}";
        return;
    }

    /* Extract required fields */
    std::string name = parser.getString("name");
    if (name.empty()) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Profile name is required\"}";
        return;
    }

    std::string description = parser.getString("description", "");
    int camera_id = (int)parser.getNumber("camera_id", 0);
    bool snapshot_current = parser.getBool("snapshot_current", false);

    /* Get parameters */
    std::map<std::string, std::string> params;

    if (snapshot_current) {
        /* Snapshot current configuration */
        cls_config *cfg;
        if (webua->cam != nullptr) {
            cfg = webua->cam->cfg;
        } else {
            cfg = app->cfg;
        }
        params = app->profiles->snapshot_config(cfg);
    } else {
        /* Use params from request body (TODO: parse nested params object) */
        /* For now, just create empty profile - params can be added via update */
    }

    /* Create profile */
    pthread_mutex_lock(&app->mutex_post);
    int profile_id = app->profiles->create_profile(camera_id, name, description, params);
    pthread_mutex_unlock(&app->mutex_post);

    if (profile_id < 0) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Failed to create profile\"}";
        return;
    }

    webua->resp_page = "{\"status\":\"ok\",\"profile_id\":" + std::to_string(profile_id) + "}";

    MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO,
        _("Profile created: id=%d, name='%s', camera=%d"), profile_id, name.c_str(), camera_id);
}

void cls_webu_json::api_profiles_update()
{
    webua->resp_type = WEBUI_RESP_JSON;

    /* Validate CSRF token (supports both session and global tokens) */
    const char* csrf_token = MHD_lookup_connection_value(
        webua->connection, MHD_HEADER_KIND, "X-CSRF-Token");
    if (!webu->csrf_validate_request(csrf_token ? std::string(csrf_token) : "", webua->session_token)) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("CSRF token validation failed for profile update from %s"), webua->clientip.c_str());
        webua->resp_page = "{\"status\":\"error\",\"message\":\"CSRF validation failed\"}";
        return;
    }

    if (!app->profiles || !app->profiles->enabled) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Profile system not available\"}";
        return;
    }

    /* Parse profile_id from URI */
    int profile_id = atoi(webua->uri_cmd3.c_str());
    if (profile_id <= 0) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Invalid profile ID\"}";
        return;
    }

    /* Parse JSON body */
    JsonParser parser;
    if (!parser.parse(webua->raw_body)) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("JSON parse error: %s"), parser.getError().c_str());
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Invalid JSON: " +
                          parser.getError() + "\"}";
        return;
    }

    /* Extract params (for now, simple key-value pairs) */
    std::map<std::string, std::string> params;
    for (const auto &kv : parser.getAll()) {
        params[kv.first] = parser.getString(kv.first);
    }

    /* Update profile */
    pthread_mutex_lock(&app->mutex_post);
    int retcd = app->profiles->update_profile(profile_id, params);
    pthread_mutex_unlock(&app->mutex_post);

    if (retcd < 0) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Failed to update profile\"}";
        return;
    }

    webua->resp_page = "{\"status\":\"ok\"}";

    MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO,
        _("Profile updated: id=%d"), profile_id);
}

void cls_webu_json::api_profiles_delete()
{
    webua->resp_type = WEBUI_RESP_JSON;

    /* Validate CSRF token (supports both session and global tokens) */
    const char* csrf_token = MHD_lookup_connection_value(
        webua->connection, MHD_HEADER_KIND, "X-CSRF-Token");
    if (!webu->csrf_validate_request(csrf_token ? std::string(csrf_token) : "", webua->session_token)) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("CSRF token validation failed for profile delete from %s"), webua->clientip.c_str());
        webua->resp_page = "{\"status\":\"error\",\"message\":\"CSRF validation failed\"}";
        return;
    }

    if (!app->profiles || !app->profiles->enabled) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Profile system not available\"}";
        return;
    }

    /* Parse profile_id from URI */
    int profile_id = atoi(webua->uri_cmd3.c_str());
    if (profile_id <= 0) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Invalid profile ID\"}";
        return;
    }

    /* Delete profile */
    pthread_mutex_lock(&app->mutex_post);
    int retcd = app->profiles->delete_profile(profile_id);
    pthread_mutex_unlock(&app->mutex_post);

    if (retcd < 0) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Failed to delete profile\"}";
        return;
    }

    webua->resp_page = "{\"status\":\"ok\"}";

    MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO,
        _("Profile deleted: id=%d"), profile_id);
}

void cls_webu_json::api_profiles_apply()
{
    webua->resp_type = WEBUI_RESP_JSON;

    /* Validate CSRF token (supports both session and global tokens) */
    const char* csrf_token = MHD_lookup_connection_value(
        webua->connection, MHD_HEADER_KIND, "X-CSRF-Token");
    if (!webu->csrf_validate_request(csrf_token ? std::string(csrf_token) : "", webua->session_token)) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("CSRF token validation failed for profile apply from %s"), webua->clientip.c_str());
        webua->resp_page = "{\"status\":\"error\",\"message\":\"CSRF validation failed\"}";
        return;
    }

    if (!app->profiles || !app->profiles->enabled) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Profile system not available\"}";
        return;
    }

    /* Parse profile_id from URI */
    int profile_id = atoi(webua->uri_cmd3.c_str());
    if (profile_id <= 0) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Invalid profile ID\"}";
        return;
    }

    /* Get config for this camera/device */
    cls_config *cfg;
    if (webua->cam != nullptr) {
        cfg = webua->cam->cfg;
    } else {
        cfg = app->cfg;
    }

    /* Apply profile */
    pthread_mutex_lock(&app->mutex_post);
    std::vector<std::string> needs_restart = app->profiles->apply_profile(cfg, profile_id);
    pthread_mutex_unlock(&app->mutex_post);

    /* Build response with restart requirements */
    webua->resp_page = "{\"status\":\"ok\",\"requires_restart\":[";
    bool first = true;
    for (const auto &param : needs_restart) {
        if (!first) {
            webua->resp_page += ",";
        }
        first = false;
        webua->resp_page += "\"" + escstr(param) + "\"";
    }
    webua->resp_page += "]}";

    MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO,
        _("Profile applied: id=%d, restart_required=%s"),
        profile_id, needs_restart.empty() ? "no" : "yes");
}

void cls_webu_json::api_profiles_set_default()
{
    webua->resp_type = WEBUI_RESP_JSON;

    /* Validate CSRF token (supports both session and global tokens) */
    const char* csrf_token = MHD_lookup_connection_value(
        webua->connection, MHD_HEADER_KIND, "X-CSRF-Token");
    if (!webu->csrf_validate_request(csrf_token ? std::string(csrf_token) : "", webua->session_token)) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("CSRF token validation failed for set default from %s"), webua->clientip.c_str());
        webua->resp_page = "{\"status\":\"error\",\"message\":\"CSRF validation failed\"}";
        return;
    }

    if (!app->profiles || !app->profiles->enabled) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Profile system not available\"}";
        return;
    }

    /* Parse profile_id from URI */
    int profile_id = atoi(webua->uri_cmd3.c_str());
    if (profile_id <= 0) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Invalid profile ID\"}";
        return;
    }

    /* Set as default */
    pthread_mutex_lock(&app->mutex_post);
    int retcd = app->profiles->set_default_profile(profile_id);
    pthread_mutex_unlock(&app->mutex_post);

    if (retcd < 0) {
        webua->resp_page = "{\"status\":\"error\",\"message\":\"Failed to set default profile\"}";
        return;
    }

    webua->resp_page = "{\"status\":\"ok\"}";

    MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO,
        _("Default profile set: id=%d"), profile_id);
}
