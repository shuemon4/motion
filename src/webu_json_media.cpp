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
 * webu_json_media.cpp - Media Browsing and Management API
 *
 * Implements JSON REST API endpoints for browsing and managing recorded
 * media: paginated picture and movie listings from the database, date
 * grouping for filters, filesystem folder navigation with statistics,
 * and file/folder deletion with realpath-based path traversal protection.
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

void cls_webu_json::api_media_pictures()
{
    vec_files flst, flst_count;
    std::string sql, where_clause;
    int offset = 0, limit = 100;
    int64_t total_count = 0;
    const char* date_filter = nullptr;

    if (webua->cam == nullptr) {
        webua->bad_request();
        return;
    }

    /* Parse query parameters */
    const char* offset_str = MHD_lookup_connection_value(
        webua->connection, MHD_GET_ARGUMENT_KIND, "offset");
    const char* limit_str = MHD_lookup_connection_value(
        webua->connection, MHD_GET_ARGUMENT_KIND, "limit");
    date_filter = MHD_lookup_connection_value(
        webua->connection, MHD_GET_ARGUMENT_KIND, "date");

    if (offset_str) offset = std::max(0, atoi(offset_str));
    if (limit_str) limit = std::min(std::max(1, atoi(limit_str)), 100); // Cap at 100

    /* Build WHERE clause */
    where_clause  = " where device_id = " + std::to_string(webua->cam->cfg->device_id);
    where_clause += " and file_typ = 'pic'";
    if (date_filter && strlen(date_filter) == 8) {
        where_clause += " and file_dtl = " + std::string(date_filter);
    }

    /* Get total count - query just record_id for efficiency */
    sql = " select record_id from motion " + where_clause + ";";
    app->dbse->filelist_get(sql, flst_count);
    total_count = flst_count.size();

    /* Get paginated results */
    sql  = " select * from motion ";
    sql += where_clause;
    sql += " order by file_dtl desc, file_tml desc";
    sql += " limit " + std::to_string(limit);
    sql += " offset " + std::to_string(offset) + ";";

    app->dbse->filelist_get(sql, flst);

    /* Build JSON response with pagination metadata */
    webua->resp_page = "{";
    webua->resp_page += "\"total_count\":" + std::to_string(total_count) + ",";
    webua->resp_page += "\"offset\":" + std::to_string(offset) + ",";
    webua->resp_page += "\"limit\":" + std::to_string(limit) + ",";
    webua->resp_page += "\"date_filter\":";
    if (date_filter) {
        webua->resp_page += "\"" + std::string(date_filter) + "\"";
    } else {
        webua->resp_page += "null";
    }
    webua->resp_page += ",\"pictures\":[";

    for (size_t i = 0; i < flst.size(); i++) {
        if (i > 0) webua->resp_page += ",";
        webua->resp_page += "{";
        webua->resp_page += "\"id\":" + std::to_string(flst[i].record_id) + ",";
        webua->resp_page += "\"filename\":\"" + escstr(flst[i].file_nm) + "\",";
        /* Return URL path for browser access, not filesystem path */
        webua->resp_page += "\"path\":\"/" + std::to_string(webua->cam->cfg->device_id) +
            "/media/" + std::to_string(flst[i].record_id) + "/" + escstr(flst[i].file_nm) + "\",";
        webua->resp_page += "\"date\":\"" + std::to_string(flst[i].file_dtl) + "\",";
        webua->resp_page += "\"time\":\"" + escstr(flst[i].file_tml) + "\",";
        webua->resp_page += "\"size\":" + std::to_string(flst[i].file_sz);
        webua->resp_page += "}";
    }
    webua->resp_page += "]}";
    webua->resp_type = WEBUI_RESP_JSON;
}

