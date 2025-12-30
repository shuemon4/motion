# Wave 3 Backend Implementation Requirements

**Date**: 2025-12-29 17:00
**Status**: Deferred - Requires Backend C++ Development
**Related**: `docs/plans/20251229-1600-Feature-Parity-Wave2-4-Handoff.md`

## Overview

Wave 3 requires backend C++ changes that were not implemented in this session. The frontend components for Waves 2 and 4 have been completed successfully. This document captures the backend requirements for Wave 3 implementation.

## Wave 3A: Mask Editor Backend

### Required API Endpoints

Add to `src/webu_json.hpp`:
```cpp
void api_mask_get();    // GET /{camId}/api/mask/{type}
void api_mask_post();   // POST /{camId}/api/mask/{type}
void api_mask_delete(); // DELETE /{camId}/api/mask/{type}
```

Add to `src/webu_ans.cpp` routing (in the `uri_cmd1 == "api"` section around line 1018):
```cpp
} else if (uri_cmd2 == "mask" && uri_cmd3 != "") {
    if (cnct_method == "GET") {
        webu_json->api_mask_get();
        mhd_send();
    } else if (cnct_method == "POST") {
        webu_json->api_mask_post();
        mhd_send();
    } else if (cnct_method == "DELETE") {
        webu_json->api_mask_delete();
        mhd_send();
    } else {
        bad_request();
    }
```

### GET /api/mask/{type}

**Purpose**: Retrieve current mask file (if exists) for motion or privacy mask

**Implementation**:
1. Parse `type` from `uri_cmd3` ("motion" or "privacy")
2. Get appropriate config parameter:
   - "motion" → `cam->conf->mask_file`
   - "privacy" → `cam->conf->mask_privacy`
3. If file exists, read PGM file and return:
   ```json
   {
     "type": "motion",
     "path": "/path/to/mask.pgm",
     "exists": true,
     "width": 1920,
     "height": 1080,
     "data": "base64-encoded-pgm-data"
   }
   ```
4. If no mask, return:
   ```json
   {
     "type": "motion",
     "exists": false
   }
   ```

### POST /api/mask/{type}

**Purpose**: Save polygon coordinates as PGM mask file

**Request Body**:
```json
{
  "polygons": [
    [
      {"x": 100, "y": 100},
      {"x": 500, "y": 100},
      {"x": 500, "y": 400},
      {"x": 100, "y": 400}
    ]
  ],
  "width": 1920,
  "height": 1080,
  "invert": false
}
```

**Implementation**:
1. Parse JSON request body (use existing `json_parse.hpp` utilities)
2. Create PGM bitmap:
   - Initialize to 0 (black = masked) or 255 (white = detect) based on `invert`
   - For each polygon, fill interior pixels with opposite value
   - Use scanline fill algorithm for efficiency
3. Write PGM file:
   ```cpp
   // PGM P5 format (binary grayscale)
   // Header: "P5\n{width} {height}\n255\n"
   // Data: width*height bytes (0 = masked, 255 = detect)
   ```
4. Update config parameter:
   - Set `mask_file` or `mask_privacy` to saved file path
   - Trigger hot reload if applicable
5. Return success:
   ```json
   {
     "success": true,
     "path": "/path/to/mask.pgm"
   }
   ```

### DELETE /api/mask/{type}

**Purpose**: Remove mask and delete file

**Implementation**:
1. Get current mask path from config
2. Delete file if exists
3. Clear config parameter (set to empty string)
4. Trigger hot reload if applicable
5. Return success:
   ```json
   {
     "success": true
   }
   ```

### Polygon Fill Algorithm

Use scanline fill for CPU efficiency on Pi:

```cpp
struct Point {
    int x, y;
};

void fillPolygon(unsigned char* bitmap, int width, int height,
                const std::vector<Point>& polygon, unsigned char fillValue) {
    // 1. Find bounding box
    int minY = height, maxY = 0;
    for (const auto& pt : polygon) {
        minY = std::min(minY, pt.y);
        maxY = std::max(maxY, pt.y);
    }

    // 2. For each scanline
    for (int y = minY; y <= maxY; y++) {
        std::vector<int> intersections;

        // 3. Find intersections with polygon edges
        for (size_t i = 0; i < polygon.size(); i++) {
            Point p1 = polygon[i];
            Point p2 = polygon[(i + 1) % polygon.size()];

            if ((p1.y <= y && p2.y > y) || (p2.y <= y && p1.y > y)) {
                int x = p1.x + (y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
                intersections.push_back(x);
            }
        }

        // 4. Sort intersections
        std::sort(intersections.begin(), intersections.end());

        // 5. Fill between pairs of intersections
        for (size_t i = 0; i < intersections.size(); i += 2) {
            if (i + 1 < intersections.size()) {
                int xStart = intersections[i];
                int xEnd = intersections[i + 1];
                for (int x = xStart; x <= xEnd; x++) {
                    if (x >= 0 && x < width && y >= 0 && y < height) {
                        bitmap[y * width + x] = fillValue;
                    }
                }
            }
        }
    }
}
```

### CPU Efficiency Considerations

- **Avoid re-encoding**: PGM is binary, no compression needed
- **Minimize allocations**: Reuse bitmap buffer
- **Direct file I/O**: Use `fwrite()` for PGM output
- **No image libraries**: Motion already has PGM support in `picture.cpp`

## Wave 3B: Notification System Backend

### Approach: Shell Script Integration

