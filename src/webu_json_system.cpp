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

void cls_webu_json::api_system_temperature()
{
    FILE *temp_file;
    int temp_raw;
    double temp_celsius;

    webua->resp_page = "{";

    temp_file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (temp_file != nullptr) {
        if (fscanf(temp_file, "%d", &temp_raw) == 1) {
            temp_celsius = temp_raw / 1000.0;
            webua->resp_page += "\"celsius\":" + std::to_string(temp_celsius) + ",";
            webua->resp_page += "\"fahrenheit\":" + std::to_string(temp_celsius * 9.0 / 5.0 + 32.0);
        }
        fclose(temp_file);
    } else {
        webua->resp_page += "\"error\":\"Temperature not available\"";
    }

    webua->resp_page += "}";
    webua->resp_type = WEBUI_RESP_JSON;
}

void cls_webu_json::api_system_status()
{
    FILE *file;
    char buffer[256];
    int temp_raw;
    double temp_celsius;
    unsigned long uptime_sec, mem_total, mem_free, mem_available;
    struct statvfs fs_stat;

    webua->resp_page = "{";

    /* CPU Temperature */
    file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (file != nullptr) {
        if (fscanf(file, "%d", &temp_raw) == 1) {
            temp_celsius = temp_raw / 1000.0;
            webua->resp_page += "\"temperature\":{";
            webua->resp_page += "\"celsius\":" + std::to_string(temp_celsius) + ",";
            webua->resp_page += "\"fahrenheit\":" + std::to_string(temp_celsius * 9.0 / 5.0 + 32.0);
            webua->resp_page += "},";
        }
        fclose(file);
    }

    /* System Uptime */
    file = fopen("/proc/uptime", "r");
    if (file != nullptr) {
        if (fscanf(file, "%lu", &uptime_sec) == 1) {
            webua->resp_page += "\"uptime\":{";
            webua->resp_page += "\"seconds\":" + std::to_string(uptime_sec) + ",";
            webua->resp_page += "\"days\":" + std::to_string(uptime_sec / 86400) + ",";
            webua->resp_page += "\"hours\":" + std::to_string((uptime_sec % 86400) / 3600);
            webua->resp_page += "},";
        }
        fclose(file);
    }

    /* Memory Information */
    file = fopen("/proc/meminfo", "r");
    if (file != nullptr) {
        mem_total = mem_free = mem_available = 0;
        while (fgets(buffer, sizeof(buffer), file)) {
            if (sscanf(buffer, "MemTotal: %lu kB", &mem_total) == 1) continue;
            if (sscanf(buffer, "MemFree: %lu kB", &mem_free) == 1) continue;
            if (sscanf(buffer, "MemAvailable: %lu kB", &mem_available) == 1) break;
        }
        fclose(file);

        if (mem_total > 0) {
            unsigned long mem_used = mem_total - mem_available;
            double mem_percent = (double)mem_used / static_cast<double>(mem_total) * 100.0;
            webua->resp_page += "\"memory\":{";
            webua->resp_page += "\"total\":" + std::to_string(mem_total * 1024) + ",";
            webua->resp_page += "\"used\":" + std::to_string(mem_used * 1024) + ",";
            webua->resp_page += "\"free\":" + std::to_string(mem_free * 1024) + ",";
            webua->resp_page += "\"available\":" + std::to_string(mem_available * 1024) + ",";
            webua->resp_page += "\"percent\":" + std::to_string(mem_percent);
            webua->resp_page += "},";
        }
    }

    /* Disk Usage (root filesystem) */
    if (statvfs("/", &fs_stat) == 0) {
        unsigned long long total_bytes = (unsigned long long)fs_stat.f_blocks * fs_stat.f_frsize;
        unsigned long long free_bytes = (unsigned long long)fs_stat.f_bfree * fs_stat.f_frsize;
        unsigned long long avail_bytes = (unsigned long long)fs_stat.f_bavail * fs_stat.f_frsize;
        unsigned long long used_bytes = total_bytes - free_bytes;
        double disk_percent = (double)used_bytes / static_cast<double>(total_bytes) * 100.0;

        webua->resp_page += "\"disk\":{";
        webua->resp_page += "\"total\":" + std::to_string(total_bytes) + ",";
        webua->resp_page += "\"used\":" + std::to_string(used_bytes) + ",";
        webua->resp_page += "\"free\":" + std::to_string(free_bytes) + ",";
        webua->resp_page += "\"available\":" + std::to_string(avail_bytes) + ",";
        webua->resp_page += "\"percent\":" + std::to_string(disk_percent);
        webua->resp_page += "},";
    }

    /* Device Model (Raspberry Pi) */
    file = fopen("/proc/device-tree/model", "r");
    if (file != nullptr) {
        if (fgets(buffer, sizeof(buffer), file)) {
            /* Remove trailing newline/null */
            size_t len = strlen(buffer);
            while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\0' || buffer[len-1] == '\r')) {
                buffer[--len] = '\0';
            }
            webua->resp_page += "\"device_model\":\"" + escstr(buffer) + "\",";

            /* Detect Pi generation */
            if (strstr(buffer, "Pi 5") != nullptr) {
                webua->resp_page += "\"pi_generation\":5,";
            } else if (strstr(buffer, "Pi 4") != nullptr) {
                webua->resp_page += "\"pi_generation\":4,";
            } else if (strstr(buffer, "Pi 3") != nullptr) {
                webua->resp_page += "\"pi_generation\":3,";
            } else {
                webua->resp_page += "\"pi_generation\":0,";
            }
        }
        fclose(file);
    }

    /* Hardware Encoder Availability */
    {
        const AVCodec *codec_check;
        webua->resp_page += "\"hardware_encoders\":{";

        /* Check for V4L2 M2M H.264 encoder (Pi 4 only) */
        codec_check = avcodec_find_encoder_by_name("h264_v4l2m2m");
        webua->resp_page += "\"h264_v4l2m2m\":" + std::string(codec_check ? "true" : "false");

        webua->resp_page += "},";
    }

    /* Webcontrol Actions Status */
    webua->resp_page += "\"actions\":{";

    bool service_enabled = false;
    bool power_enabled = false;
    for (int indx = 0; indx < webu->wb_actions->params_cnt; indx++) {
        if (webu->wb_actions->params_array[indx].param_name == "service" &&
            webu->wb_actions->params_array[indx].param_value == "on") {
            service_enabled = true;
        }
        if (webu->wb_actions->params_array[indx].param_name == "power" &&
            webu->wb_actions->params_array[indx].param_value == "on") {
            power_enabled = true;
        }
    }

    webua->resp_page += "\"service\":" + std::string(service_enabled ? "true" : "false");
    webua->resp_page += ",\"power\":" + std::string(power_enabled ? "true" : "false");
    webua->resp_page += "},";

    /* Motion Version */
    webua->resp_page += "\"version\":\"" + escstr(VERSION) + "\"";

    /* Camera Status (includes FPS for each camera) */
    webua->resp_page += ",\"status\":{";
    webua->resp_page += "\"count\":" + std::to_string(app->cam_cnt);
    for (int indx_cam = 0; indx_cam < app->cam_cnt; indx_cam++) {
        webua->resp_page += ",\"cam" +
            std::to_string(app->cam_list[indx_cam]->cfg->device_id) + "\":";
        status_vars(indx_cam);
    }
    webua->resp_page += "}";

    webua->resp_page += "}";
    webua->resp_type = WEBUI_RESP_JSON;
}

