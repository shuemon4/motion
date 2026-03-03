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
 * webu_json_auth.cpp - Authentication and Session Management API
 *
 * Implements JSON REST API endpoints for user authentication: login with
 * bcrypt password validation, logout with session destruction, session
 * token management with CSRF protection, and auth status checks supporting
 * both session-based and HTTP Basic/Digest authentication fallback.
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

void cls_webu_json::api_auth_me()
{
    webua->resp_page = "{";

    /* Check if authentication is configured */
    if (app->cfg->webcontrol_authentication != "") {
        webua->resp_page += "\"authenticated\":true,";
        webua->resp_page += "\"auth_method\":\"digest\",";

        /* Include role from HTTP Basic/Digest auth */
        if (webua->auth_role != "") {
            webua->resp_page += "\"role\":\"" + webua->auth_role + "\"";
        } else {
            /* Default to admin if role not determined */
            webua->resp_page += "\"role\":\"admin\"";
        }
    } else {
        webua->resp_page += "\"authenticated\":false";
    }

    webua->resp_page += "}";
    webua->resp_type = WEBUI_RESP_JSON;
}

void cls_webu_json::api_auth_login()
{
    webua->resp_type = WEBUI_RESP_JSON;

    /* Only accept POST */
    if (webua->get_method() != WEBUI_METHOD_POST) {
        webua->resp_page = "{\"error\":\"Method not allowed\"}";
        webua->resp_code = 405;
        return;
    }

    /* Parse JSON body for username/password */
    JsonParser parser;
    if (!parser.parse(webua->raw_body)) {
        webua->resp_page = "{\"error\":\"Invalid JSON\"}";
        webua->resp_code = 400;
        return;
    }

    std::string username = parser.getString("username");
    std::string password = parser.getString("password");

    if (username.empty() || password.empty()) {
        webua->resp_page = "{\"error\":\"Missing username or password\"}";
        webua->resp_code = 400;
        return;
    }

    /* Validate credentials against config */
    std::string role = "";

    /* Check admin credentials */
    std::string admin_auth = app->cfg->webcontrol_authentication;
    if (!admin_auth.empty()) {
        size_t colon_pos = admin_auth.find(':');
        if (colon_pos != std::string::npos) {
            std::string admin_user = admin_auth.substr(0, colon_pos);
            std::string stored_value = admin_auth.substr(colon_pos + 1);

            /* Verify username matches */
            if (username == admin_user) {
                /* Check if stored value is bcrypt hash or plaintext */
                if (cls_webu_auth::is_bcrypt_hash(stored_value)) {
                    /* Bcrypt hash - verify password */
                    if (cls_webu_auth::verify_password(password, stored_value)) {
                        role = "admin";
                    }
                } else {
                    /* Plaintext password (for initial setup compatibility) */
                    if (password == stored_value) {
                        role = "admin";

                        /* Log warning about plaintext password */
                        MOTION_LOG(WRN, TYPE_ALL, NO_ERRNO,
                            "Plaintext admin password detected - "
                            "run motion-setup to hash credentials");
                    }
                }
            }
        }
    }

    /* Check user credentials if admin didn't match */
    if (role.empty()) {
        std::string user_auth = app->cfg->webcontrol_user_authentication;
        if (!user_auth.empty()) {
            size_t colon_pos = user_auth.find(':');
            if (colon_pos != std::string::npos) {
                std::string user_user = user_auth.substr(0, colon_pos);
                std::string stored_value = user_auth.substr(colon_pos + 1);

                /* Verify username matches */
                if (username == user_user) {
                    /* Check if stored value is bcrypt hash or plaintext */
                    if (cls_webu_auth::is_bcrypt_hash(stored_value)) {
                        /* Bcrypt hash - verify password */
                        if (cls_webu_auth::verify_password(password, stored_value)) {
                            role = "user";
                        }
                    } else {
                        /* Plaintext password (for initial setup compatibility) */
                        if (password == stored_value) {
                            role = "user";

                            /* Log warning about plaintext password */
                            MOTION_LOG(WRN, TYPE_ALL, NO_ERRNO,
                                "Plaintext viewer password detected - "
                                "run motion-setup to hash credentials");
                        }
                    }
                }
            }
        }
    }

    if (role.empty()) {
        /* Log failed attempt for rate limiting */
        webua->failauth_log(true, username);

        webua->resp_page = "{\"error\":\"Invalid credentials\"}";
        webua->resp_code = 401;
        return;
    }

    /* Create session */
    std::string session_token = webu->session_create(role, webua->clientip);
    std::string csrf_token = webu->session_get_csrf(session_token);

    /* Set session cookie for browser stream authentication */
    webua->cookie_header = "motion_session=" + session_token +
        "; HttpOnly; SameSite=Strict; Path=/; Max-Age=" +
        std::to_string(app->cfg->webcontrol_session_timeout);
    if (app->cfg->webcontrol_tls) {
        webua->cookie_header += "; Secure";
    }

    /* Return session info */
    webua->resp_page = "{";
    webua->resp_page += "\"session_token\":\"" + session_token + "\",";
    webua->resp_page += "\"csrf_token\":\"" + csrf_token + "\",";
    webua->resp_page += "\"role\":\"" + role + "\",";
    webua->resp_page += "\"expires_in\":" + std::to_string(app->cfg->webcontrol_session_timeout);
    webua->resp_page += "}";
}

