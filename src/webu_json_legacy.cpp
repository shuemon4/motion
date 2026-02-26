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

void cls_webu_json::cameras_list()
{
    int indx_cam;
    std::string response;
    std::string strid;
    cls_camera     *cam;

    webua->resp_page += "{\"count\" : " + std::to_string(app->cam_cnt);

    for (indx_cam=0; indx_cam<app->cam_cnt; indx_cam++) {
        cam = app->cam_list[indx_cam];
        strid =std::to_string(cam->cfg->device_id);
        webua->resp_page += ",\"" + std::to_string(indx_cam) + "\":";
        if (cam->cfg->device_name == "") {
            webua->resp_page += "{\"name\": \"camera " + strid + "\"";
        } else {
            webua->resp_page += "{\"name\": \"" + escstr(cam->cfg->device_name) + "\"";
        }
        webua->resp_page += ",\"id\": " + strid;
        webua->resp_page += ",\"all_xpct_st\": " + std::to_string(cam->all_loc.xpct_st);
        webua->resp_page += ",\"all_xpct_en\": " + std::to_string(cam->all_loc.xpct_en);
        webua->resp_page += ",\"all_ypct_st\": " + std::to_string(cam->all_loc.ypct_st);
        webua->resp_page += ",\"all_ypct_en\": " + std::to_string(cam->all_loc.ypct_en);
        webua->resp_page += ",\"url\": \"" + webua->hostfull + "/" + strid + "/\"} ";
    }
    webua->resp_page += "}";

}

void cls_webu_json::categories_list()
{
    int indx_cat;
    std::string catnm_short, catnm_long;

    webua->resp_page += "{";

    indx_cat = 0;
    while (indx_cat != PARM_CAT_MAX) {
        if (indx_cat != 0) {
            webua->resp_page += ",";
        }
        webua->resp_page += "\"" + std::to_string(indx_cat) + "\": ";

        catnm_long = webua->app->cfg->cat_desc((enum PARM_CAT)indx_cat, false);
        catnm_short = webua->app->cfg->cat_desc((enum PARM_CAT)indx_cat, true);

        webua->resp_page += "{\"name\":\"" + catnm_short + "\",\"display\":\"" + catnm_long + "\"}";

        indx_cat++;
    }

    webua->resp_page += "}";
}

void cls_webu_json::config()
{
    webua->resp_type = WEBUI_RESP_JSON;

    webua->resp_page += "{\"version\" : \"" VERSION "\"";

    webua->resp_page += ",\"cameras\" : ";
    cameras_list();

    webua->resp_page += ",\"configuration\" : ";
    parms_all();

    webua->resp_page += ",\"categories\" : ";
    categories_list();

    webua->resp_page += "}";
}