Motion already supports event hooks via:
- `on_event_start`
- `on_event_end`
- `on_picture_save`
- `on_movie_start`
- `on_movie_end`

**No backend changes required** - use existing hook system.

### Shell Script: `data/scripts/motion-notify.sh`

Create notification script that reads config from environment or file:

```bash
#!/bin/bash
# Motion notification dispatcher
# Called by Motion via on_event_* hooks

# Read notification config (could be from /etc/motion/notify.conf or env vars)
NOTIFY_EMAIL_ENABLED="${MOTION_NOTIFY_EMAIL:-false}"
NOTIFY_TELEGRAM_ENABLED="${MOTION_NOTIFY_TELEGRAM:-false}"
NOTIFY_WEBHOOK_ENABLED="${MOTION_NOTIFY_WEBHOOK:-false}"

EVENT_TYPE="$1"  # start, end, picture, movie_start, movie_end
CAMERA_ID="$2"
FILE_PATH="$3"   # For picture/movie hooks

# Email notification
if [ "$NOTIFY_EMAIL_ENABLED" = "true" ]; then
    SUBJECT="Motion Detected - Camera $CAMERA_ID"
    BODY="Event: $EVENT_TYPE at $(date)"
    echo "$BODY" | mail -s "$SUBJECT" "$NOTIFY_EMAIL_TO"
fi

# Telegram notification
if [ "$NOTIFY_TELEGRAM_ENABLED" = "true" ]; then
    MESSAGE="🚨 Motion detected on Camera $CAMERA_ID\\n$EVENT_TYPE at $(date)"
    curl -s -X POST "https://api.telegram.org/bot$TELEGRAM_BOT_TOKEN/sendMessage" \
        -d "chat_id=$TELEGRAM_CHAT_ID" \
        -d "text=$MESSAGE"
fi

# Webhook notification
if [ "$NOTIFY_WEBHOOK_ENABLED" = "true" ]; then
    JSON_PAYLOAD=$(cat <<EOF
{
  "event": "$EVENT_TYPE",
  "camera_id": "$CAMERA_ID",
  "timestamp": "$(date -Iseconds)",
  "file_path": "$FILE_PATH"
}
EOF
)
    curl -s -X POST "$NOTIFY_WEBHOOK_URL" \
        -H "Content-Type: application/json" \
        -d "$JSON_PAYLOAD"
fi
```

### Frontend Integration

The NotificationSettings component would:
1. Save notification config to Motion parameters:
   ```
   on_event_start = /path/to/motion-notify.sh start %t
   on_picture_save = /path/to/motion-notify.sh picture %t %f
   ```
2. Store notification credentials in separate config file:
   - `/etc/motion/notify.conf` (read by script)
   - Environment variables (set before Motion starts)

**Security**: Credentials should NOT be stored in Motion config database, as they'd be exposed via `/api/config` endpoint.

### Alternative: Store in Motion Config

Could add new parameters:
```cpp
// In src/conf.cpp config_parms[]
{"notify_email_to", ""},
{"notify_email_from", ""},
{"notify_email_smtp", ""},
{"notify_telegram_token", ""},
{"notify_telegram_chat", ""},
{"notify_webhook_url", ""},
```

Then script reads from Motion's environment or via API.

## Implementation Priority

### High Priority (Essential for Feature Parity)
1. **Wave 3A Mask Editor**: Core MotionEye feature
   - Requires C++ backend work
   - Estimated: 4-6 hours for experienced C++ developer

### Medium Priority (Enhanced Functionality)
2. **Wave 3B Notifications**: Enhanced beyond MotionEye
   - Can be done with shell scripts (no backend changes)
   - Estimated: 2-3 hours for script + frontend testing

### Optional Enhancements
3. **Storage API** (Wave 4A):
   - Disk usage endpoint: `GET /api/storage/usage`
   - Manual cleanup: `POST /api/storage/cleanup`
   - Low CPU impact, useful for user visibility

4. **Upload Scripts** (Wave 4B):
   - Shell script using `rclone`
   - No backend changes needed

## Testing Requirements

### Mask Editor Testing
1. Create polygon mask in UI
2. Verify PGM file created at correct path
3. Check Motion logs for mask loading
4. Verify motion detection respects mask
5. Test privacy mask separately
6. Test mask deletion

### Notification Testing
1. Configure email/Telegram/webhook
2. Trigger motion event
3. Verify notifications sent
4. Check Motion logs for script execution
5. Test credential security (not in logs/API)

## CPU Impact Analysis

### Mask Operations
- **PGM generation**: One-time on save, minimal CPU
- **Polygon fill**: O(n) scanlines, efficient
- **File I/O**: Direct write, no encoding overhead
- **Motion usage**: Motion reads PGM once on startup/reload

### Notification Scripts
- **Triggered**: Only during events (not continuous)
- **External process**: `fork()/exec()` overhead minimal
- **Network I/O**: Async (doesn't block Motion)

Both features have minimal CPU impact suitable for Pi.

## Next Steps

1. **Assign C++ developer** for Wave 3A mask endpoints
2. **Test existing hook system** for Wave 3B notifications
3. **Create notification script** and test with real SMTP/Telegram
4. **Document API** in `docs/api/` once implemented
5. **Update frontend** to call new endpoints when available

## References

- Motion PGM handling: `src/picture.cpp`
- JSON parsing: `src/json_parse.cpp`, `src/json_parse.hpp`
- Event hooks: `src/conf.cpp` (on_event_* parameters)
- Existing mask support: `src/alg.cpp` (reads PGM masks)