void cls_webu_json::api_auth_logout()
{
    webua->resp_type = WEBUI_RESP_JSON;

    if (webua->get_method() != WEBUI_METHOD_POST) {
        webua->resp_page = "{\"error\":\"Method not allowed\"}";
        webua->resp_code = 405;
        return;
    }

    /* Get session token from header */
    std::string session_token = webua->session_token;

    if (!session_token.empty()) {
        webu->session_destroy(session_token);
    }

    /* Clear session cookie */
    webua->cookie_header = "motion_session=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0";
    if (app->cfg->webcontrol_tls) {
        webua->cookie_header += "; Secure";
    }

    webua->resp_page = "{\"success\":true}";
}

void cls_webu_json::api_auth_status()
{
    webua->resp_type = WEBUI_RESP_JSON;
    webua->resp_page = "{";

    /* Check if authentication is configured */
    bool auth_required = (app->cfg->webcontrol_authentication != "");

    webua->resp_page += "\"auth_required\":" + std::string(auth_required ? "true" : "false");

    if (!auth_required) {
        /* No auth configured - full access with pseudo-session for CSRF protection */
        /* Create or reuse session for CSRF token even when auth not required */
        if (webua->session_token.empty()) {
            /* No session yet - create pseudo-session for CSRF */
            std::string new_token = webu->session_create("admin", webua->clientip);
            /* Set cookie for stream auth even when auth not configured */
            webua->cookie_header = "motion_session=" + new_token +
                "; HttpOnly; SameSite=Strict; Path=/; Max-Age=" +
                std::to_string(app->cfg->webcontrol_session_timeout);
            if (app->cfg->webcontrol_tls) {
                webua->cookie_header += "; Secure";
            }
            webua->resp_page += ",\"authenticated\":true";
            webua->resp_page += ",\"role\":\"admin\"";
            webua->resp_page += ",\"session_token\":\"" + new_token + "\"";
            webua->resp_page += ",\"csrf_token\":\"" + webu->session_get_csrf(new_token) + "\"";
        } else {
            /* Reuse existing session */
            std::string role = webu->session_validate(webua->session_token, webua->clientip);
            if (!role.empty()) {
                webua->resp_page += ",\"authenticated\":true";
                webua->resp_page += ",\"role\":\"" + role + "\"";
                webua->resp_page += ",\"csrf_token\":\"" + webu->session_get_csrf(webua->session_token) + "\"";
            } else {
                /* Session expired - create new one */
                std::string new_token = webu->session_create("admin", webua->clientip);
                /* Set cookie for stream auth even when auth not configured */
                webua->cookie_header = "motion_session=" + new_token +
                    "; HttpOnly; SameSite=Strict; Path=/; Max-Age=" +
                    std::to_string(app->cfg->webcontrol_session_timeout);
                if (app->cfg->webcontrol_tls) {
                    webua->cookie_header += "; Secure";
                }
                webua->resp_page += ",\"authenticated\":true";
                webua->resp_page += ",\"role\":\"admin\"";
                webua->resp_page += ",\"session_token\":\"" + new_token + "\"";
                webua->resp_page += ",\"csrf_token\":\"" + webu->session_get_csrf(new_token) + "\"";
            }
        }
    } else if (!webua->session_token.empty()) {
        /* Session token provided - validate it */
        std::string role = webu->session_validate(
            webua->session_token, webua->clientip);

        if (!role.empty()) {
            webua->resp_page += ",\"authenticated\":true";
            webua->resp_page += ",\"role\":\"" + role + "\"";
            webua->resp_page += ",\"csrf_token\":\"" +
                webu->session_get_csrf(webua->session_token) + "\"";
        } else {
            webua->resp_page += ",\"authenticated\":false";
        }
    } else if (!webua->auth_role.empty()) {
        /* HTTP Basic/Digest auth for external API clients (curl, scripts, etc.) */
        webua->resp_page += ",\"authenticated\":true";
        webua->resp_page += ",\"role\":\"" + webua->auth_role + "\"";
        webua->resp_page += ",\"csrf_token\":\"" + webu->csrf_token + "\"";
    } else {
        /* Auth required but no credentials */
        webua->resp_page += ",\"authenticated\":false";
    }

    webua->resp_page += "}";
}