void cls_webu_json::movies_list()
{
    int indx, indx2;
    std::string response;
    char fmt[PATH_MAX];
    vec_files flst;
    std::string sql;

    for (indx=0;indx<webu->wb_actions->params_cnt;indx++) {
        if (webu->wb_actions->params_array[indx].param_name == "movies") {
            if (webu->wb_actions->params_array[indx].param_value == "off") {
                MOTION_LOG(INF, TYPE_ALL, NO_ERRNO, "Movies via webcontrol disabled");
                webua->resp_page += "{\"count\" : 0} ";
                webua->resp_page += ",\"device_id\" : ";
                webua->resp_page += std::to_string(webua->cam->cfg->device_id);
                webua->resp_page += "}";
                return;
            } else {
                break;
            }
        }
    }

    sql  = " select * from motion ";
    sql += " where device_id = " + std::to_string(webua->cam->cfg->device_id);
    sql += " order by file_dtl, file_tml;";
    app->dbse->filelist_get(sql, flst);

    webua->resp_page += "{";
    indx = 0;
    for (indx2=0;indx2<flst.size();indx2++){
        if (flst[indx2].found == true) {
            if ((flst[indx2].file_sz/1000) < 1000) {
                snprintf(fmt,PATH_MAX,"%.1fKB"
                    ,((double)flst[indx2].file_sz/1000));
            } else if ((flst[indx2].file_sz/1000000) < 1000) {
                snprintf(fmt,PATH_MAX,"%.1fMB"
                    ,((double)flst[indx2].file_sz/1000000));
            } else {
                snprintf(fmt,PATH_MAX,"%.1fGB"
                    ,((double)flst[indx2].file_sz/1000000000));
            }
            webua->resp_page += "\""+ std::to_string(indx) + "\":";

            webua->resp_page += "{\"name\": \"";
            webua->resp_page += escstr(flst[indx2].file_nm) + "\"";

            webua->resp_page += ",\"size\": \"";
            webua->resp_page += std::string(fmt) + "\"";

            webua->resp_page += ",\"date\": \"";
            webua->resp_page += std::to_string(flst[indx2].file_dtl) + "\"";

            webua->resp_page += ",\"time\": \"";
            webua->resp_page += flst[indx2].file_tmc + "\"";

            webua->resp_page += ",\"diff_avg\": \"";
            webua->resp_page += std::to_string(flst[indx2].diff_avg) + "\"";

            webua->resp_page += ",\"sdev_min\": \"";
            webua->resp_page += std::to_string(flst[indx2].sdev_min) + "\"";

            webua->resp_page += ",\"sdev_max\": \"";
            webua->resp_page += std::to_string(flst[indx2].sdev_max) + "\"";

            webua->resp_page += ",\"sdev_avg\": \"";
            webua->resp_page += std::to_string(flst[indx2].sdev_avg) + "\"";

            webua->resp_page += "}";
            webua->resp_page += ",";
            indx++;
        }
    }
    webua->resp_page += "\"count\" : " + std::to_string(indx);
    webua->resp_page += ",\"device_id\" : ";
    webua->resp_page += std::to_string(webua->cam->cfg->device_id);
    webua->resp_page += "}";
}
void cls_webu_json::movies()
{
    int indx_cam, indx_req;

    webua->resp_type = WEBUI_RESP_JSON;

    webua->resp_page += "{\"movies\" : ";
    if (webua->cam == NULL) {
        webua->resp_page += "{\"count\" :" + std::to_string(app->cam_cnt);

        for (indx_cam=0; indx_cam<app->cam_cnt; indx_cam++) {
            webua->cam = app->cam_list[indx_cam];
            webua->resp_page += ",\""+ std::to_string(indx_cam) + "\":";
            movies_list();
        }
        webua->resp_page += "}";
        webua->cam = NULL;
    } else {
        indx_req = -1;
        for (indx_cam=0; indx_cam<app->cam_cnt; indx_cam++) {
            if (webua->cam->cfg->device_id == app->cam_list[indx_cam]->cfg->device_id){
                indx_req = indx_cam;
            }
        }
        webua->resp_page += "{\"count\" : 1";
        webua->resp_page += ",\""+ std::to_string(indx_req) + "\":";
        movies_list();
        webua->resp_page += "}";
    }
    webua->resp_page += "}";
}

