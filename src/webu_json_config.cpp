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
 * webu_json_config.cpp - Configuration Management API
 *
 * Implements JSON REST API endpoints for reading, updating, and persisting
 * Motion configuration parameters. Supports hot-reload of libcam camera
 * controls via hash-map dispatch, bcrypt hashing of auth passwords,
 * permission enforcement via webcontrol_parms, and write-to-disk.
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

/* Hot-reload parameter dispatch table
 * Maps parameter names to lambda functions that apply the change to a camera
 * This replaces the 28-branch if/else chain with O(1) hash map lookup
 */

namespace {
    using HotReloadFunc = std::function<void(cls_camera*, const std::string&)>;

    const std::unordered_map<std::string, HotReloadFunc> hot_reload_map = {
        {"libcam_brightness", [](cls_camera *cam, const std::string &val) {
            cam->set_libcam_brightness(strtof(val.c_str(), nullptr));
        }},
        {"libcam_contrast", [](cls_camera *cam, const std::string &val) {
            cam->set_libcam_contrast(strtof(val.c_str(), nullptr));
        }},
        {"libcam_sharpness", [](cls_camera *cam, const std::string &val) {
            cam->set_libcam_sharpness(strtof(val.c_str(), nullptr));
        }},
        {"libcam_saturation", [](cls_camera *cam, const std::string &val) {
            cam->set_libcam_saturation(strtof(val.c_str(), nullptr));
        }},
        {"libcam_gain", [](cls_camera *cam, const std::string &val) {
            cam->set_libcam_gain(strtof(val.c_str(), nullptr));
        }},
        {"libcam_awb_enable", [](cls_camera *cam, const std::string &val) {
            cam->set_libcam_awb_enable(val == "true" || val == "1");
        }},
        {"libcam_awb_mode", [](cls_camera *cam, const std::string &val) {
            cam->set_libcam_awb_mode(atoi(val.c_str()));
        }},
        {"libcam_awb_locked", [](cls_camera *cam, const std::string &val) {
            cam->set_libcam_awb_locked(val == "true" || val == "1");
        }},
        {"libcam_colour_temp", [](cls_camera *cam, const std::string &val) {
            cam->set_libcam_colour_temp(atoi(val.c_str()));
        }},
        {"libcam_colour_gain_r", [](cls_camera *cam, const std::string &val) {
            float r = strtof(val.c_str(), nullptr);
            float b = cam->cfg->parm_cam.libcam_colour_gain_b;
            cam->set_libcam_colour_gains(r, b);
        }},
        {"libcam_colour_gain_b", [](cls_camera *cam, const std::string &val) {
            float r = cam->cfg->parm_cam.libcam_colour_gain_r;
            float b = strtof(val.c_str(), nullptr);
            cam->set_libcam_colour_gains(r, b);
        }},
        {"libcam_af_mode", [](cls_camera *cam, const std::string &val) {
            cam->set_libcam_af_mode(atoi(val.c_str()));
        }},
        {"libcam_lens_position", [](cls_camera *cam, const std::string &val) {
            cam->set_libcam_lens_position(strtof(val.c_str(), nullptr));
        }},
        {"libcam_af_range", [](cls_camera *cam, const std::string &val) {
            cam->set_libcam_af_range(atoi(val.c_str()));
        }},
        {"libcam_af_speed", [](cls_camera *cam, const std::string &val) {
            cam->set_libcam_af_speed(atoi(val.c_str()));
        }},
        {"libcam_af_trigger", [](cls_camera *cam, const std::string &val) {
            int v = atoi(val.c_str());
            if (v == 0) {
                cam->trigger_libcam_af_scan();
            } else {
                cam->cancel_libcam_af_scan();
            }
        }},
        {"libcam_noise_reduction_mode", [](cls_camera *cam, const std::string &val) {
            cam->set_libcam_noise_reduction_mode(atoi(val.c_str()));
        }},
        {"libcam_exposure_time", [](cls_camera *cam, const std::string &val) {
            cam->set_libcam_exposure_time(atoi(val.c_str()));
        }},
    };
}

