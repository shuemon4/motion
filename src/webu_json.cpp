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
 * webu_json.cpp - JSON REST API Router and Dispatcher
 *
 * Central dispatcher for the JSON REST API. Routes incoming HTTP requests
 * to specialized webu_json_*.cpp modules (auth, camera, config, legacy,
 * mask, media, profiles, system) that implement each API domain. Also
 * handles legacy GET endpoints (config.json, movies.json, status.json).
 *
 */

#include "motion.hpp"
#include "util.hpp"
#include "camera.hpp"
#include "conf.hpp"
#include "logger.hpp"
#include "webu.hpp"
#include "webu_ans.hpp"
#include "webu_auth.hpp"
#include "webu_json.hpp"
#include "libcam.hpp"
#include "json_parse.hpp"
#ifndef HAVE_IPCAM
#include "conf_profile.hpp"
#include "cam_detect.hpp"
#include "dbse.hpp"
#include "netcam.hpp"
#include "video_v4l2.hpp"
#endif
#include <map>
#include <algorithm>
#include <vector>
#include <thread>
#include <functional>
#include <unordered_map>
#include <sys/statvfs.h>
#include <dirent.h>
#include <set>

std::string cls_webu_json::escstr(std::string invar)
{
    std::string  outvar;
    size_t indx;
    for (indx = 0; indx <invar.length(); indx++) {
        if (invar[indx] == '\\' ||
            invar[indx] == '\"') {
                outvar += '\\';
            }
        outvar += invar[indx];
    }
    return outvar;
}

bool cls_webu_json::validate_csrf()
{
    const char* csrf_token = MHD_lookup_connection_value(
        webua->connection, MHD_HEADER_KIND, "X-CSRF-Token");
    if (!webu->csrf_validate_request(csrf_token ? std::string(csrf_token) : "", webua->session_token)) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("CSRF token validation failed from %s"), webua->clientip.c_str());
        webua->resp_page = "{\"error\":\"CSRF validation failed\"}";
        webua->resp_code = 403;
        return false;
    }
    return true;
}

bool cls_webu_json::check_action_permission(const std::string &action_name)
{
    for (int indx = 0; indx < webu->wb_actions->params_cnt; indx++) {
        if (webu->wb_actions->params_array[indx].param_name == action_name) {
            if (webu->wb_actions->params_array[indx].param_value == "off") {
                MOTION_LOG(INF, TYPE_ALL, NO_ERRNO,
                    "%s action disabled", action_name.c_str());
                webua->resp_page = "{\"error\":\"" + action_name + " action is disabled\"}";
                return false;
            }
            break;
        }
    }
    return true;
}

void cls_webu_json::main()
{
    pthread_mutex_lock(&app->mutex_post);
        if (webua->uri_cmd1 == "config.json") {
#ifndef HAVE_IPCAM
            config();
#else
            webua->bad_request();
            pthread_mutex_unlock(&app->mutex_post);
            return;
#endif
#ifndef HAVE_IPCAM
        } else if (webua->uri_cmd1 == "movies.json") {
            movies();
        } else if (webua->uri_cmd1 == "status.json") {
            status();
        } else if (webua->uri_cmd1 == "log") {
            loghistory();
#endif
        } else {
            webua->bad_request();
            pthread_mutex_unlock(&app->mutex_post);
            return;
        }
    pthread_mutex_unlock(&app->mutex_post);
    webua->mhd_send();
}

cls_webu_json::cls_webu_json(cls_webu_ans *p_webua)
{
    app    = p_webua->app;
    webu   = p_webua->webu;
    webua  = p_webua;
}

cls_webu_json::~cls_webu_json()
{
    app    = nullptr;
    webu   = nullptr;
    webua  = nullptr;
}