void cls_webu_json::status_vars(int indx_cam)
{
    char buf[32];
    struct tm timestamp_tm;
    struct timespec curr_ts;
    cls_camera *cam;

    cam = app->cam_list[indx_cam];

    webua->resp_page += "{";

    webua->resp_page += "\"name\":\"" + escstr(cam->cfg->device_name)+"\"";
    webua->resp_page += ",\"id\":" + std::to_string(cam->cfg->device_id);
    webua->resp_page += ",\"width\":" + std::to_string(cam->imgs.width);
    webua->resp_page += ",\"height\":" + std::to_string(cam->imgs.height);
    webua->resp_page += ",\"fps\":" + std::to_string(cam->lastrate);

    clock_gettime(CLOCK_REALTIME, &curr_ts);
    localtime_r(&curr_ts.tv_sec, &timestamp_tm);
    strftime(buf, sizeof(buf), "%FT%T", &timestamp_tm);
    webua->resp_page += ",\"current_time\":\"" + std::string(buf)+"\"";

    webua->resp_page += ",\"missing_frame_counter\":" +
        std::to_string(cam->missing_frame_counter);

    if (cam->lost_connection) {
        webua->resp_page += ",\"lost_connection\":true";
    } else {
        webua->resp_page += ",\"lost_connection\":false";
    }

    if (cam->connectionlosttime.tv_sec != 0) {
        localtime_r(&cam->connectionlosttime.tv_sec, &timestamp_tm);
        strftime(buf, sizeof(buf), "%FT%T", &timestamp_tm);
        webua->resp_page += ",\"connection_lost_time\":\"" + std::string(buf)+"\"";
    } else {
        webua->resp_page += ",\"connection_lost_time\":\"\"" ;
    }
    if (cam->detecting_motion) {
        webua->resp_page += ",\"detecting\":true";
    } else {
        webua->resp_page += ",\"detecting\":false";
    }

    if (cam->pause) {
        webua->resp_page += ",\"pause\":true";
    } else {
        webua->resp_page += ",\"pause\":false";
    }

    webua->resp_page += ",\"user_pause\":\"" + cam->user_pause +"\"";

    if (cam->hw_encoder_fallback) {
        webua->resp_page += ",\"hw_encoder_fallback\":true";
    } else {
        webua->resp_page += ",\"hw_encoder_fallback\":false";
    }

    /* Add supportedControls for libcamera capability discovery */
    #ifdef HAVE_LIBCAM
    if (cam->has_libcam()) {
        webua->resp_page += ",\"supportedControls\":{";
        std::map<std::string, bool> caps = cam->get_libcam_capabilities();
        bool first = true;
        for (const auto& [name, supported] : caps) {
            if (!first) {
                webua->resp_page += ",";
            }
            webua->resp_page += "\"" + name + "\":" +
                               (supported ? "true" : "false");
            first = false;
        }
        webua->resp_page += "}";
    }
    #endif

    /* Add camera_type field */
    std::string type_str;
    switch (cam->camera_type) {
        case CAMERA_TYPE_LIBCAM: type_str = "libcam"; break;
        case CAMERA_TYPE_V4L2:   type_str = "v4l2"; break;
        case CAMERA_TYPE_NETCAM: type_str = "netcam"; break;
        default:                 type_str = "unknown"; break;
    }
    webua->resp_page += ",\"camera_type\":\"" + type_str + "\"";

    /* Add camera_device identifier */
    std::string device_str = "";
    if (cam->camera_type == CAMERA_TYPE_V4L2) {
        device_str = cam->cfg->v4l2_device;
    } else if (cam->camera_type == CAMERA_TYPE_NETCAM) {
        device_str = cam->cfg->netcam_url;
    } else if (cam->camera_type == CAMERA_TYPE_LIBCAM) {
        device_str = cam->cfg->libcam_device;
    }
    webua->resp_page += ",\"camera_device\":\"" + escstr(device_str) + "\"";

    /* Add V4L2 controls array (if V4L2 camera) */
    #ifdef HAVE_V4L2
    if (cam->camera_type == CAMERA_TYPE_V4L2 && cam->has_v4l2()) {
        vec_v4l2ctrl controls = cam->get_v4l2_controls();
        webua->resp_page += ",\"v4l2_controls\":[";
        bool first_ctrl = true;
        for (const auto& ctrl : controls) {
            if (ctrl.ctrl_menuitem) continue;  // Skip menu items
            if (!first_ctrl) webua->resp_page += ",";
            webua->resp_page += "{";
            webua->resp_page += "\"name\":\"" + escstr(ctrl.ctrl_name) + "\"";
            webua->resp_page += ",\"id\":\"" + escstr(ctrl.ctrl_iddesc) + "\"";
            // Map V4L2 control type to string
            std::string ctrl_type_str;
            if (ctrl.ctrl_type == V4L2_CTRL_TYPE_BOOLEAN) {
                ctrl_type_str = "boolean";
            } else if (ctrl.ctrl_type == V4L2_CTRL_TYPE_MENU) {
                ctrl_type_str = "menu";
            } else {
                ctrl_type_str = "integer";
            }
            webua->resp_page += ",\"type\":\"" + ctrl_type_str + "\"";
            webua->resp_page += ",\"min\":" + std::to_string(ctrl.ctrl_minimum);
            webua->resp_page += ",\"max\":" + std::to_string(ctrl.ctrl_maximum);
            webua->resp_page += ",\"default\":" + std::to_string(ctrl.ctrl_default);
            webua->resp_page += ",\"current\":" + std::to_string(ctrl.ctrl_currval);
            webua->resp_page += "}";
            first_ctrl = false;
        }
        webua->resp_page += "]";
    }
    #endif

    /* Add NETCAM status and high stream indicator (if NETCAM) */
    if (cam->camera_type == CAMERA_TYPE_NETCAM && cam->has_netcam()) {
        std::string netcam_status_str;
        switch (cam->netcam->status) {
            case NETCAM_CONNECTED:      netcam_status_str = "connected"; break;
            case NETCAM_READINGIMAGE:   netcam_status_str = "reading"; break;
            case NETCAM_NOTCONNECTED:   netcam_status_str = "not_connected"; break;
            case NETCAM_RECONNECTING:   netcam_status_str = "reconnecting"; break;
            default:                    netcam_status_str = "unknown"; break;
        }
        webua->resp_page += ",\"netcam_status\":\"" + netcam_status_str + "\"";

        // Check if high resolution stream is configured
        if (cam->has_netcam_high()) {
            webua->resp_page += ",\"has_high_stream\":true";
        } else {
            webua->resp_page += ",\"has_high_stream\":false";
        }
    }

    webua->resp_page += "}";
}

