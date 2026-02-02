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

/* CPU-efficient polygon fill using scanline algorithm
 * Fills polygon interior with specified value in bitmap
 * O(height * edges) complexity, minimal memory allocation
 */

static void fill_polygon(u_char *bitmap, int width, int height,
    const std::vector<std::pair<int,int>> &polygon, u_char fill_val)
{
    if (polygon.size() < 3) return;

    /* Find vertical bounds */
    int min_y = height, max_y = 0;
    for (const auto &pt : polygon) {
        if (pt.second < min_y) min_y = pt.second;
        if (pt.second > max_y) max_y = pt.second;
    }

    /* Clamp to image bounds */
    if (min_y < 0) min_y = 0;
    if (max_y >= height) max_y = height - 1;

    /* Scanline fill */
    std::vector<int> x_intersects;
    for (int y = min_y; y <= max_y; y++) {
        x_intersects.clear();

        /* Find intersections with polygon edges */
        size_t n = polygon.size();
        for (size_t i = 0; i < n; i++) {
            int x1 = polygon[i].first;
            int y1 = polygon[i].second;
            int x2 = polygon[(i + 1) % n].first;
            int y2 = polygon[(i + 1) % n].second;

            /* Check if edge crosses this scanline */
            if ((y1 <= y && y2 > y) || (y2 <= y && y1 > y)) {
                /* Compute x intersection using integer math to avoid float */
                int x = x1 + ((y - y1) * (x2 - x1)) / (y2 - y1);
                x_intersects.push_back(x);
            }
        }

        /* Sort intersections */
        std::sort(x_intersects.begin(), x_intersects.end());

        /* Fill between pairs */
        for (size_t i = 0; i + 1 < x_intersects.size(); i += 2) {
            int xs = x_intersects[i];
            int xe = x_intersects[i + 1];

            /* Clamp to image bounds */
            if (xs < 0) xs = 0;
            if (xe >= width) xe = width - 1;

            /* Fill the span */
            for (int x = xs; x <= xe; x++) {
                bitmap[y * width + x] = fill_val;
            }
        }
    }
}

static std::string build_mask_path(cls_camera *cam, const std::string &type)
{
    std::string target = cam->cfg->target_dir;
    if (target.empty()) {
        target = "/var/lib/motion";
    }
    /* Remove trailing slash */
    if (!target.empty() && target.back() == '/') {
        target.pop_back();
    }
    return target + "/cam" + std::to_string(cam->cfg->device_id) +
           "_" + type + ".pgm";
}

void cls_webu_json::api_mask_get()
{
    webua->resp_type = WEBUI_RESP_JSON;

    if (webua->cam == nullptr) {
        webua->resp_page = "{\"error\":\"Camera not specified\"}";
        return;
    }

    std::string type = webua->uri_cmd3;
    if (type != "motion" && type != "privacy") {
        webua->resp_page = "{\"error\":\"Invalid mask type. Use 'motion' or 'privacy'\"}";
        return;
    }

    /* Get current mask path from config */
    std::string mask_path;
    if (type == "motion") {
        mask_path = webua->cam->cfg->mask_file;
    } else {
        mask_path = webua->cam->cfg->mask_privacy;
    }

    webua->resp_page = "{";
    webua->resp_page += "\"type\":\"" + type + "\"";

    if (mask_path.empty()) {
        webua->resp_page += ",\"exists\":false";
        webua->resp_page += ",\"path\":\"\"";
    } else {
        /* Check if file exists and get dimensions */
        FILE *f = myfopen(mask_path.c_str(), "rbe");
        if (f != nullptr) {
            char line[256];
            int w = 0, h = 0;

            /* Skip magic number P5 */
            if (fgets(line, sizeof(line), f)) {
                /* Skip comments */
                do {
                    if (!fgets(line, sizeof(line), f)) break;
                } while (line[0] == '#');

                /* Parse dimensions */
                sscanf(line, "%d %d", &w, &h);
            }
            myfclose(f);

            webua->resp_page += ",\"exists\":true";
            webua->resp_page += ",\"path\":\"" + escstr(mask_path) + "\"";
            webua->resp_page += ",\"width\":" + std::to_string(w);
            webua->resp_page += ",\"height\":" + std::to_string(h);
        } else {
            webua->resp_page += ",\"exists\":false";
            webua->resp_page += ",\"path\":\"" + escstr(mask_path) + "\"";
            webua->resp_page += ",\"error\":\"File not accessible\"";
        }
    }

    webua->resp_page += "}";
}