void cls_webu_json::parms_item_detail(cls_config *conf, std::string pNm)
{
    ctx_params  *params;
    ctx_params_item *itm;
    int indx;

    params = new ctx_params;
    params->params_cnt = 0;
    mylower(pNm);

    if (pNm == "v4l2_params") {
        util_parms_parse(params, pNm, conf->v4l2_params);
    } else if (pNm == "netcam_params") {
        util_parms_parse(params, pNm, conf->netcam_params);
    } else if (pNm == "netcam_high_params") {
        util_parms_parse(params, pNm, conf->netcam_high_params);
    } else if (pNm == "libcam_params") {
        util_parms_parse(params, pNm, conf->libcam_params);
    } else if (pNm == "schedule_params") {
        util_parms_parse(params, pNm, conf->schedule_params);
    } else if (pNm == "picture_schedule_params") {
        util_parms_parse(params, pNm, conf->picture_schedule_params);
    } else if (pNm == "cleandir_params") {
        util_parms_parse(params, pNm, conf->cleandir_params);
    } else if (pNm == "secondary_params") {
        util_parms_parse(params, pNm, conf->secondary_params);
    } else if (pNm == "webcontrol_actions") {
        util_parms_parse(params, pNm, conf->webcontrol_actions);
    } else if (pNm == "webcontrol_headers") {
        util_parms_parse(params, pNm, conf->webcontrol_headers);
    } else if (pNm == "stream_preview_params") {
        util_parms_parse(params, pNm, conf->stream_preview_params);
    } else if (pNm == "snd_params") {
        util_parms_parse(params, pNm, conf->snd_params);
    }

    webua->resp_page += ",\"count\":";
    webua->resp_page += std::to_string(params->params_cnt);

    if (params->params_cnt > 0) {
        webua->resp_page += ",\"parsed\" :{";
        for (indx=0; indx<params->params_cnt; indx++) {
            itm = &params->params_array[indx];
            if (indx != 0) {
                webua->resp_page += ",";
            }
            webua->resp_page += "\""+std::to_string(indx)+"\":";
            webua->resp_page += "{\"name\":\""+itm->param_name+"\",";
            webua->resp_page += "\"value\":\""+itm->param_value+"\"}";

        }
        webua->resp_page += "}";
    }

    mydelete(params);

}