void cls_webu_json::api_delete_picture()
{
    int indx;
    std::string sql, full_path;
    vec_files flst;

    if (webua->cam == nullptr) {
        webua->resp_code = 400;
        webua->resp_page = "{\"error\":\"Camera not specified\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    /* Check if delete action is enabled */
    for (indx=0; indx<webu->wb_actions->params_cnt; indx++) {
        if (webu->wb_actions->params_array[indx].param_name == "delete") {
            if (webu->wb_actions->params_array[indx].param_value == "off") {
                MOTION_LOG(INF, TYPE_ALL, NO_ERRNO, "Delete action disabled");
                webua->resp_code = 403;
                webua->resp_page = "{\"error\":\"Delete action is disabled\"}";
                webua->resp_type = WEBUI_RESP_JSON;
                return;
            }
            break;
        }
    }

    /* Get file ID from URI: uri_cmd4 contains the record ID */
    if (webua->uri_cmd4.empty()) {
        webua->resp_code = 400;
        webua->resp_page = "{\"error\":\"File ID required\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    int file_id = mtoi(webua->uri_cmd4);
    if (file_id <= 0) {
        webua->resp_code = 400;
        webua->resp_page = "{\"error\":\"Invalid file ID\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    /* Look up the file in database */
    sql  = " select * from motion ";
    sql += " where record_id = " + std::to_string(file_id);
    sql += " and device_id = " + std::to_string(webua->cam->cfg->device_id);
    sql += " and file_typ = 'pic'";
    app->dbse->filelist_get(sql, flst);

    if (flst.empty()) {
        webua->resp_code = 404;
        webua->resp_page = "{\"error\":\"File not found\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    /* Security: Validate file path to prevent directory traversal */
    full_path = flst[0].full_nm;
    if (full_path.find("..") != std::string::npos) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("Path traversal attempt blocked: %s from %s"),
            full_path.c_str(), webua->clientip.c_str());
        webua->resp_code = 400;
        webua->resp_page = "{\"error\":\"Invalid file path\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    /* Delete the file from filesystem */
    errno = 0;
    if (remove(full_path.c_str()) != 0 && errno != ENOENT) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("Failed to delete file: %s"), full_path.c_str());
        webua->resp_code = 500;
        webua->resp_page = "{\"error\":\"Failed to delete file\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    /* Delete from database */
    sql  = "delete from motion where record_id = " + std::to_string(file_id);
    if (app->dbse->exec_sql(sql) == false) {
        MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO,
            "Database delete failed for picture id=%d", file_id);
        webua->resp_code = 500;
        webua->resp_page = "{\"error\":\"Database delete failed\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    MOTION_LOG(INF, TYPE_ALL, NO_ERRNO,
        "Deleted picture: %s (id=%d) by %s",
        flst[0].file_nm.c_str(), file_id, webua->clientip.c_str());

    webua->resp_page = "{\"success\":true,\"deleted_id\":" + std::to_string(file_id) + "}";
    webua->resp_type = WEBUI_RESP_JSON;
}

void cls_webu_json::api_delete_movie()
{
    int indx;
    std::string sql, full_path;
    vec_files flst;

    if (webua->cam == nullptr) {
        webua->resp_code = 400;
        webua->resp_page = "{\"error\":\"Camera not specified\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    /* Check if delete action is enabled */
    for (indx=0; indx<webu->wb_actions->params_cnt; indx++) {
        if (webu->wb_actions->params_array[indx].param_name == "delete") {
            if (webu->wb_actions->params_array[indx].param_value == "off") {
                MOTION_LOG(INF, TYPE_ALL, NO_ERRNO, "Delete action disabled");
                webua->resp_code = 403;
                webua->resp_page = "{\"error\":\"Delete action is disabled\"}";
                webua->resp_type = WEBUI_RESP_JSON;
                return;
            }
            break;
        }
    }

    /* Get file ID from URI: uri_cmd4 contains the record ID */
    if (webua->uri_cmd4.empty()) {
        webua->resp_code = 400;
        webua->resp_page = "{\"error\":\"File ID required\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    int file_id = mtoi(webua->uri_cmd4);
    if (file_id <= 0) {
        webua->resp_code = 400;
        webua->resp_page = "{\"error\":\"Invalid file ID\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    /* Look up the file in database */
    sql  = " select * from motion ";
    sql += " where record_id = " + std::to_string(file_id);
    sql += " and device_id = " + std::to_string(webua->cam->cfg->device_id);
    sql += " and file_typ = 'movie'";
    app->dbse->filelist_get(sql, flst);

    if (flst.empty()) {
        webua->resp_code = 404;
        webua->resp_page = "{\"error\":\"File not found\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    /* Security: Validate file path to prevent directory traversal */
    full_path = flst[0].full_nm;
    if (full_path.find("..") != std::string::npos) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("Path traversal attempt blocked: %s from %s"),
            full_path.c_str(), webua->clientip.c_str());
        webua->resp_code = 400;
        webua->resp_page = "{\"error\":\"Invalid file path\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    /* Delete the file from filesystem */
    errno = 0;
    if (remove(full_path.c_str()) != 0 && errno != ENOENT) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("Failed to delete file: %s"), full_path.c_str());
        webua->resp_code = 500;
        webua->resp_page = "{\"error\":\"Failed to delete file\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    /* Delete associated thumbnail */
    std::string thumb_path = full_path + ".thumb.jpg";
    errno = 0;
    if (remove(thumb_path.c_str()) != 0 && errno != ENOENT) {
        MOTION_LOG(NTC, TYPE_STREAM, SHOW_ERRNO,
            _("Could not delete thumbnail: %s"), thumb_path.c_str());
        /* Non-fatal - continue with database deletion */
    }

    /* Delete from database */
    sql  = "delete from motion where record_id = " + std::to_string(file_id);
    if (app->dbse->exec_sql(sql) == false) {
        MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO,
            "Database delete failed for movie id=%d", file_id);
        webua->resp_code = 500;
        webua->resp_page = "{\"error\":\"Database delete failed\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    MOTION_LOG(INF, TYPE_ALL, NO_ERRNO,
        "Deleted movie: %s (id=%d) by %s",
        flst[0].file_nm.c_str(), file_id, webua->clientip.c_str());

    webua->resp_page = "{\"success\":true,\"deleted_id\":" + std::to_string(file_id) + "}";
    webua->resp_type = WEBUI_RESP_JSON;
}

void cls_webu_json::api_media_movies()
{
    vec_files flst, flst_count;
    std::string sql, where_clause, cam_id;
    int offset = 0, limit = 100;
    int64_t total_count = 0;
    const char* date_filter = nullptr;

    if (webua->cam == nullptr) {
        webua->bad_request();
        return;
    }

    cam_id = std::to_string(webua->cam->cfg->device_id);

    /* Parse query parameters */
    const char* offset_str = MHD_lookup_connection_value(
        webua->connection, MHD_GET_ARGUMENT_KIND, "offset");
    const char* limit_str = MHD_lookup_connection_value(
        webua->connection, MHD_GET_ARGUMENT_KIND, "limit");
    date_filter = MHD_lookup_connection_value(
        webua->connection, MHD_GET_ARGUMENT_KIND, "date");

    if (offset_str) offset = std::max(0, atoi(offset_str));
    if (limit_str) limit = std::min(std::max(1, atoi(limit_str)), 100); // Cap at 100

    /* Build WHERE clause */
    where_clause  = " where device_id = " + cam_id;
    where_clause += " and file_typ = 'movie'";
    if (date_filter && strlen(date_filter) == 8) {
        where_clause += " and file_dtl = " + std::string(date_filter);
    }

    /* Get total count - query just record_id for efficiency */
    sql = " select record_id from motion " + where_clause + ";";
    app->dbse->filelist_get(sql, flst_count);
    total_count = flst_count.size();

    /* Get paginated results */
    sql  = " select * from motion ";
    sql += where_clause;
    sql += " order by file_dtl desc, file_tml desc";
    sql += " limit " + std::to_string(limit);
    sql += " offset " + std::to_string(offset) + ";";

    app->dbse->filelist_get(sql, flst);

    /* Build JSON response with pagination metadata */
    webua->resp_page = "{";
    webua->resp_page += "\"total_count\":" + std::to_string(total_count) + ",";
    webua->resp_page += "\"offset\":" + std::to_string(offset) + ",";
    webua->resp_page += "\"limit\":" + std::to_string(limit) + ",";
    webua->resp_page += "\"date_filter\":";
    if (date_filter) {
        webua->resp_page += "\"" + std::string(date_filter) + "\"";
    } else {
        webua->resp_page += "null";
    }
    webua->resp_page += ",\"movies\":[";

    for (size_t i = 0; i < flst.size(); i++) {
        if (i > 0) webua->resp_page += ",";
        webua->resp_page += "{";
        webua->resp_page += "\"id\":" + std::to_string(flst[i].record_id) + ",";
        webua->resp_page += "\"filename\":\"" + escstr(flst[i].file_nm) + "\",";
        /* Return URL path for browser access, not filesystem path */
        webua->resp_page += "\"path\":\"/" + cam_id + "/media/" + std::to_string(flst[i].record_id) + "/" + escstr(flst[i].file_nm) + "\",";
        webua->resp_page += "\"date\":\"" + std::to_string(flst[i].file_dtl) + "\",";
        webua->resp_page += "\"time\":\"" + escstr(flst[i].file_tml) + "\",";
        webua->resp_page += "\"size\":" + std::to_string(flst[i].file_sz);

        /* Add thumbnail path if exists */
        std::string thumb_path = flst[i].full_nm + ".thumb.jpg";
        struct stat st;
        if (stat(thumb_path.c_str(), &st) == 0) {
            webua->resp_page += ",\"thumbnail\":\"/" + cam_id + "/media/" +
                                std::to_string(flst[i].record_id) + "/" + escstr(flst[i].file_nm) + ".thumb.jpg\"";
        }

        webua->resp_page += "}";
    }
    webua->resp_page += "]}";
    webua->resp_type = WEBUI_RESP_JSON;
}

void cls_webu_json::api_media_dates()
{
    vec_files flst;
    std::string sql, file_typ;
    std::map<std::string, int> date_counts;
    int64_t total_count = 0;
    const char* type_param;

    if (webua->cam == nullptr) {
        webua->bad_request();
        return;
    }

    /* Parse type parameter (required) */
    type_param = MHD_lookup_connection_value(
        webua->connection, MHD_GET_ARGUMENT_KIND, "type");

    if (!type_param || (strcmp(type_param, "pic") != 0 && strcmp(type_param, "movie") != 0)) {
        webua->resp_page = "{\"error\":\"Invalid or missing 'type' parameter. Must be 'pic' or 'movie'\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    file_typ = type_param;

    /* Query all records for this type to build date summary */
    sql  = " select record_id, file_dtl from motion ";
    sql += " where device_id = " + std::to_string(webua->cam->cfg->device_id);
    sql += " and file_typ = '" + file_typ + "'";
    sql += " order by file_dtl desc;";

    app->dbse->filelist_get(sql, flst);
    total_count = flst.size();

    /* Group by date */
    for (size_t i = 0; i < flst.size(); i++) {
        std::string date_str = std::to_string(flst[i].file_dtl);
        date_counts[date_str]++;
    }

    /* Build JSON response */
    webua->resp_page = "{";
    webua->resp_page += "\"type\":\"" + file_typ + "\",";
    webua->resp_page += "\"total_count\":" + std::to_string(total_count) + ",";
    webua->resp_page += "\"dates\":[";

    bool first = true;
    for (const auto& pair : date_counts) {
        if (!first) webua->resp_page += ",";
        webua->resp_page += "{";
        webua->resp_page += "\"date\":\"" + pair.first + "\",";
        webua->resp_page += "\"count\":" + std::to_string(pair.second);
        webua->resp_page += "}";
        first = false;
    }

    webua->resp_page += "]}";
    webua->resp_type = WEBUI_RESP_JSON;
}

static bool is_media_extension(const std::string &ext)
{
    static const std::set<std::string> media_exts = {
        ".mp4", ".mkv", ".avi", ".webm", ".mov",
        ".jpg", ".jpeg", ".png", ".gif", ".bmp"
    };
    std::string lower_ext = ext;
    std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);
    return media_exts.find(lower_ext) != media_exts.end();
}

static bool is_thumbnail(const std::string &filename)
{
    return filename.length() > 10 &&
           filename.substr(filename.length() - 10) == ".thumb.jpg";
}

static std::string get_file_extension(const std::string &filename)
{
    size_t dot_pos = filename.rfind('.');
    if (dot_pos == std::string::npos || dot_pos == 0) return "";
    return filename.substr(dot_pos);
}

/* Determine media file type from extension (case-insensitive) */
static std::string get_media_type(const std::string &filename)
{
    std::string ext = get_file_extension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" ||
        ext == ".gif" || ext == ".bmp") {
        return "picture";
    }
    return "movie";
}

static bool validate_folder_path(const std::string &target_dir, const std::string &rel_path,
                                 std::string &full_path)
{
    /* Check for path traversal attempts */
    if (rel_path.find("..") != std::string::npos) {
        return false;
    }

    /* Build full path */
    full_path = target_dir;
    if (!full_path.empty() && full_path.back() != '/') {
        full_path += '/';
    }
    if (!rel_path.empty()) {
        full_path += rel_path;
    }

    /* Resolve symlinks and check real path is still under target_dir */
    char resolved[PATH_MAX];
    if (realpath(full_path.c_str(), resolved) == nullptr) {
        /* Path doesn't exist - that's ok for empty folder case */
        return true;
    }

    std::string real_path(resolved);
    char target_resolved[PATH_MAX];
    if (realpath(target_dir.c_str(), target_resolved) == nullptr) {
        return false;
    }
    std::string real_target(target_resolved);

    /* Ensure resolved path starts with target_dir */
    if (real_path.length() < real_target.length() ||
        real_path.substr(0, real_target.length()) != real_target) {
        return false;
    }

    /* Ensure it's either exactly target_dir or has a / separator after */
    if (real_path.length() > real_target.length() &&
        real_path[real_target.length()] != '/') {
        return false;
    }

    return true;
}

void cls_webu_json::api_media_folders()
{
    vec_files flst;
    std::string sql, target_dir, full_path;
    int offset = 0, limit = 100;
    const char* path_param = nullptr;
    const char* type_param = nullptr;
    std::string rel_path;
    std::string type_filter;  /* "picture", "movie", or empty for all */

    if (webua->cam == nullptr) {
        webua->bad_request();
        return;
    }

    /* Get target directory for this camera */
    target_dir = webua->cam->cfg->target_dir;
    if (target_dir.empty()) {
        webua->resp_page = "{\"error\":\"Target directory not configured\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    /* Parse query parameters */
    path_param = MHD_lookup_connection_value(
        webua->connection, MHD_GET_ARGUMENT_KIND, "path");
    const char* offset_str = MHD_lookup_connection_value(
        webua->connection, MHD_GET_ARGUMENT_KIND, "offset");
    const char* limit_str = MHD_lookup_connection_value(
        webua->connection, MHD_GET_ARGUMENT_KIND, "limit");
    type_param = MHD_lookup_connection_value(
        webua->connection, MHD_GET_ARGUMENT_KIND, "type");

    if (path_param) rel_path = path_param;
    if (offset_str) offset = std::max(0, atoi(offset_str));
    if (limit_str) limit = std::min(std::max(1, atoi(limit_str)), 100);
    if (type_param && (strcmp(type_param, "picture") == 0 || strcmp(type_param, "movie") == 0)) {
        type_filter = type_param;
    }

    /* Validate and build full path */
    if (!validate_folder_path(target_dir, rel_path, full_path)) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("Path traversal attempt blocked: %s from %s"),
            rel_path.c_str(), webua->clientip.c_str());
        webua->resp_page = "{\"error\":\"Invalid path\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    /* Open directory */
    DIR *dir = opendir(full_path.c_str());
    if (dir == nullptr) {
        webua->resp_page = "{\"error\":\"Directory not found\"}";
        webua->resp_type = WEBUI_RESP_JSON;
        return;
    }

    /* Scan directory entries */
    struct dirent *entry;
    std::vector<std::pair<std::string, std::string>> folders; /* name, path */
    std::vector<std::pair<std::string, std::string>> media_files; /* filename, type */
    int total_pictures = 0;
    int total_movies = 0;

    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;

        /* Skip . and .. */
        if (name == "." || name == "..") continue;

        /* Skip hidden files */
        if (name[0] == '.') continue;

        std::string entry_path = full_path + "/" + name;
        struct stat st;
        if (stat(entry_path.c_str(), &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            /* Directory - add to folders list */
            std::string folder_rel = rel_path.empty() ? name : rel_path + "/" + name;
            folders.push_back({name, folder_rel});
        } else if (S_ISREG(st.st_mode)) {
            /* Regular file - check if it's a media file (not thumbnail) */
            std::string ext = get_file_extension(name);
            if (is_media_extension(ext) && !is_thumbnail(name)) {
                std::string file_type = get_media_type(name);

                /* Track totals for all types */
                if (file_type == "picture") {
                    total_pictures++;
                } else {
                    total_movies++;
                }

                /* Apply type filter if specified */
                if (type_filter.empty() || type_filter == file_type) {
                    media_files.push_back({name, file_type});
                }
            }
        }
    }
    closedir(dir);

    /* Sort folders and files reverse alphabetically (newest first) */
    std::sort(folders.begin(), folders.end(), std::greater<>());
    std::sort(media_files.begin(), media_files.end(), std::greater<>());

    /* Calculate folder statistics (file count, total size) */
    std::string cam_id = std::to_string(webua->cam->cfg->device_id);

    /* Build JSON response */
    webua->resp_page = "{";
    webua->resp_page += "\"path\":\"" + escstr(rel_path) + "\",";

    /* Parent path for navigation */
    if (rel_path.empty()) {
        webua->resp_page += "\"parent\":null,";
    } else {
        size_t last_slash = rel_path.rfind('/');
        std::string parent = (last_slash == std::string::npos) ? "" : rel_path.substr(0, last_slash);
        webua->resp_page += "\"parent\":\"" + escstr(parent) + "\",";
    }

    /* Folders */
    webua->resp_page += "\"folders\":[";
    for (size_t i = 0; i < folders.size(); i++) {
        if (i > 0) webua->resp_page += ",";

        /* Count files in this folder (from database) */
        std::string folder_path = full_path + "/" + folders[i].first;
        int64_t file_count = 0;
        int64_t total_size = 0;

        /* Count by scanning directory */
        DIR *subdir = opendir(folder_path.c_str());
        if (subdir != nullptr) {
            struct dirent *subentry;
            while ((subentry = readdir(subdir)) != nullptr) {
                std::string subname = subentry->d_name;
                if (subname == "." || subname == "..") continue;
                std::string subpath = folder_path + "/" + subname;
                struct stat sub_st;
                if (stat(subpath.c_str(), &sub_st) == 0 && S_ISREG(sub_st.st_mode)) {
                    std::string ext = get_file_extension(subname);
                    if (is_media_extension(ext) && !is_thumbnail(subname)) {
                        file_count++;
                        total_size += sub_st.st_size;
                    }
                }
            }
            closedir(subdir);
        }

        webua->resp_page += "{";
        webua->resp_page += "\"name\":\"" + escstr(folders[i].first) + "\",";
        webua->resp_page += "\"path\":\"" + escstr(folders[i].second) + "\",";
        webua->resp_page += "\"file_count\":" + std::to_string(file_count) + ",";
        webua->resp_page += "\"total_size\":" + std::to_string(total_size);
        webua->resp_page += "}";
    }
    webua->resp_page += "],";

    /* Files with pagination */
    int total_files = (int)media_files.size();
    int start_idx = std::min(offset, total_files);
    int end_idx = std::min(offset + limit, total_files);

    webua->resp_page += "\"files\":[";
    for (int i = start_idx; i < end_idx; i++) {
        if (i > start_idx) webua->resp_page += ",";

        std::string filename = media_files[i].first;
        std::string file_type = media_files[i].second;
        std::string file_path = full_path + "/" + filename;
        struct stat st;
        stat(file_path.c_str(), &st);

        /* Look up in database for metadata */
        sql = " select * from motion ";
        sql += " where device_id = " + cam_id;
        sql += " and file_nm = '" + filename + "'";
        sql += " limit 1;";
        flst.clear();
        app->dbse->filelist_get(sql, flst);

        webua->resp_page += "{";

        if (!flst.empty()) {
            webua->resp_page += "\"id\":" + std::to_string(flst[0].record_id) + ",";
            webua->resp_page += "\"date\":\"" + std::to_string(flst[0].file_dtl) + "\",";
            webua->resp_page += "\"time\":\"" + escstr(flst[0].file_tml) + "\",";
        } else {
            webua->resp_page += "\"id\":0,";
            /* Extract date from filename if possible (common format: camera-YYYYMMDD...) */
            webua->resp_page += "\"date\":\"\",";
            webua->resp_page += "\"time\":\"\",";
        }

        webua->resp_page += "\"filename\":\"" + escstr(filename) + "\",";

        /* Build URL path for access */
        if (file_type == "movie") {
            std::string url_path = "/" + cam_id + "/media/";
            if (!rel_path.empty()) url_path += rel_path + "/";
            url_path += filename;
            webua->resp_page += "\"path\":\"" + escstr(url_path) + "\",";

            /* Check for thumbnail */
            std::string thumb_file = file_path + ".thumb.jpg";
            struct stat thumb_st;
            if (stat(thumb_file.c_str(), &thumb_st) == 0) {
                webua->resp_page += "\"thumbnail\":\"" + escstr(url_path + ".thumb.jpg") + "\",";
            }
        } else {
            /* Pictures use URL path like movies */
            std::string pic_url = "/" + cam_id + "/media/";
            if (!rel_path.empty()) pic_url += rel_path + "/";
            pic_url += filename;
            webua->resp_page += "\"path\":\"" + escstr(pic_url) + "\",";
        }

        webua->resp_page += "\"type\":\"" + file_type + "\",";
        webua->resp_page += "\"size\":" + std::to_string(st.st_size);
        webua->resp_page += "}";
    }
    webua->resp_page += "],";

    webua->resp_page += "\"total_files\":" + std::to_string(total_files) + ",";
    webua->resp_page += "\"total_pictures\":" + std::to_string(total_pictures) + ",";
    webua->resp_page += "\"total_movies\":" + std::to_string(total_movies) + ",";
    webua->resp_page += "\"offset\":" + std::to_string(offset) + ",";
    webua->resp_page += "\"limit\":" + std::to_string(limit);
    webua->resp_page += "}";
    webua->resp_type = WEBUI_RESP_JSON;
}

void cls_webu_json::api_delete_folder_files()
{
    std::string target_dir, full_path, sql;
    const char* path_param = nullptr;
    std::string rel_path;
    int deleted_movies = 0, deleted_pictures = 0, deleted_thumbnails = 0;
    std::vector<std::string> errors;

    webua->resp_type = WEBUI_RESP_JSON;

    if (webua->cam == nullptr) {
        webua->resp_page = "{\"error\":\"Camera not specified\"}";
        return;
    }

    /* Require admin role */
    if (webua->auth_role != "admin") {
        MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO,
            _("Delete folder files denied - requires admin role (from %s)"),
            webua->clientip.c_str());
        webua->resp_page = "{\"error\":\"Admin access required\"}";
        return;
    }

    /* Check if delete action is enabled */
    for (int indx = 0; indx < webu->wb_actions->params_cnt; indx++) {
        if (webu->wb_actions->params_array[indx].param_name == "delete") {
            if (webu->wb_actions->params_array[indx].param_value == "off") {
                MOTION_LOG(INF, TYPE_ALL, NO_ERRNO, "Delete action disabled");
                webua->resp_page = "{\"error\":\"Delete action is disabled\"}";
                return;
            }
            break;
        }
    }

    /* Get path parameter (required) */
    path_param = MHD_lookup_connection_value(
        webua->connection, MHD_GET_ARGUMENT_KIND, "path");

    if (path_param == nullptr) {
        webua->resp_page = "{\"error\":\"Path parameter required\"}";
        return;
    }
    rel_path = path_param;

    /* Get target directory for this camera */
    target_dir = webua->cam->cfg->target_dir;
    if (target_dir.empty()) {
        webua->resp_page = "{\"error\":\"Target directory not configured\"}";
        return;
    }

    /* Validate and build full path */
    if (!validate_folder_path(target_dir, rel_path, full_path)) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("Path traversal attempt blocked: %s from %s"),
            rel_path.c_str(), webua->clientip.c_str());
        webua->resp_page = "{\"error\":\"Invalid path\"}";
        return;
    }

    /* Open directory */
    DIR *dir = opendir(full_path.c_str());
    if (dir == nullptr) {
        webua->resp_page = "{\"error\":\"Directory not found\"}";
        return;
    }

    MOTION_LOG(INF, TYPE_ALL, NO_ERRNO,
        "Delete all media files in folder '%s' requested by %s",
        rel_path.c_str(), webua->clientip.c_str());

    /* Collect media files to delete */
    struct dirent *entry;
    std::vector<std::string> files_to_delete;
    std::vector<std::string> thumbs_to_delete;

    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;

        std::string entry_path = full_path + "/" + name;
        struct stat st;
        if (stat(entry_path.c_str(), &st) != 0) continue;

        if (S_ISREG(st.st_mode)) {
            std::string ext = get_file_extension(name);
            if (is_thumbnail(name)) {
                /* Track thumbnails separately - they'll be deleted with their movie */
                continue;
            } else if (is_media_extension(ext)) {
                files_to_delete.push_back(entry_path);
                /* Check for associated thumbnail */
                std::string thumb_path = entry_path + ".thumb.jpg";
                struct stat thumb_st;
                if (stat(thumb_path.c_str(), &thumb_st) == 0) {
                    thumbs_to_delete.push_back(thumb_path);
                }
            }
        }
    }

    closedir(dir);

    std::string cam_id = std::to_string(webua->cam->cfg->device_id);

    /* Build progress key and initialize tracking */
    std::string progress_key = std::to_string(webua->cam->cfg->device_id) + ":" + rel_path;

    pthread_mutex_lock(&app->mutex_delete_progress);
    app->delete_progress_map[progress_key] = {
        .in_progress = true,
        .total_files = static_cast<int>(files_to_delete.size()),
        .current_index = 0,
        .deleted_movies = 0,
        .deleted_pictures = 0,
        .deleted_thumbnails = 0,
        .path = rel_path,
        .completion_time = 0
    };

    pthread_mutex_unlock(&app->mutex_delete_progress);

    int current_index = 0;

    /* Delete files */
    for (const auto& file_path : files_to_delete) {
        std::string ext = get_file_extension(file_path);
        bool is_movie = (ext == ".mp4" || ext == ".mkv" || ext == ".avi" ||
                        ext == ".webm" || ext == ".mov");

        if (remove(file_path.c_str()) == 0) {
            if (is_movie) {
                deleted_movies++;
            } else {
                deleted_pictures++;
            }

            /* Delete from database */
            size_t last_slash = file_path.rfind('/');
            std::string filename = (last_slash == std::string::npos) ?
                file_path : file_path.substr(last_slash + 1);

            sql = "delete from motion where device_id = " + cam_id +
                  " and file_nm = '" + filename + "'";
            app->dbse->exec_sql(sql);
        } else {
            errors.push_back("Failed to delete: " + file_path);
            MOTION_LOG(ERR, TYPE_STREAM, SHOW_ERRNO,
                _("Failed to delete file: %s"), file_path.c_str());
        }

        current_index++;

        /* Update progress every 10 files to minimize mutex contention */
        if ((current_index % 10 == 0) || (current_index == static_cast<int>(files_to_delete.size()))) {
            pthread_mutex_lock(&app->mutex_delete_progress);
            auto it = app->delete_progress_map.find(progress_key);
            if (it != app->delete_progress_map.end()) {
                it->second.current_index = current_index;
                it->second.deleted_movies = deleted_movies;
                it->second.deleted_pictures = deleted_pictures;
            }
            pthread_mutex_unlock(&app->mutex_delete_progress);
        }
    }

    /* Delete thumbnails */
    for (const auto& thumb_path : thumbs_to_delete) {
        if (remove(thumb_path.c_str()) == 0) {
            deleted_thumbnails++;
        }
    }

    /* Mark operation complete */
    pthread_mutex_lock(&app->mutex_delete_progress);
    auto it = app->delete_progress_map.find(progress_key);
    if (it != app->delete_progress_map.end()) {
        it->second.in_progress = false;
        it->second.current_index = it->second.total_files;
        it->second.deleted_thumbnails = deleted_thumbnails;
        it->second.completion_time = time(nullptr);
    }
    pthread_mutex_unlock(&app->mutex_delete_progress);

    MOTION_LOG(INF, TYPE_ALL, NO_ERRNO,
        "Deleted %d movies, %d pictures, %d thumbnails from '%s'",
        deleted_movies, deleted_pictures, deleted_thumbnails, rel_path.c_str());

    /* Build response */
    webua->resp_page = "{";
    webua->resp_page += "\"success\":true,";
    webua->resp_page += "\"deleted\":{";
    webua->resp_page += "\"movies\":" + std::to_string(deleted_movies) + ",";
    webua->resp_page += "\"pictures\":" + std::to_string(deleted_pictures) + ",";
    webua->resp_page += "\"thumbnails\":" + std::to_string(deleted_thumbnails);
    webua->resp_page += "},";
    webua->resp_page += "\"errors\":[";
    for (size_t i = 0; i < errors.size(); i++) {
        if (i > 0) webua->resp_page += ",";
        webua->resp_page += "\"" + escstr(errors[i]) + "\"";
    }
    webua->resp_page += "],";
    webua->resp_page += "\"path\":\"" + escstr(rel_path) + "\"";
    webua->resp_page += "}";
}