void cls_webu_json::api_mask_post()
{
    webua->resp_type = WEBUI_RESP_JSON;

    if (webua->cam == nullptr) {
        webua->resp_page = "{\"error\":\"Camera not specified\"}";
        return;
    }

    std::string type = webua->uri_cmd3;
    if (type != "motion" && type != "privacy") {
        webua->resp_page = "{\"error\":\"Invalid mask type. Use 'motion' or 'privacy'\"}";
        return;
    }

    /* Validate CSRF (supports both session and global tokens) */
    const char* csrf_token = MHD_lookup_connection_value(
        webua->connection, MHD_HEADER_KIND, "X-CSRF-Token");
    if (!webu->csrf_validate_request(csrf_token ? std::string(csrf_token) : "", webua->session_token)) {
        webua->resp_page = "{\"error\":\"CSRF validation failed\"}";
        return;
    }

    /* Parse JSON request body */
    std::string body = webua->raw_body;

    /* Extract dimensions - default to camera size */
    int img_width = webua->cam->imgs.width;
    int img_height = webua->cam->imgs.height;
    bool invert = false;

    /* Parse width/height from body if present */
    size_t pos = body.find("\"width\":");
    if (pos != std::string::npos) {
        img_width = atoi(body.c_str() + pos + 8);
    }
    pos = body.find("\"height\":");
    if (pos != std::string::npos) {
        img_height = atoi(body.c_str() + pos + 9);
    }
    pos = body.find("\"invert\":");
    if (pos != std::string::npos) {
        invert = (body.substr(pos + 9, 4) == "true");
    }

    /* Validate dimensions match camera */
    if (img_width != webua->cam->imgs.width || img_height != webua->cam->imgs.height) {
        MOTION_LOG(WRN, TYPE_ALL, NO_ERRNO,
            "Mask dimensions %dx%d differ from camera %dx%d, will be resized on load",
            img_width, img_height, webua->cam->imgs.width, webua->cam->imgs.height);
    }

    /* Allocate bitmap */
    u_char default_val = invert ? 255 : 0;  /* 255=detect, 0=mask */
    u_char fill_val = invert ? 0 : 255;
    std::vector<u_char> bitmap(img_width * img_height, default_val);

    /* Parse polygons array */
    /* Format: "polygons":[[[x,y],[x,y],...],[[x,y],...]] */
    pos = body.find("\"polygons\":");
    if (pos != std::string::npos) {
        size_t start = body.find('[', pos);
        if (start != std::string::npos) {
            start++; /* Skip outer [ */

            while (start < body.length() && body[start] != ']') {
                /* Skip whitespace */
                while (start < body.length() &&
                       (body[start] == ' ' || body[start] == '\n' || body[start] == ',')) {
                    start++;
                }

                if (body[start] == '[') {
                    /* Parse one polygon */
                    std::vector<std::pair<int,int>> polygon;
                    start++; /* Skip [ */

                    while (start < body.length() && body[start] != ']') {
                        /* Skip to { or [ */
                        while (start < body.length() &&
                               body[start] != '{' && body[start] != '[' && body[start] != ']') {
                            start++;
                        }
                        if (body[start] == ']') break;

                        /* Parse point {x:N, y:N} or [x,y] */
                        int x = 0, y = 0;
                        if (body[start] == '{') {
                            /* Object format */
                            size_t xpos = body.find("\"x\":", start);
                            size_t ypos = body.find("\"y\":", start);
                            if (xpos != std::string::npos && ypos != std::string::npos) {
                                x = atoi(body.c_str() + xpos + 4);
                                y = atoi(body.c_str() + ypos + 4);
                            }
                            start = body.find('}', start) + 1;
                        } else if (body[start] == '[') {
                            /* Array format [x,y] */
                            start++;
                            x = atoi(body.c_str() + start);
                            size_t comma = body.find(',', start);
                            if (comma != std::string::npos) {
                                y = atoi(body.c_str() + comma + 1);
                            }
                            start = body.find(']', start) + 1;
                        }

                        polygon.push_back({x, y});
                    }
                    start++; /* Skip ] */

                    /* Fill polygon */
                    if (polygon.size() >= 3) {
                        fill_polygon(bitmap.data(), img_width, img_height, polygon, fill_val);
                    }
                } else {
                    break;
                }
            }
        }
    }

    /* Generate mask path */
    std::string mask_path = build_mask_path(webua->cam, type);

    /* Write PGM file */
    FILE *f = myfopen(mask_path.c_str(), "wbe");
    if (f == nullptr) {
        MOTION_LOG(ERR, TYPE_ALL, SHOW_ERRNO,
            "Cannot write mask file: %s", mask_path.c_str());
        webua->resp_page = "{\"error\":\"Cannot write mask file\"}";
        return;
    }

    /* Write PGM P5 header */
    fprintf(f, "P5\n");
    fprintf(f, "# Motion mask - type: %s\n", type.c_str());
    fprintf(f, "%d %d\n", img_width, img_height);
    fprintf(f, "255\n");

    /* Write bitmap data */
    if (fwrite(bitmap.data(), 1, bitmap.size(), f) != bitmap.size()) {
        MOTION_LOG(ERR, TYPE_ALL, SHOW_ERRNO,
            "Failed writing mask data to: %s", mask_path.c_str());
        myfclose(f);
        webua->resp_page = "{\"error\":\"Failed writing mask data\"}";
        return;
    }

    myfclose(f);

    /* Update config parameter */
    pthread_mutex_lock(&app->mutex_post);
    if (type == "motion") {
        webua->cam->cfg->mask_file = mask_path;
        app->cfg->edit_set("mask_file", mask_path);
    } else {
        webua->cam->cfg->mask_privacy = mask_path;
        app->cfg->edit_set("mask_privacy", mask_path);
    }
    pthread_mutex_unlock(&app->mutex_post);

    MOTION_LOG(INF, TYPE_ALL, NO_ERRNO,
        "Mask saved: %s (type=%s, %dx%d, polygons parsed)",
        mask_path.c_str(), type.c_str(), img_width, img_height);

    webua->resp_page = "{";
    webua->resp_page += "\"success\":true";
    webua->resp_page += ",\"path\":\"" + escstr(mask_path) + "\"";
    webua->resp_page += ",\"width\":" + std::to_string(img_width);
    webua->resp_page += ",\"height\":" + std::to_string(img_height);
    webua->resp_page += ",\"message\":\"Mask saved. Reload camera to apply.\"";
    webua->resp_page += "}";
}