void cls_webu_json::parms_item(cls_config *conf, int indx_parm)
{
    std::string parm_orig, parm_val, parm_list, parm_enable;
    std::string parm_name = config_parms[indx_parm].parm_name;
    bool password_set = false;

    parm_orig = "";
    parm_val = "";
    parm_list = "[]";  // Default to empty JSON array for valid JSON

    if (app->cfg->webcontrol_parms < PARM_LEVEL_LIMITED) {
        parm_enable = "false";
    } else {
        parm_enable = "true";
    }

    conf->edit_get(config_parms[indx_parm].parm_name
        , parm_orig, config_parms[indx_parm].parm_cat);

    /* Mask password values for authentication parameters
     * Returns username with empty password, plus password_set flag */
    if (parm_name == "webcontrol_authentication" ||
        parm_name == "webcontrol_user_authentication") {
        size_t colon_pos = parm_orig.find(':');
        if (colon_pos != std::string::npos) {
            std::string username = parm_orig.substr(0, colon_pos);
            std::string password = parm_orig.substr(colon_pos + 1);
            password_set = !password.empty();
            /* Return username with empty password portion */
            parm_val = escstr(username) + ":";
        } else {
            parm_val = "";
        }
    } else {
        parm_val = escstr(parm_orig);
    }

    if (config_parms[indx_parm].parm_type == PARM_TYP_INT) {
        webua->resp_page +=
            "\"" + config_parms[indx_parm].parm_name + "\"" +
            ":{" +
            " \"value\":" + parm_val +
            ",\"enabled\":" + parm_enable +
            ",\"category\":" + std::to_string(config_parms[indx_parm].parm_cat) +
            ",\"type\":\"" + conf->type_desc(config_parms[indx_parm].parm_type) + "\"" +
            "}";

    } else if (config_parms[indx_parm].parm_type == PARM_TYP_FLOAT) {
        webua->resp_page +=
            "\"" + config_parms[indx_parm].parm_name + "\"" +
            ":{" +
            " \"value\":" + parm_val +
            ",\"enabled\":" + parm_enable +
            ",\"category\":" + std::to_string(config_parms[indx_parm].parm_cat) +
            ",\"type\":\"" + conf->type_desc(config_parms[indx_parm].parm_type) + "\"" +
            "}";

    } else if (config_parms[indx_parm].parm_type == PARM_TYP_BOOL) {
        if (parm_val == "on") {
            webua->resp_page +=
                "\"" + config_parms[indx_parm].parm_name + "\"" +
                ":{" +
                " \"value\":true" +
                ",\"enabled\":" + parm_enable +
                ",\"category\":" + std::to_string(config_parms[indx_parm].parm_cat) +
                ",\"type\":\"" + conf->type_desc(config_parms[indx_parm].parm_type) + "\""+
                "}";
        } else {
            webua->resp_page +=
                "\"" + config_parms[indx_parm].parm_name + "\"" +
                ":{" +
                " \"value\":false" +
                ",\"enabled\":" + parm_enable +
                ",\"category\":" + std::to_string(config_parms[indx_parm].parm_cat) +
                ",\"type\":\"" + conf->type_desc(config_parms[indx_parm].parm_type) + "\"" +
                "}";
        }
    } else if (config_parms[indx_parm].parm_type == PARM_TYP_LIST) {
        conf->edit_list(config_parms[indx_parm].parm_name
            , parm_list, config_parms[indx_parm].parm_cat);
        webua->resp_page +=
            "\"" + config_parms[indx_parm].parm_name + "\"" +
            ":{" +
            " \"value\": \"" + parm_val + "\"" +
            ",\"enabled\":" + parm_enable +
            ",\"category\":" + std::to_string(config_parms[indx_parm].parm_cat) +
            ",\"type\":\"" + conf->type_desc(config_parms[indx_parm].parm_type) + "\"" +
            ",\"list\":" + parm_list +
            "}";
    } else if (config_parms[indx_parm].parm_type == PARM_TYP_PARAMS) {
        webua->resp_page +=
            "\"" + config_parms[indx_parm].parm_name + "\"" +
            ":{" +
            " \"value\":\"" + parm_val + "\"" +
            ",\"enabled\":" + parm_enable +
            ",\"category\":" + std::to_string(config_parms[indx_parm].parm_cat) +
            ",\"type\":\""+ conf->type_desc(config_parms[indx_parm].parm_type) + "\"";
        parms_item_detail(conf, config_parms[indx_parm].parm_name);
        webua->resp_page += "}";
    } else {
        webua->resp_page +=
            "\"" + parm_name + "\"" +
            ":{" +
            " \"value\":\"" + parm_val + "\"" +
            ",\"enabled\":" + parm_enable +
            ",\"category\":" + std::to_string(config_parms[indx_parm].parm_cat) +
            ",\"type\":\""+ conf->type_desc(config_parms[indx_parm].parm_type) + "\"";
        /* Add password_set flag for authentication parameters */
        if (parm_name == "webcontrol_authentication" ||
            parm_name == "webcontrol_user_authentication") {
            webua->resp_page += ",\"password_set\":" + std::string(password_set ? "true" : "false");
        }
        webua->resp_page += "}";
    }
}