void cls_webu_json::api_system_reboot()
{
    webua->resp_type = WEBUI_RESP_JSON;

    /* Validate CSRF token (supports both session and global tokens) */
    const char* csrf_token = MHD_lookup_connection_value(
        webua->connection, MHD_HEADER_KIND, "X-CSRF-Token");
    if (!webu->csrf_validate_request(csrf_token ? std::string(csrf_token) : "", webua->session_token)) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("CSRF token validation failed for reboot from %s"), webua->clientip.c_str());
        webua->resp_page = "{\"error\":\"CSRF validation failed\"}";
        return;
    }

    /* Check if power control is enabled via webcontrol_actions */
    bool power_enabled = false;
    for (int indx = 0; indx < webu->wb_actions->params_cnt; indx++) {
        if (webu->wb_actions->params_array[indx].param_name == "power") {
            if (webu->wb_actions->params_array[indx].param_value == "on") {
                power_enabled = true;
            }
            break;
        }
    }

    if (!power_enabled) {
        MOTION_LOG(INF, TYPE_ALL, NO_ERRNO,
            "Reboot request denied - power control disabled (from %s)", webua->clientip.c_str());
        webua->resp_page = "{\"error\":\"Power control is disabled\"}";
        return;
    }

    /* Log the reboot request */
    MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO,
        "System reboot requested by %s", webua->clientip.c_str());

    /* Schedule reboot with 2-second delay to allow HTTP response to complete */
    std::thread([]() {
        sleep(2);
        /* Try reboot commands in sequence (like MotionEye) */
        if (system("sudo /sbin/reboot") != 0) {
            if (system("sudo /sbin/shutdown -r now") != 0) {
                if (system("sudo /usr/bin/systemctl reboot") != 0) {
                    (void)system("sudo /sbin/init 6");
                }
            }
        }
    }).detach();

    webua->resp_page = "{\"success\":true,\"operation\":\"reboot\",\"message\":\"System will reboot in 2 seconds\"}";
}

