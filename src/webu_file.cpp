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

#include "motion.hpp"
#include "util.hpp"
#include "camera.hpp"
#include "conf.hpp"
#include "logger.hpp"
#include "picture.hpp"
#include "webu.hpp"
#include "webu_ans.hpp"
#include "webu_file.hpp"
#include "dbse.hpp"

#include <climits>  /* For PATH_MAX */

/**
 * Validate that a requested file path is within the allowed base directory
 * Prevents path traversal attacks (e.g., ../../../etc/passwd)
 *
 * Uses realpath() to resolve symlinks and relative components,
 * then verifies the resolved path starts with the allowed base.
 *
 * @param requested_path The full path requested (may contain ../ or symlinks)
 * @param allowed_base The base directory that files must be within
 * @return true if the path is safe, false if it's outside allowed_base
 */
static bool validate_file_path(const std::string &requested_path,
    const std::string &allowed_base)
{
    char resolved_request[PATH_MAX];
    char resolved_base[PATH_MAX];

    /* Resolve the requested path to its canonical form */
    if (realpath(requested_path.c_str(), resolved_request) == nullptr) {
        /* File doesn't exist or path is invalid - this is not necessarily
         * a traversal attack, but we can't verify it's safe */
        return false;
    }

    /* Resolve the allowed base directory */
    if (realpath(allowed_base.c_str(), resolved_base) == nullptr) {
        /* Base directory doesn't exist - configuration error */
        return false;
    }

    std::string req_str(resolved_request);
    std::string base_str(resolved_base);

    /* Ensure base path ends with / for proper prefix matching */
    if (base_str.back() != '/') {
        base_str += '/';
    }

    /* Check that resolved request starts with allowed base
     * This prevents:
     * - ../../../etc/passwd -> /etc/passwd (doesn't start with /home/motion/videos/)
     * - Symlink escapes: /videos/link -> /etc (resolved path is /etc/...)
     */
    if (req_str.length() < base_str.length()) {
        return false;
    }

    return (req_str.compare(0, base_str.length(), base_str) == 0);
}

/**
 * Get MIME type based on file extension
 */
static std::string get_mime_type(const std::string &filename)
{
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "application/octet-stream";
    }

    std::string ext = filename.substr(dot_pos + 1);
    mylower(ext);

    if (ext == "html") return "text/html; charset=utf-8";
    if (ext == "htm")  return "text/html; charset=utf-8";
    if (ext == "js")   return "text/javascript; charset=utf-8";
    if (ext == "css")  return "text/css; charset=utf-8";
    if (ext == "json") return "application/json; charset=utf-8";
    if (ext == "png")  return "image/png";
    if (ext == "jpg")  return "image/jpeg";
    if (ext == "jpeg") return "image/jpeg";
    if (ext == "gif")  return "image/gif";
    if (ext == "svg")  return "image/svg+xml";
    if (ext == "ico")  return "image/x-icon";
    if (ext == "woff") return "font/woff";
    if (ext == "woff2") return "font/woff2";
    if (ext == "ttf")  return "font/ttf";
    if (ext == "eot")  return "application/vnd.ms-fontobject";

    return "application/octet-stream";
}

/**
 * Get cache control header based on path
 * - /assets/* files are hashed, cache aggressively
 * - index.html must not be cached (for SPA updates)
 */
static std::string get_cache_control(const std::string &path)
{
    if (path.find("/assets/") != std::string::npos) {
        return "public, max-age=31536000, immutable";  /* 1 year */
    }
    if (path.find("index.html") != std::string::npos) {
        return "no-cache, no-store, must-revalidate";
    }
    return "public, max-age=3600";  /* 1 hour for other files */
}

/* Callback for the file reader response*/
static ssize_t webu_file_reader (void *cls, uint64_t pos, char *buf, size_t max)
{
    cls_webu_ans *webu_ans =(cls_webu_ans *)cls;
    (void)fseek (webu_ans->req_file, (long)(webu_ans->req_file_offset + pos), SEEK_SET);
    return (ssize_t)fread (buf, 1, max, webu_ans->req_file);
}