void cls_webu_json::parms_one(cls_config *conf)
{
    int indx_parm;
    bool first;
    std::string response;

    indx_parm = 0;
    first = true;
    while ((config_parms[indx_parm].parm_name != "") ) {
        if (config_parms[indx_parm].webui_level == PARM_LEVEL_NEVER) {
            indx_parm++;
            continue;
        }
        if (first) {
            first = false;
            webua->resp_page += "{";
        } else {
            webua->resp_page += ",";
        }
        /* Allow limited parameters to be read only to the web page */
        if ((config_parms[indx_parm].webui_level >
                app->cfg->webcontrol_parms) &&
            (config_parms[indx_parm].webui_level > PARM_LEVEL_LIMITED)) {

            webua->resp_page +=
                "\""+config_parms[indx_parm].parm_name+"\"" +
                ":{" +
                " \"value\":\"\"" +
                ",\"enabled\":false" +
                ",\"category\":" + std::to_string(config_parms[indx_parm].parm_cat) +
                ",\"type\":\""+ conf->type_desc(config_parms[indx_parm].parm_type) + "\"";

            if (config_parms[indx_parm].parm_type == PARM_TYP_LIST) {
                webua->resp_page += ",\"list\":[\"na\"]";
            }
            webua->resp_page +="}";
        } else {
           parms_item(conf, indx_parm);
        }
        indx_parm++;
    }
    webua->resp_page += "}";
}

void cls_webu_json::parms_all()
{
    int indx_cam;

    webua->resp_page += "{";
    webua->resp_page += "\"default\": ";
    parms_one(app->cfg);

    for (indx_cam=0; indx_cam<app->cam_cnt; indx_cam++) {
        webua->resp_page += ",\"cam" +
            std::to_string(app->cam_list[indx_cam]->cfg->device_id) + "\": ";
        parms_one(app->cam_list[indx_cam]->cfg);
    }
    webua->resp_page += "}";
}

bool cls_webu_json::validate_hot_reload(const std::string &parm_name, int &parm_index)
{
    parm_index = 0;
    while (config_parms[parm_index].parm_name != "") {
        if (config_parms[parm_index].parm_name == parm_name) {
            /* Check permission level */
            if (config_parms[parm_index].webui_level > app->cfg->webcontrol_parms) {
                return false;
            }
            /* Check hot reload flag */
            return config_parms[parm_index].hot_reload;
        }
        parm_index++;
    }
    parm_index = -1;  /* Not found */
    return false;
}

void cls_webu_json::apply_hot_reload_to_camera(cls_camera *cam,
    const std::string &parm_name, const std::string &parm_val)
{
    auto it = hot_reload_map.find(parm_name);
    if (it != hot_reload_map.end()) {
        it->second(cam, parm_val);
    }
}

void cls_webu_json::apply_hot_reload(int parm_index, const std::string &parm_val)
{
    std::string parm_name = config_parms[parm_index].parm_name;

    if (webua->device_id == 0) {
        /* Update default config */
        app->cfg->edit_set(parm_name, parm_val);
        app->conf_src->edit_set(parm_name, parm_val);

        /* Update all running cameras - currently unreachable from UI but kept for
         * future "Apply to All Cameras" feature and external API clients */
        for (int indx = 0; indx < app->cam_cnt; indx++) {
            app->cam_list[indx]->cfg->edit_set(parm_name, parm_val);
            app->cam_list[indx]->conf_src->edit_set(parm_name, parm_val);
            apply_hot_reload_to_camera(app->cam_list[indx], parm_name, parm_val);
        }
    } else if (webua->cam != nullptr) {
        /* Update specific camera only */
        webua->cam->cfg->edit_set(parm_name, parm_val);
        webua->cam->conf_src->edit_set(parm_name, parm_val);
        apply_hot_reload_to_camera(webua->cam, parm_name, parm_val);
    }

    MOTION_LOG(INF, TYPE_ALL, NO_ERRNO,
        "Hot reload: %s = %s (camera %d)",
        parm_name.c_str(), parm_val.c_str(), webua->device_id);
}