void cls_webu_json::api_delete_progress()
{
    std::string progress_key;
    const char* path_param = nullptr;

    webua->resp_type = WEBUI_RESP_JSON;

    if (webua->cam == nullptr) {
        webua->resp_page = "{\"error\":\"Camera not specified\"}";
        return;
    }

    /* Get path parameter */
    path_param = MHD_lookup_connection_value(
        webua->connection, MHD_GET_ARGUMENT_KIND, "path");
    std::string path = path_param ? path_param : "";

    /* Build progress key */
    progress_key = std::to_string(webua->cam->cfg->device_id) + ":" + path;

    /* Lock and read progress */
    pthread_mutex_lock(&app->mutex_delete_progress);

    auto it = app->delete_progress_map.find(progress_key);
    if (it == app->delete_progress_map.end()) {
        pthread_mutex_unlock(&app->mutex_delete_progress);
        webua->resp_page = "{\"in_progress\":false,\"total\":0,\"current\":0,"
                          "\"deleted\":{\"movies\":0,\"pictures\":0,\"thumbnails\":0}}";
        return;
    }

    ctx_delete_progress prog = it->second;  /* Copy before unlock */
    pthread_mutex_unlock(&app->mutex_delete_progress);

    /* Build JSON response */
    webua->resp_page = "{";
    webua->resp_page += "\"in_progress\":" + std::string(prog.in_progress ? "true" : "false") + ",";
    webua->resp_page += "\"total\":" + std::to_string(prog.total_files) + ",";
    webua->resp_page += "\"current\":" + std::to_string(prog.current_index) + ",";
    webua->resp_page += "\"deleted\":{";
    webua->resp_page += "\"movies\":" + std::to_string(prog.deleted_movies) + ",";
    webua->resp_page += "\"pictures\":" + std::to_string(prog.deleted_pictures) + ",";
    webua->resp_page += "\"thumbnails\":" + std::to_string(prog.deleted_thumbnails);
    webua->resp_page += "},";
    webua->resp_page += "\"path\":\"" + escstr(prog.path) + "\"";
    webua->resp_page += "}";
}
