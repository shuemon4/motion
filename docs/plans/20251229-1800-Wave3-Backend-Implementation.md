# Wave 3 Backend Implementation Plan

**Date**: 2025-12-29 18:00
**Status**: Ready for Implementation
**Priority**: High - Essential for Feature Parity
**Related**: `docs/scratchpads/20251229-1700-Wave3-Backend-Requirements.md`

## Executive Summary

This plan implements the C++ backend APIs required for Wave 3 feature parity:
1. **Wave 3A**: Mask Editor API - CRUD operations for motion and privacy masks
2. **Wave 3B**: Notification System - Shell script integration (no backend changes needed)

**Design Decisions Made:**
- Masks auto-generate in `target_dir` with predictable names (e.g., `cam1_motion.pgm`)
- Notification system uses existing Motion event hooks - no new backend parameters

---

## Wave 3A: Mask Editor Backend

### Overview

Implement three new API endpoints for mask management:
- `GET /{camId}/api/mask/{type}` - Retrieve mask info
- `POST /{camId}/api/mask/{type}` - Create/update mask from polygon data
- `DELETE /{camId}/api/mask/{type}` - Remove mask

**Type values**: `motion` or `privacy`

### Architecture

```
Frontend                    Backend (C++)
┌───────────────┐           ┌─────────────────┐
│ MaskEditor.tsx│           │ webu_json.cpp   │
│ - Canvas draw │ ─POST───► │ api_mask_post() │
│ - Polygons    │           │ - Parse JSON    │
│               │           │ - Fill polygons │
│               │           │ - Write PGM     │
└───────────────┘           └─────────────────┘
                                   │
                                   ▼
                            ┌─────────────────┐
                            │ target_dir/     │
                            │ cam1_motion.pgm │
                            └─────────────────┘
```

### Implementation Steps

#### Step 1: Add Method Declarations to `webu_json.hpp`

**File**: `src/webu_json.hpp`

Add after line 37:
```cpp
void api_mask_get();     // GET /{camId}/api/mask/{type}
void api_mask_post();    // POST /{camId}/api/mask/{type}
void api_mask_delete();  // DELETE /{camId}/api/mask/{type}
```

#### Step 2: Add Routing in `webu_ans.cpp`

**File**: `src/webu_ans.cpp`

In `answer_get()` function (around line 1018), after the `uri_cmd2 == "config"` block:

```cpp
} else if (uri_cmd2 == "mask" && uri_cmd3 != "") {
    webu_json->api_mask_get();
    mhd_send();
```

In `answer_main()` for POST (where POST is handled):

Add mask POST handling in the appropriate routing section:
```cpp
// In the POST handling section, add routing for mask
if (uri_cmd1 == "api" && uri_cmd2 == "mask" && uri_cmd3 != "") {
    if (webu_json == nullptr) {
        webu_json = new cls_webu_json(this);
    }
    webu_json->api_mask_post();
    mhd_send();
}
```

In `answer_delete()`:
```cpp
} else if (uri_cmd1 == "api" && uri_cmd2 == "mask" && uri_cmd3 != "") {
    if (webu_json == nullptr) {
        webu_json = new cls_webu_json(this);
    }
    webu_json->api_mask_delete();
    mhd_send();
}
```

#### Step 3: Implement API Methods in `webu_json.cpp`

**File**: `src/webu_json.cpp`

##### Helper: Polygon Fill Function (Scanline Algorithm)

Add at top of file (after includes):

```cpp
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
```

##### Helper: Build Mask Path

```cpp
/* Generate auto-path for mask file in target_dir */
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
```

##### GET /api/mask/{type}

```cpp
/*
 * React UI API: Get mask information
 * GET /{camId}/api/mask/{type}
 * type = "motion" or "privacy"
 */
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
```

##### POST /api/mask/{type}

```cpp
/*
 * React UI API: Save mask from polygon data
 * POST /{camId}/api/mask/{type}
 * Request body: {"polygons":[[{x,y},...]], "width":W, "height":H, "invert":bool}
 */
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

    /* Validate CSRF */
    const char* csrf_token = MHD_lookup_connection_value(
        webua->connection, MHD_HEADER_KIND, "X-CSRF-Token");
    if (csrf_token == nullptr || !webu->csrf_validate(std::string(csrf_token))) {
        webua->resp_page = "{\"error\":\"CSRF validation failed\"}";
        return;
    }

    /* Parse JSON request body */
    /* Note: Need extended JSON parser for arrays - implement minimal parser */
    std::string body = webua->raw_body;

    /* Extract dimensions */
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
```

##### DELETE /api/mask/{type}

```cpp
/*
 * React UI API: Delete mask file
 * DELETE /{camId}/api/mask/{type}
 */
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

    /* Validate CSRF */
    const char* csrf_token = MHD_lookup_connection_value(
        webua->connection, MHD_HEADER_KIND, "X-CSRF-Token");
    if (csrf_token == nullptr || !webu->csrf_validate(std::string(csrf_token))) {
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
```