void cls_webu_json::api_config()
{
    webua->resp_type = WEBUI_RESP_JSON;

    /* Add CSRF token at the start of the response */
    webua->resp_page = "{\"csrf_token\":\"" + webu->csrf_token + "\"";

    /* Add version - config() normally starts with { so we skip it */
    webua->resp_page += ",\"version\" : \"" VERSION "\"";

    /* Add cameras list */
    webua->resp_page += ",\"cameras\" : ";
    cameras_list();

    /* Add configuration parameters */
    webua->resp_page += ",\"configuration\" : ";
    parms_all();

    /* Add categories */
    webua->resp_page += ",\"categories\" : ";
    categories_list();

    webua->resp_page += "}";
}

void cls_webu_json::api_config_patch()
{
    webua->resp_type = WEBUI_RESP_JSON;

    /* Validate CSRF token - returns 403 so frontend can auto-retry with fresh token */
    if (!validate_csrf()) {
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

    /* Get config for this camera/device */
    cls_config *cfg;
    if (webua->cam != nullptr) {
        cfg = webua->cam->cfg;
    } else {
        cfg = app->cfg;
    }

    /* Start response */
    webua->resp_page = "{\"status\":\"ok\",\"applied\":[";
    bool first_item = true;
    int success_count = 0;
    int error_count = 0;

    /* Process each parameter */
    pthread_mutex_lock(&app->mutex_post);
    for (const auto& kv : parser.getAll()) {
        std::string parm_name = kv.first;
        std::string parm_val = parser.getString(parm_name);
        std::string old_val;
        int parm_index = -1;
        bool applied = false;
        bool hot_reload = false;
        bool unchanged = false;
        std::string error_msg;

        /* Auto-hash authentication passwords if not already hashed */
        if (parm_name == "webcontrol_authentication" ||
            parm_name == "webcontrol_user_authentication") {

            size_t colon_pos = parm_val.find(':');
            if (colon_pos != std::string::npos) {
                std::string username = parm_val.substr(0, colon_pos);
                std::string password = parm_val.substr(colon_pos + 1);

                /* If password is not already a bcrypt hash, hash it */
                if (!cls_webu_auth::is_bcrypt_hash(password)) {
                    std::string hashed = cls_webu_auth::hash_password(password);
                    if (!hashed.empty()) {
                        parm_val = username + ":" + hashed;
                        MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO,
                            "Auto-hashed password for %s", parm_name.c_str());
                    } else {
                        /* Hash failed - log error but allow plaintext */
                        MOTION_LOG(WRN, TYPE_ALL, NO_ERRNO,
                            "Failed to hash password for %s - saving plaintext", parm_name.c_str());
                    }
                }
            }
        }

        /* SECURITY: Reject SQL parameter modifications */
        if (parm_name.substr(0, 4) == "sql_") {
            error_msg = "SQL parameters cannot be modified via web interface (security restriction)";
            error_count++;
        }
        /* SECURITY: Allow initial authentication setup regardless of webcontrol_parms
         * This enables first-run configuration without requiring webcontrol_parms 3
         * Exception only applies when BOTH auth parameters are empty (fresh install)
         * Once any auth is configured, normal permission levels apply */
        else if ((parm_name == "webcontrol_authentication" ||
                  parm_name == "webcontrol_user_authentication") &&
                 cfg->webcontrol_authentication == "" &&
                 cfg->webcontrol_user_authentication == "") {
            /* Initial setup exception - find parameter without permission check */
            parm_index = 0;
            while (config_parms[parm_index].parm_name != "") {
                if (config_parms[parm_index].parm_name == parm_name) {
                    break;
                }
                parm_index++;
            }

            if (config_parms[parm_index].parm_name == "") {
                /* Parameter not found */
                parm_index = -1;
                error_msg = "Unknown parameter";
                error_count++;
            } else {
                /* Parameter exists - get current value */
                cfg->edit_get(parm_name, old_val, config_parms[parm_index].parm_cat);

                /* Check if value actually changed */
                if (old_val == parm_val) {
                    unchanged = true;
                    hot_reload = config_parms[parm_index].hot_reload;
                    success_count++;
                } else {
                    /* Authentication parameters require restart */
                    cfg->edit_set(parm_name, parm_val);
                    applied = true;
                    hot_reload = false;
                    success_count++;

                    MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO,
                        "Initial setup: %s configured (restart required)",
                        parm_name.c_str());
                }
            }
        }
        /* Check if parameter exists */
        else {
            validate_hot_reload(parm_name, parm_index);

            if (parm_index < 0) {
                /* Parameter doesn't exist */
                error_msg = "Unknown parameter";
                error_count++;
            } else if (config_parms[parm_index].webui_level > app->cfg->webcontrol_parms) {
                /* Permission level too low */
                error_msg = "Insufficient permissions (requires webcontrol_parms " +
                            std::to_string(config_parms[parm_index].webui_level) + ")";
                error_count++;
            } else {
                /* Parameter exists - get current value */
                cfg->edit_get(parm_name, old_val, config_parms[parm_index].parm_cat);

                /* Check if value actually changed */
                bool values_equal;
                if (config_parms[parm_index].parm_type == PARM_TYP_FLOAT) {
                    values_equal = (strtof(old_val.c_str(), nullptr) ==
                                    strtof(parm_val.c_str(), nullptr));
                } else {
                    values_equal = (old_val == parm_val);
                }
                if (values_equal) {
                    unchanged = true;
                    hot_reload = config_parms[parm_index].hot_reload;
                    success_count++;
                } else {
                    /* Check if hot-reloadable */
                    if (config_parms[parm_index].hot_reload) {
                        /* Apply immediately */
                        apply_hot_reload(parm_index, parm_val);
                        applied = true;
                        hot_reload = true;
                        success_count++;
                    } else {
                        /* Save to config - requires restart to take effect */
                        cfg->edit_set(parm_name, parm_val);

                        /* Also update source config for restart persistence */
                        if (webua->cam != nullptr) {
                            webua->cam->conf_src->edit_set(parm_name, parm_val);
                        } else {
                            app->conf_src->edit_set(parm_name, parm_val);
                        }

                        applied = true;
                        hot_reload = false;
                        success_count++;
                    }
                }
            }
        }

        /* Add this parameter to response */
        if (!first_item) {
            webua->resp_page += ",";
        }
        first_item = false;

        webua->resp_page += "{\"param\":\"" + parm_name + "\"";
        webua->resp_page += ",\"old\":\"" + escstr(old_val) + "\"";
        webua->resp_page += ",\"new\":\"" + escstr(parm_val) + "\"";

        if (unchanged) {
            webua->resp_page += ",\"unchanged\":true";
        } else if (applied) {
            webua->resp_page += ",\"hot_reload\":" + std::string(hot_reload ? "true" : "false");
        }

        if (!error_msg.empty()) {
            webua->resp_page += ",\"error\":\"" + escstr(error_msg) + "\"";
        }

        webua->resp_page += "}";
    }
    pthread_mutex_unlock(&app->mutex_post);

    webua->resp_page += "]";
    webua->resp_page += ",\"summary\":{";
    webua->resp_page += "\"total\":" + std::to_string(success_count + error_count);
    webua->resp_page += ",\"success\":" + std::to_string(success_count);
    webua->resp_page += ",\"errors\":" + std::to_string(error_count);
    webua->resp_page += "}}";
}

void cls_webu_json::api_config_write()
{
    webua->resp_type = WEBUI_RESP_JSON;

    if (!validate_csrf()) {
        return;
    }

    if (!check_action_permission("config_write")) {
        return;
    }

    MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO,
        "Config write requested by %s", webua->clientip.c_str());

    pthread_mutex_lock(&app->mutex_post);
    app->conf_src->parms_write();
    pthread_mutex_unlock(&app->mutex_post);

    webua->resp_page = "{\"status\":\"ok\"}";
}