/* Parse HTTP Range header. Returns true if a valid single byte range was parsed. */
static bool parse_range_header(const char *range_str, uint64_t file_size,
    uint64_t &range_start, uint64_t &range_end)
{
    if (range_str == nullptr) {
        return false;
    }

    /* Must start with "bytes=" */
    if (strncmp(range_str, "bytes=", 6) != 0) {
        return false;
    }

    const char *spec = range_str + 6;

    /* Suffix range: bytes=-500 (last 500 bytes) */
    if (spec[0] == '-') {
        uint64_t suffix_len = (uint64_t)strtoull(spec + 1, nullptr, 10);
        if (suffix_len == 0 || suffix_len > file_size) {
            return false;
        }
        range_start = file_size - suffix_len;
        range_end = file_size - 1;
        return true;
    }

    /* Standard range: bytes=START-END or bytes=START- */
    char *endptr;
    range_start = (uint64_t)strtoull(spec, &endptr, 10);
    if (*endptr != '-') {
        return false;
    }
    if (range_start >= file_size) {
        return false;
    }

    endptr++; /* skip the '-' */
    if (*endptr == '\0' || *endptr == ',') {
        /* Open-ended: bytes=START- */
        range_end = file_size - 1;
    } else {
        range_end = (uint64_t)strtoull(endptr, nullptr, 10);
        if (range_end >= file_size) {
            range_end = file_size - 1;
        }
    }

    if (range_start > range_end) {
        return false;
    }

    return true;
}