#### Step 4: Add Required Includes

**File**: `src/webu_json.cpp`

Add at top if not present:
```cpp
#include <algorithm>  // for std::sort
#include <vector>     // for std::vector
```

---

## Wave 3B: Notification System

### Overview

The notification system uses Motion's existing event hooks. No backend C++ changes required.

### Shell Script Implementation

**File**: `data/scripts/motion-notify.sh`

```bash
#!/bin/bash
# Motion Notification Dispatcher
# Called by Motion via on_event_* hooks
#
# Usage: motion-notify.sh <event_type> <camera_id> [file_path]
# Events: start, end, picture, movie_start, movie_end
#
# Configuration: /etc/motion/notify.conf
# Environment variables can also be used.

SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
CONFIG_FILE="${MOTION_NOTIFY_CONF:-/etc/motion/notify.conf}"

# Load configuration
if [[ -f "$CONFIG_FILE" ]]; then
    source "$CONFIG_FILE"
fi

EVENT_TYPE="${1:-unknown}"
CAMERA_ID="${2:-0}"
FILE_PATH="${3:-}"
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')

# Logging helper
log() {
    logger -t "motion-notify" "$@"
}

# Email notification (uses sendmail/msmtp)
send_email() {
    if [[ "$NOTIFY_EMAIL_ENABLED" != "true" ]]; then
        return
    fi

    local subject="Motion Alert - Camera $CAMERA_ID"
    local body="Event: $EVENT_TYPE\nTime: $TIMESTAMP\nCamera: $CAMERA_ID"

    if [[ -n "$FILE_PATH" ]]; then
        body="$body\nFile: $FILE_PATH"
    fi

    if command -v msmtp &> /dev/null; then
        echo -e "Subject: $subject\n\n$body" | msmtp "$NOTIFY_EMAIL_TO"
    elif command -v sendmail &> /dev/null; then
        echo -e "Subject: $subject\n\n$body" | sendmail "$NOTIFY_EMAIL_TO"
    else
        log "Email not sent - no mail client found"
    fi
}

# Telegram notification
send_telegram() {
    if [[ "$NOTIFY_TELEGRAM_ENABLED" != "true" ]]; then
        return
    fi

    if [[ -z "$NOTIFY_TELEGRAM_TOKEN" || -z "$NOTIFY_TELEGRAM_CHAT" ]]; then
        log "Telegram not configured"
        return
    fi

    local message="🚨 *Motion Detected*%0A"
    message+="Camera: $CAMERA_ID%0A"
    message+="Event: $EVENT_TYPE%0A"
    message+="Time: $TIMESTAMP"

    curl -s -X POST \
        "https://api.telegram.org/bot${NOTIFY_TELEGRAM_TOKEN}/sendMessage" \
        -d "chat_id=${NOTIFY_TELEGRAM_CHAT}" \
        -d "text=${message}" \
        -d "parse_mode=Markdown" \
        > /dev/null 2>&1 &
}

# Webhook notification
send_webhook() {
    if [[ "$NOTIFY_WEBHOOK_ENABLED" != "true" ]]; then
        return
    fi

    if [[ -z "$NOTIFY_WEBHOOK_URL" ]]; then
        log "Webhook not configured"
        return
    fi

    local payload=$(cat <<EOF
{
    "event": "$EVENT_TYPE",
    "camera_id": "$CAMERA_ID",
    "timestamp": "$TIMESTAMP",
    "file_path": "$FILE_PATH"
}
EOF
)

    curl -s -X POST \
        -H "Content-Type: application/json" \
        -d "$payload" \
        "$NOTIFY_WEBHOOK_URL" \
        > /dev/null 2>&1 &
}

# Main dispatch
main() {
    log "Event: $EVENT_TYPE, Camera: $CAMERA_ID, File: $FILE_PATH"

    case "$EVENT_TYPE" in
        start)
            send_email
            send_telegram
            send_webhook
            ;;
        picture)
            # Optionally send with attachment
            send_webhook
            ;;
        movie_end)
            send_webhook
            ;;
        *)
            send_webhook
            ;;
    esac
}

main
```

**File**: `data/scripts/notify.conf.example`

```bash
# Motion Notification Configuration
# Copy to /etc/motion/notify.conf

# Email Settings
NOTIFY_EMAIL_ENABLED=false
NOTIFY_EMAIL_TO="user@example.com"
# Configure /etc/msmtprc for SMTP settings

# Telegram Settings
NOTIFY_TELEGRAM_ENABLED=false
NOTIFY_TELEGRAM_TOKEN=""
NOTIFY_TELEGRAM_CHAT=""

# Webhook Settings
NOTIFY_WEBHOOK_ENABLED=false
NOTIFY_WEBHOOK_URL=""
```

### Motion Configuration

Configure these parameters via the UI or motion.conf:

```
on_event_start /usr/local/share/motion/motion-notify.sh start %t
on_event_end /usr/local/share/motion/motion-notify.sh end %t
on_picture_save /usr/local/share/motion/motion-notify.sh picture %t %f
on_movie_end /usr/local/share/motion/motion-notify.sh movie_end %t %f
```