void cls_webu_json::api_mask_delete()
{
    webua->resp_type = WEBUI_RESP_JSON;

    if (webua->cam == nullptr) {
        webua->resp_page = "{\"error\":\"Camera not specified\"}";
        return;
    }

    std::string type = webua->uri_cmd3;
    if (type != "motion" && type != "privacy") {
        webua->resp_page = "{\"error\":\"Invalid mask type. Use 'motion' or 'privacy'\"}";
        return;
    }

    /* Validate CSRF (supports both session and global tokens) */
    const char* csrf_token = MHD_lookup_connection_value(
        webua->connection, MHD_HEADER_KIND, "X-CSRF-Token");
    if (!webu->csrf_validate_request(csrf_token ? std::string(csrf_token) : "", webua->session_token)) {
        webua->resp_page = "{\"error\":\"CSRF validation failed\"}";
        return;
    }

    /* Get current mask path */
    std::string mask_path;
    if (type == "motion") {
        mask_path = webua->cam->cfg->mask_file;
    } else {
        mask_path = webua->cam->cfg->mask_privacy;
    }

    bool file_deleted = false;
    if (!mask_path.empty()) {
        /* Security: Validate path doesn't contain traversal */
        if (mask_path.find("..") != std::string::npos) {
            MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
                "Path traversal attempt blocked: %s", mask_path.c_str());
            webua->resp_page = "{\"error\":\"Invalid path\"}";
            return;
        }

        /* Delete file */
        if (remove(mask_path.c_str()) == 0) {
            file_deleted = true;
            MOTION_LOG(INF, TYPE_ALL, NO_ERRNO,
                "Deleted mask file: %s", mask_path.c_str());
        } else if (errno != ENOENT) {
            MOTION_LOG(WRN, TYPE_ALL, SHOW_ERRNO,
                "Failed to delete mask file: %s", mask_path.c_str());
        }
    }

    /* Clear config parameter */
    pthread_mutex_lock(&app->mutex_post);
    if (type == "motion") {
        webua->cam->cfg->mask_file = "";
        app->cfg->edit_set("mask_file", "");
    } else {
        webua->cam->cfg->mask_privacy = "";
        app->cfg->edit_set("mask_privacy", "");
    }
    pthread_mutex_unlock(&app->mutex_post);

    webua->resp_page = "{";
    webua->resp_page += "\"success\":true";
    webua->resp_page += ",\"deleted\":" + std::string(file_deleted ? "true" : "false");
    webua->resp_page += ",\"message\":\"Mask removed. Reload camera to apply.\"";
    webua->resp_page += "}";
}