void cls_webu_file::main() {
    mhdrslt retcd;
    struct stat statbuf;
    struct MHD_Response *response;
    std::string full_nm;
    vec_files flst;
    int indx;
    std::string sql;

    /*If we have not fully started yet, simply return*/
    if (app->dbse == NULL) {
        webua->bad_request();
        return;
    }

    for (indx=0;indx<webu->wb_actions->params_cnt;indx++) {
        if (webu->wb_actions->params_array[indx].param_name == "movies") {
            if (webu->wb_actions->params_array[indx].param_value == "off") {
                MOTION_LOG(INF, TYPE_ALL, NO_ERRNO, "Movies via webcontrol disabled");
                webua->bad_request();
                return;
            } else {
                break;
            }
        }
    }


    full_nm = "";

    /* Check if this is a thumbnail request (.thumb.jpg suffix) */
    std::string requested_file = webua->uri_cmd2;
    std::string original_requested_file = requested_file;  /* Preserve FULL path for fallback (including .thumb.jpg if present) */
    bool is_thumbnail = false;
    const std::string thumb_suffix = ".thumb.jpg";

    if (requested_file.length() > thumb_suffix.length() &&
        requested_file.substr(requested_file.length() - thumb_suffix.length()) == thumb_suffix) {
        /* Strip suffix to get base video filename for database lookup */
        requested_file = requested_file.substr(0, requested_file.length() - thumb_suffix.length());
        /* Note: original_requested_file keeps the .thumb.jpg suffix for direct path fallback */
        is_thumbnail = true;
    }

    /* Try to extract record_id from URL (new format: "123/filename.jpg")
     * Only treat as record_id if it's a reasonable database ID (< 10000000)
     * to avoid confusing date folders like "20250130" with record IDs */
    int record_id = 0;
    size_t first_slash = requested_file.find('/');
    if (first_slash != std::string::npos) {
        std::string id_str = requested_file.substr(0, first_slash);
        bool is_numeric = !id_str.empty();
        for (char c : id_str) {
            if (!isdigit(c)) { is_numeric = false; break; }
        }
        if (is_numeric) {
            int potential_id = atoi(id_str.c_str());
            /* Only treat as record_id if it looks like a database ID, not a date folder */
            if (potential_id > 0 && potential_id < 10000000) {
                record_id = potential_id;
                requested_file = requested_file.substr(first_slash + 1);
            }
        }
    }

    if (record_id > 0) {
        /* Direct record_id lookup - O(1) with primary key index */
        sql = "SELECT * FROM motion WHERE record_id = " + std::to_string(record_id) +
              " AND device_id = " + std::to_string(webua->cam->cfg->device_id) + ";";
        app->dbse->filelist_get(sql, flst);
        if (!flst.empty()) {
            full_nm = is_thumbnail ? flst[0].full_nm + ".thumb.jpg" : flst[0].full_nm;
        }
    } else {
        /* Legacy filename-based lookup (backwards compat) */
        sql  = " select * from motion ";
        sql += " where device_id = " + std::to_string(webua->cam->cfg->device_id);
        sql += " order by file_dtl, file_tml;";
        app->dbse->filelist_get(sql, flst);

        /* Extract just the filename from the request (may include subdirectory path) */
        std::string requested_filename = requested_file;
        size_t last_slash = requested_file.rfind('/');
        if (last_slash != std::string::npos) {
            requested_filename = requested_file.substr(last_slash + 1);
        }

        for (indx=0;indx<(int)flst.size();indx++) {
            if (flst[indx].file_nm == requested_filename) {
                if (is_thumbnail) {
                    full_nm = flst[indx].full_nm + ".thumb.jpg";
                } else {
                    full_nm = flst[indx].full_nm;
                }
                break;
            }
        }
    }

    /* If not found in database, try direct path construction for subfolder files
     * Only use this fallback for date-folder URLs (record_id == 0), not for record_id URLs
     * because the numeric prefix in record_id URLs is a database ID, not a folder name.
     * original_requested_file already contains the full path including .thumb.jpg if applicable */
    if (full_nm.empty() && !original_requested_file.empty() && record_id == 0) {
        std::string direct_path = webua->cam->cfg->target_dir + "/" + original_requested_file;
        struct stat st;
        if (stat(direct_path.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            full_nm = direct_path;
        }
    }

    /* SECURITY: Validate path before serving file to prevent path traversal attacks
     * This catches:
     * - Database entries modified to contain ../../../etc/passwd
     * - Symlink escapes from target_dir
     * - URL-encoded traversal attempts (already decoded by this point)
     */
    if (!full_nm.empty() && !validate_file_path(full_nm, webua->cam->cfg->target_dir)) {
        MOTION_LOG(ALR, TYPE_STREAM, NO_ERRNO,
            _("Path traversal attempt blocked: %s requested %s (resolved outside %s) from %s"),
            webua->uri_cmd2.c_str(), full_nm.c_str(),
            webua->cam->cfg->target_dir.c_str(), webua->clientip.c_str());
        webua->bad_request();
        return;
    }

    if (stat(full_nm.c_str(), &statbuf) == 0) {
        webua->req_file = myfopen(full_nm.c_str(), "rbe");
    } else {
        webua->req_file = nullptr;
        MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO
            ,"Security warning: Client IP %s requested file: %s"
            ,webua->clientip.c_str(), webua->uri_cmd2.c_str());
    }

    if (webua->req_file == nullptr) {
        webua->resp_page = "<html><head><title>Bad File</title>"
            "</head><body>Bad File</body></html>";
        webua->resp_type = WEBUI_RESP_HTML;
        webua->mhd_send();
        retcd = MHD_YES;
    } else {
        uint64_t file_size = (uint64_t)statbuf.st_size;
        uint64_t range_start = 0;
        uint64_t range_end = 0;
        unsigned int http_status = MHD_HTTP_OK;

        /* Check for Range request header */
        const char *range_hdr = MHD_lookup_connection_value(
            webua->connection, MHD_HEADER_KIND, "Range");

        bool has_range = parse_range_header(range_hdr, file_size,
            range_start, range_end);

        if (range_hdr != nullptr && !has_range) {
            /* Invalid or unsatisfiable range */
            if (webua->req_file != nullptr) {
                myfclose(webua->req_file);
                webua->req_file = nullptr;
            }
            response = MHD_create_response_from_buffer(0, (void *)"",
                MHD_RESPMEM_MUST_COPY);
            std::string cr_hdr = "bytes */" + std::to_string(file_size);
            MHD_add_response_header(response, "Content-Range", cr_hdr.c_str());
            retcd = MHD_queue_response(webua->connection,
                MHD_HTTP_RANGE_NOT_SATISFIABLE, response);
            MHD_destroy_response(response);
            if (retcd == MHD_NO) {
                MOTION_LOG(INF, TYPE_ALL, NO_ERRNO, "Error processing file request");
            }
            return;
        }

        uint64_t resp_size;
        if (has_range) {
            resp_size = range_end - range_start + 1;
            webua->req_file_offset = range_start;
            http_status = MHD_HTTP_PARTIAL_CONTENT;
        } else {
            resp_size = file_size;
            webua->req_file_offset = 0;
        }

        response = MHD_create_response_from_callback(
            (size_t)resp_size, 32 * 1024,
            &webu_file_reader, webua, NULL);
        if (response == NULL) {
            if (webua->req_file != nullptr) {
                myfclose(webua->req_file);
                webua->req_file = nullptr;
            }
            webua->bad_request();
            return;
        }

        /* Set Content-Type based on file extension */
        std::string ext = full_nm.substr(full_nm.rfind('.') + 1);
        const char *mime = "application/octet-stream";
        if (ext == "jpg" || ext == "jpeg") mime = "image/jpeg";
        else if (ext == "png") mime = "image/png";
        else if (ext == "gif") mime = "image/gif";
        else if (ext == "mkv") mime = "video/x-matroska";
        else if (ext == "mp4" || ext == "m4v") mime = "video/mp4";
        else if (ext == "avi") mime = "video/x-msvideo";
        else if (ext == "webm") mime = "video/webm";
        MHD_add_response_header(response, "Content-Type", mime);

        /* Always advertise Range support */
        MHD_add_response_header(response, "Accept-Ranges", "bytes");

        if (has_range) {
            std::string cr_val = "bytes " + std::to_string(range_start)
                + "-" + std::to_string(range_end)
                + "/" + std::to_string(file_size);
            MHD_add_response_header(response, "Content-Range", cr_val.c_str());
        }

        retcd = MHD_queue_response(webua->connection, http_status, response);
        MHD_destroy_response(response);
    }
    if (retcd == MHD_NO) {
        MOTION_LOG(INF, TYPE_ALL, NO_ERRNO, "Error processing file request");
    }

}