void cls_webu_json::api_system_shutdown()
{
    webua->resp_type = WEBUI_RESP_JSON;

    /* Validate CSRF token (supports both session and global tokens) */
    const char* csrf_token = MHD_lookup_connection_value(
        webua->connection, MHD_HEADER_KIND, "X-CSRF-Token");
    if (!webu->csrf_validate_request(csrf_token ? std::string(csrf_token) : "", webua->session_token)) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("CSRF token validation failed for shutdown from %s"), webua->clientip.c_str());
        webua->resp_page = "{\"error\":\"CSRF validation failed\"}";
        return;
    }

    /* Check if power control is enabled via webcontrol_actions */
    bool power_enabled = false;
    for (int indx = 0; indx < webu->wb_actions->params_cnt; indx++) {
        if (webu->wb_actions->params_array[indx].param_name == "power") {
            if (webu->wb_actions->params_array[indx].param_value == "on") {
                power_enabled = true;
            }
            break;
        }
    }

    if (!power_enabled) {
        MOTION_LOG(INF, TYPE_ALL, NO_ERRNO,
            "Shutdown request denied - power control disabled (from %s)", webua->clientip.c_str());
        webua->resp_page = "{\"error\":\"Power control is disabled\"}";
        return;
    }

    /* Log the shutdown request */
    MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO,
        "System shutdown requested by %s", webua->clientip.c_str());

    /* Schedule shutdown with 2-second delay to allow HTTP response to complete */
    std::thread([]() {
        sleep(2);
        /* Try shutdown commands in sequence (like MotionEye) */
        if (system("sudo /sbin/poweroff") != 0) {
            if (system("sudo /sbin/shutdown -h now") != 0) {
                if (system("sudo /usr/bin/systemctl poweroff") != 0) {
                    (void)system("sudo /sbin/init 0");
                }
            }
        }
    }).detach();

    webua->resp_page = "{\"success\":true,\"operation\":\"shutdown\",\"message\":\"System will shut down in 2 seconds\"}";
}

void cls_webu_json::api_system_service_restart()
{
    webua->resp_type = WEBUI_RESP_JSON;

    /* Validate CSRF token (supports both session and global tokens) */
    const char* csrf_token = MHD_lookup_connection_value(
        webua->connection, MHD_HEADER_KIND, "X-CSRF-Token");
    if (!webu->csrf_validate_request(csrf_token ? std::string(csrf_token) : "", webua->session_token)) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("CSRF token validation failed for service restart from %s"), webua->clientip.c_str());
        webua->resp_page = "{\"error\":\"CSRF validation failed\"}";
        return;
    }

    /* Check if service control is enabled via webcontrol_actions */
    bool service_enabled = false;
    for (int indx = 0; indx < webu->wb_actions->params_cnt; indx++) {
        if (webu->wb_actions->params_array[indx].param_name == "service") {
            if (webu->wb_actions->params_array[indx].param_value == "on") {
                service_enabled = true;
            }
            break;
        }
    }

    if (!service_enabled) {
        MOTION_LOG(INF, TYPE_ALL, NO_ERRNO,
            "Service restart request denied - service control disabled (from %s)", webua->clientip.c_str());
        webua->resp_page = "{\"error\":\"Service control is disabled\"}";
        return;
    }

    /* Log the restart request */
    MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO,
        "Motion service restart requested by %s", webua->clientip.c_str());

    /* Schedule restart with 2-second delay to allow HTTP response to complete */
    std::thread([]() {
        sleep(2);
        (void)system("sudo /usr/bin/systemctl restart motion");
    }).detach();

    webua->resp_page = "{\"success\":true,\"operation\":\"service-restart\",\"message\":\"Motion service will restart in 2 seconds\"}";
}