### Installation

Add to `Makefile.am` or installation script:
```bash
install -d $(DESTDIR)$(datadir)/motion
install -m 755 data/scripts/motion-notify.sh $(DESTDIR)$(datadir)/motion/
install -m 644 data/scripts/notify.conf.example $(DESTDIR)$(sysconfdir)/motion/
```

---

## CPU Efficiency Analysis

### Mask Operations

| Operation | CPU Impact | Notes |
|-----------|-----------|-------|
| Polygon fill | O(h * e) | h=height, e=edges; uses integer math only |
| PGM write | O(w * h) | Single buffered write, no encoding |
| File I/O | Minimal | fwrite uses OS buffering |
| Total | Low | One-time operation on save |

### Notification Scripts

| Operation | CPU Impact | Notes |
|-----------|-----------|-------|
| Script launch | fork() overhead | Only during events |
| curl calls | Async (backgrounded) | Non-blocking with & |
| Total | Negligible | Events are infrequent |

**Optimization Applied:**
- Scanline fill uses integer arithmetic only (no floating point)
- Single memory allocation for bitmap (no reallocation)
- Direct fwrite without intermediate buffers
- Notification curls run in background (&)

---

## Security Considerations

### Mask API

1. **CSRF Protection**: All POST/DELETE require valid X-CSRF-Token
2. **Path Traversal**: Reject paths containing ".."
3. **Permission Level**: Uses existing webcontrol authentication
4. **File Permissions**: Created with 0644 in target_dir

### Notification System

1. **Credentials Storage**: notify.conf should be 0600 root-only
2. **No Logging of Secrets**: Script doesn't log tokens/passwords
3. **Command Injection**: Parameters are quoted in script
4. **Background Execution**: Prevents motion from blocking

---

## Testing Procedure

### Mask API Testing

```bash
# 1. Get mask status (should show exists: false)
curl -X GET http://192.168.1.176:7999/1/api/mask/motion

# 2. Create mask (requires CSRF token from /0/api/config)
curl -X POST http://192.168.1.176:7999/1/api/mask/motion \
  -H "Content-Type: application/json" \
  -H "X-CSRF-Token: <token>" \
  -d '{"polygons":[[[100,100],[500,100],[500,400],[100,400]]],"width":1920,"height":1080}'

# 3. Verify file created
ssh admin@192.168.1.176 "ls -la /var/lib/motion/cam1_motion.pgm"

# 4. Delete mask
curl -X DELETE http://192.168.1.176:7999/1/api/mask/motion \
  -H "X-CSRF-Token: <token>"
```

### Notification Testing

```bash
# 1. Configure notify.conf
ssh admin@192.168.1.176 "cat > /etc/motion/notify.conf << 'EOF'
NOTIFY_WEBHOOK_ENABLED=true
NOTIFY_WEBHOOK_URL="https://webhook.site/<your-id>"
EOF"

# 2. Configure Motion
# Set on_event_start via API or config file

# 3. Trigger motion detection
# Wave hand in front of camera

# 4. Check webhook.site for received payload
```

---

## File Changes Summary

### Files to Create

| File | Type | Purpose |
|------|------|---------|
| `data/scripts/motion-notify.sh` | Bash | Notification dispatcher |
| `data/scripts/notify.conf.example` | Config | Example configuration |

### Files to Modify

| File | Changes |
|------|---------|
| `src/webu_json.hpp` | Add 3 method declarations |
| `src/webu_json.cpp` | Add helper functions + 3 API methods (~200 LOC) |
| `src/webu_ans.cpp` | Add routing for mask endpoints (~30 LOC) |

---

## Implementation Order

1. **webu_json.hpp**: Add declarations (5 min)
2. **webu_json.cpp**: Add helpers + api_mask_get (30 min)
3. **webu_json.cpp**: Add api_mask_post (45 min)
4. **webu_json.cpp**: Add api_mask_delete (15 min)
5. **webu_ans.cpp**: Add routing (15 min)
6. **Build & Test**: Compile, deploy, verify (30 min)
7. **motion-notify.sh**: Create script (20 min)
8. **Integration test**: End-to-end verification (30 min)

**Total estimated time**: 3-4 hours

---

## Rollback Plan

If issues arise:

1. **Revert code changes**: `git checkout src/webu*.cpp src/webu*.hpp`
2. **Remove mask files**: `rm -f /var/lib/motion/cam*_*.pgm`
3. **Clear config**: Set mask_file and mask_privacy to empty

---

## Success Criteria

- [ ] GET /api/mask/{type} returns mask info
- [ ] POST /api/mask/{type} creates valid PGM
- [ ] DELETE /api/mask/{type} removes mask
- [ ] Motion loads mask on restart
- [ ] Privacy mask blanks detected regions
- [ ] Motion mask excludes regions from detection
- [ ] Notification script executes on events
- [ ] CPU usage increase during mask save < 1%
- [ ] No memory leaks after mask operations