/**
 * Serve static files from React build directory
 * Implements SPA routing: if file not found, serve index.html
 */
void cls_webu_file::serve_static_file()
{
    struct stat statbuf;
    std::string file_path;
    std::string index_path;
    FILE *file_handle = nullptr;
    struct MHD_Response *response;
    mhdrslt retcd;

    /* Construct file path from webcontrol_html_path + request URI */
    file_path = app->cfg->webcontrol_html_path;
    if (file_path.back() != '/') {
        file_path += '/';
    }

    /* Use full URL path, not just uri_cmd1 which only contains first segment */
    std::string uri = webua->url;
    if (uri.empty() || uri == "/") {
        uri = "index.html";
    } else if (uri[0] == '/') {
        uri = uri.substr(1);
    }

    file_path += uri;

    /* Try to serve the requested file */
    if (stat(file_path.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
        /* Security: Validate path to prevent traversal attacks (only if file exists) */
        if (!validate_file_path(file_path, app->cfg->webcontrol_html_path)) {
            MOTION_LOG(WRN, TYPE_STREAM, NO_ERRNO,
                _("Path traversal attempt blocked: %s from %s"),
                file_path.c_str(), webua->clientip.c_str());
            webua->bad_request();
            return;
        }
        file_handle = myfopen(file_path.c_str(), "rbe");
    }

    /* If file not found and SPA mode enabled, serve index.html */
    if (file_handle == nullptr && app->cfg->webcontrol_spa_mode) {
        index_path = app->cfg->webcontrol_html_path;
        if (index_path.back() != '/') {
            index_path += '/';
        }
        index_path += "index.html";

        if (stat(index_path.c_str(), &statbuf) == 0) {
            file_handle = myfopen(index_path.c_str(), "rbe");
            file_path = index_path;  /* For MIME type detection */
        }
    }

    /* If still no file, return 404 */
    if (file_handle == nullptr) {
        MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO,
            _("Static file not found: %s from %s"),
            uri.c_str(), webua->clientip.c_str());
        webua->bad_request();
        return;
    }

    /* Create response from file */
    webua->req_file = file_handle;
    response = MHD_create_response_from_callback(
        (size_t)statbuf.st_size, 32 * 1024,
        &webu_file_reader,
        webua, NULL);

    if (response == NULL) {
        myfclose(webua->req_file);
        webua->req_file = nullptr;
        webua->bad_request();
        return;
    }

    /* Set Content-Type header */
    std::string mime_type = get_mime_type(file_path);
    MHD_add_response_header(response, "Content-Type", mime_type.c_str());

    /* Set Cache-Control header */
    std::string cache_control = get_cache_control(file_path);
    MHD_add_response_header(response, "Cache-Control", cache_control.c_str());

    /* Security headers */
    MHD_add_response_header(response, "X-Content-Type-Options", "nosniff");

    retcd = MHD_queue_response(webua->connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    if (retcd == MHD_NO) {
        MOTION_LOG(WRN, TYPE_STREAM, NO_ERRNO,
            _("Error queueing static file response"));
    }
}

cls_webu_file::cls_webu_file(cls_webu_ans *p_webua)
{
    app     = p_webua->app;
    webu    = p_webua->webu;
    webua   = p_webua;
}

cls_webu_file::~cls_webu_file()
{
    app    = nullptr;
    webu   = nullptr;
    webua  = nullptr;
}