void cls_webu_json::status()
{
    int indx_cam;

    webua->resp_type = WEBUI_RESP_JSON;

    webua->resp_page += "{\"version\" : \"" VERSION "\"";
    webua->resp_page += ",\"status\" : ";

    webua->resp_page += "{\"count\" : " + std::to_string(app->cam_cnt);
        for (indx_cam=0; indx_cam<app->cam_cnt; indx_cam++) {
            webua->resp_page += ",\"cam" +
                std::to_string(app->cam_list[indx_cam]->cfg->device_id) + "\": ";
            status_vars(indx_cam);
        }
    webua->resp_page += "}";

    webua->resp_page += "}";
}

void cls_webu_json::loghistory()
{
    int indx, cnt;
    bool frst;

    webua->resp_type = WEBUI_RESP_JSON;
    webua->resp_page = "";

    frst = true;
    cnt = 0;

    pthread_mutex_lock(&motlog->mutex_log);
        for (indx=0; indx<motlog->log_vec.size();indx++) {
            if (motlog->log_vec[indx].log_nbr > mtoi(webua->uri_cmd2)) {
                if (frst == true) {
                    webua->resp_page += "{";
                    frst = false;
                } else {
                    webua->resp_page += ",";
                }
                webua->resp_page += "\"" + std::to_string(indx) +"\" : {";
                webua->resp_page += "\"lognbr\" :\"" +
                    std::to_string(motlog->log_vec[indx].log_nbr) + "\", ";
                webua->resp_page += "\"logmsg\" :\"" +
                    escstr(motlog->log_vec[indx].log_msg.substr(0,
                        motlog->log_vec[indx].log_msg.length()-1)) + "\" ";
                webua->resp_page += "}";
                cnt++;
            }
        }
    pthread_mutex_unlock(&motlog->mutex_log);
    if (frst == true) {
        webua->resp_page += "{\"0\":\"\" ";
    }
    webua->resp_page += ",\"count\":\""+std::to_string(cnt)+"\"}";

}
