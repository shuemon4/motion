# HANDOFF: React UI Phase 3 - Backend Integration & Testing

**Created**: 2025-12-28
**Status**: Ready for execution
**Repository**: `/Users/tshuey/Documents/GitHub/motion-motioneye/`
**Prerequisites**: Phases 1 & 2 complete

---

## Mission

Complete the C++ backend API extensions and test the full React UI integration with Motion daemon. Deploy and verify on Raspberry Pi 5.

---

## CRITICAL: Documentation Directory Issue

⚠️ **Two plan directories exist with different content:**

- **`doc/plans/react-ui/`** - **NEWER** (Dec 27 23:08-23:10) ← USE THESE
- **`docs/plans/react-ui/`** - Older (Dec 27 21:56-21:58)

**Action Required**: Use files from `doc/plans/react-ui/` as they are more recent. The older `docs/plans/` versions may have outdated information.

---

## What's Already Complete

### ✅ Phase 1: C++ Backend Foundation (Complete)

**Files Modified**:
- `src/parm_structs.hpp` - Added `webcontrol_html_path`, `webcontrol_spa_mode`
- `src/conf.cpp` - Parameter definitions and edit handlers
- `src/conf.hpp` - Reference aliases
- `src/webu_file.cpp` - SPA routing implementation
- `src/webu_file.hpp` - `serve_static_file()` method

**Features**:
- ✅ Configuration parameters for React path and SPA mode
- ✅ Static file serving with MIME types
- ✅ SPA routing (fallback to index.html)
- ✅ Cache headers (1 year for assets, no-cache for index.html)
- ✅ Path traversal security (existing `validate_file_path()`)

**Summary**: `docs/sub-agent-summaries/20251227-react-ui-phase1-backend.md`

### ✅ Phase 2: React Frontend (Complete)

**What Was Built**:
- React 18 + TypeScript + Vite
- Tailwind CSS v3 (dark theme)
- React Router (SPA routing)
- TanStack Query (API state)
- Camera stream component (MJPEG)
- Dashboard with 2-camera grid
- Layout with navigation

**Build Output**: `data/webui/` (81 KB gzipped)
- `index.html` - 0.29 KB
- `assets/*.js` - 80.88 KB (React + app)
- `assets/*.css` - 2.07 KB (Tailwind)

**Summary**: `docs/sub-agent-summaries/20251227-react-ui-phase2-frontend.md`

---

## Phase 3 Tasks

### Task 3.1: Extend JSON API (webu_json.cpp)

**Reference**: `doc/plans/react-ui/02-detailed-execution-20251227.md` (Step 1.3)

Add these endpoints to `src/webu_json.cpp`:

#### 3.1.1: `api_auth_me()`
```cpp
void cls_webu_json::api_auth_me()
{
    webua->resp_page = "{";
    if (webua->authenticated) {
        webua->resp_page += "\"authenticated\":true,";
        webua->resp_page += "\"username\":\"" + escstr(webua->username) + "\"";
    } else {
        webua->resp_page += "\"authenticated\":false";
    }
    webua->resp_page += "}";
    webua->resp_type = WEBUI_RESP_JSON;
}
```

**Endpoint**: `GET /api/auth/me`

#### 3.1.2: `api_media_pictures()`
```cpp
void cls_webu_json::api_media_pictures()
{
    vec_files flst;
    std::string sql;

    if (webua->cam == nullptr) {
        webua->bad_request();
        return;
    }

    sql  = " select * from motion ";
    sql += " where device_id = " + std::to_string(webua->cam->cfg->device_id);
    sql += " and file_type = 1";  /* 1 = snapshot */
    sql += " order by file_dtl desc, file_tml desc";
    sql += " limit 100;";

    app->dbse->filelist_get(sql, flst);

    webua->resp_page = "{\"pictures\":[";
    for (size_t i = 0; i < flst.size(); i++) {
        if (i > 0) webua->resp_page += ",";
        webua->resp_page += "{";
        webua->resp_page += "\"id\":" + std::to_string(flst[i].file_id) + ",";
        webua->resp_page += "\"filename\":\"" + escstr(flst[i].file_nm) + "\",";
        webua->resp_page += "\"path\":\"" + escstr(flst[i].full_nm) + "\",";
        webua->resp_page += "\"date\":\"" + flst[i].file_dtl + "\",";
        webua->resp_page += "\"time\":\"" + flst[i].file_tml + "\",";
        webua->resp_page += "\"size\":" + std::to_string(flst[i].file_sz);
        webua->resp_page += "}";
    }
    webua->resp_page += "]}";
    webua->resp_type = WEBUI_RESP_JSON;
}
```

**Endpoint**: `GET /api/media/pictures/{cam_id}`

#### 3.1.3: `api_system_temperature()`
```cpp
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
```

**Endpoint**: `GET /api/system/temperature`

#### 3.1.4: Add to webu_json.hpp

Add method declarations:
```cpp
void api_auth_me();
void api_media_pictures();
void api_system_temperature();
```

---

### Task 3.2: Add API Routing (webu_ans.cpp)

**Goal**: Route API requests and static file requests

**Find**: The routing logic in `webu_ans.cpp` (look for where URIs are parsed and handlers called)

**Add**: Before existing handlers, add routing for:

#### 3.2.1: API Routes
```cpp
/* API routes - check uri_cmd1 == "api" */
if (webua->uri_cmd1 == "api") {
    if (webua->uri_cmd2 == "auth" && webua->uri_cmd3 == "me") {
        cls_webu_json json(webua);
        json.api_auth_me();
        return MHD_YES;
    }
    if (webua->uri_cmd2 == "media" && webua->uri_cmd3 == "pictures") {
        cls_webu_json json(webua);
        json.api_media_pictures();
        return MHD_YES;
    }
    if (webua->uri_cmd2 == "system" && webua->uri_cmd3 == "temperature") {
        cls_webu_json json(webua);
        json.api_system_temperature();
        return MHD_YES;
    }
}
```

#### 3.2.2: Static File Routes

**Note**: Understand existing routing first. Then add static file serving for paths that don't match Motion's API patterns.

**Pseudocode**:
```cpp
/* After checking for Motion's existing endpoints (/0/config, /1/stream, etc.) */
/* If nothing matched and webcontrol_html_path is configured, serve static */
if (!matched && app->cfg->webcontrol_html_path != "") {
    cls_webu_file file_handler(webua);
    file_handler.serve_static_file();
    return MHD_YES;
}
```

**Important**: Study `webu_ans.cpp` routing structure carefully. Don't break existing functionality.

---

### Task 3.3: Build and Test Backend

#### 3.3.1: Build on Development Mac

```bash
cd /Users/tshuey/Documents/GitHub/motion-motioneye
autoreconf -fiv
./configure --with-libcam --with-sqlite3
make -j4
```

**Expected Issues**:
- JPEG headers may be missing (install: `brew install jpeg`)
- Other dependencies may be missing (see build errors)

**If build fails on Mac**: Document errors, proceed to Pi deployment (Pi has all deps).

#### 3.3.2: Test Configuration Parameters

```bash
./motion -h | grep webcontrol_html_path
./motion -h | grep webcontrol_spa_mode
```

**Expected**: Both parameters appear in help output.

#### 3.3.3: Create Test Configuration

**File**: `motion-test.conf` (in repo root)
```conf
# Test configuration for React UI
daemon off
webcontrol_port 7999
webcontrol_localhost off
webcontrol_html_path ./data/webui
webcontrol_spa_mode on

# Camera 0 (for testing without actual camera)
camera_name Test Camera
width 640
height 480
framerate 5
```

#### 3.3.4: Test Static File Serving

```bash
# Terminal 1: Start Motion
./motion -c motion-test.conf -n

# Terminal 2: Test endpoints
curl http://localhost:7999/ | head -5  # Should return index.html
curl http://localhost:7999/settings | head -5  # SPA routing - also index.html
curl http://localhost:7999/api/system/temperature  # JSON response
```

**Expected**:
- Root `/` serves `index.html`
- `/settings` serves `index.html` (SPA routing)
- `/api/system/temperature` returns JSON

---

### Task 3.4: Deploy to Raspberry Pi

**SSH Access**: `ssh admin@192.168.1.176` (Pi 5)

#### 3.4.1: Sync Code

```bash
rsync -avz --exclude='node_modules' --exclude='.git' \
  /Users/tshuey/Documents/GitHub/motion-motioneye/ \
  admin@192.168.1.176:~/motion-motioneye/
```

#### 3.4.2: Build on Pi

```bash
ssh admin@192.168.1.176 "cd ~/motion-motioneye && \
  autoreconf -fiv && \
  ./configure --with-libcam --with-sqlite3 && \
  make -j4"
```

**Expected**: Clean build (Pi has all dependencies).

#### 3.4.3: Install Web UI

```bash
ssh admin@192.168.1.176 "sudo mkdir -p /usr/share/motion && \
  sudo cp -r ~/motion-motioneye/data/webui /usr/share/motion/"
```

#### 3.4.4: Configure Motion

**File**: `/etc/motion/motion.conf` (on Pi)

Add:
```conf
webcontrol_html_path /usr/share/motion/webui
webcontrol_spa_mode on
```

#### 3.4.5: Restart Motion

```bash
ssh admin@192.168.1.176 "sudo systemctl restart motion"
```

#### 3.4.6: Verify Logs

```bash
ssh admin@192.168.1.176 "sudo journalctl -u motion -n 50 --no-pager"
```

**Look for**:
- Motion started successfully
- Web server listening on port 7999
- No errors about missing files

---

### Task 3.5: Integration Testing

**Access**: http://192.168.1.176:7999/

#### Test Checklist

- [ ] React app loads (dark theme, navigation visible)
- [ ] Dashboard page displays
- [ ] Camera streams display (MJPEG from `/0/stream`, `/1/stream`)
- [ ] Navigation works (Dashboard, Settings, Media)
- [ ] SPA routing works (refresh on `/settings` stays on settings)
- [ ] API endpoints respond:
  - [ ] `/api/auth/me` returns JSON
  - [ ] `/api/system/temperature` returns CPU temp
  - [ ] `/api/media/pictures/0` returns snapshot list
- [ ] Static assets load (check Network tab in DevTools)
- [ ] No 404 errors in browser console
- [ ] Cache headers correct (`Cache-Control` on assets)

---

## Success Criteria

### Functional
- [x] Phase 1 complete (C++ backend)
- [x] Phase 2 complete (React frontend)
- [ ] Phase 3.1 complete (JSON API)
- [ ] Phase 3.2 complete (Routing)
- [ ] Phase 3.3 complete (Build & test)
- [ ] Phase 3.4 complete (Pi deployment)
- [ ] Phase 3.5 complete (Integration tests pass)

### Performance
- [ ] React bundle < 500 KB gzipped (✓ Already 81 KB)
- [ ] Motion RAM usage < 30 MB (measure on Pi)
- [ ] Page load < 2 seconds (on Pi network)
- [ ] Stream latency < 500ms

### Quality
- [ ] No compiler warnings
- [ ] No TypeScript errors
- [ ] All tests pass
- [ ] Documentation complete

---

## Documentation Requirements

### Sub-Agent Summary (REQUIRED)

**File**: `docs/sub-agent-summaries/YYYYMMDD-HHMM-react-ui-phase3-integration.md`

**Content**:
```markdown
# Sub-Agent Summary: React UI Phase 3 - Integration & Testing

**Date**: YYYY-MM-DD HH:MM
**Agent Type**: Implementation
**Task**: Complete backend API, integrate with frontend, deploy to Pi

## Work Performed
- Added JSON API endpoints (api_auth_me, api_media_pictures, api_system_temperature)
- Added routing in webu_ans.cpp
- Built and tested on development Mac
- Deployed to Raspberry Pi 5
- Integration testing results

## Files Created/Modified
- src/webu_json.cpp - Added 3 new API methods
- src/webu_json.hpp - Added method declarations
- src/webu_ans.cpp - Added API and static file routing
- motion-test.conf - Test configuration (if created)

## Test Results
- Build: [Success/Failure]
- Static file serving: [Working/Issues]
- API endpoints: [Working/Issues]
- Pi deployment: [Success/Issues]
- Integration tests: [X/Y passed]

## Issues Encountered
- [List any problems and solutions]

## Status
- [Complete | Partial | Blocked]
```

---

## Key Files Reference

### Created in Phase 1 & 2
- `doc/plans/react-ui/01-architecture-20251227.md` (USE THIS VERSION)
- `doc/plans/react-ui/02-detailed-execution-20251227.md` (USE THIS VERSION)
- `docs/sub-agent-summaries/20251227-react-ui-phase1-backend.md`
- `docs/sub-agent-summaries/20251227-react-ui-phase2-frontend.md`

### To Modify in Phase 3
- `src/webu_json.cpp` - Add API methods
- `src/webu_json.hpp` - Add declarations
- `src/webu_ans.cpp` - Add routing

### To Create in Phase 3
- `motion-test.conf` - Test config (optional)
- `docs/sub-agent-summaries/YYYYMMDD-HHMM-react-ui-phase3-integration.md` (REQUIRED)

---

## Troubleshooting Guide

### Build Fails
- **Mac**: Install deps with `brew install jpeg libmicrohttpd sqlite`
- **Pi**: Unlikely, but check `sudo apt install ...` if needed

### Static Files Not Served
- Verify `webcontrol_html_path` in motion.conf
- Check files exist at that path
- Check Motion logs for errors

### API Calls Fail
- Check routing in webu_ans.cpp
- Verify method signatures match
- Check Motion logs for errors

### Camera Streams Don't Load
- Verify Motion is capturing from cameras
- Check camera IDs (0, 1, 2, etc.)
- Check Motion's existing stream endpoints work

### SPA Routing Breaks
- Verify `webcontrol_spa_mode on`
- Check `serve_static_file()` fallback logic
- Test with `curl` directly

---

## Questions to Ask

1. **Before starting**: Is the Pi powered on?
2. **If build fails on Mac**: Should I proceed to Pi deployment?
3. **If routing is complex**: Should I create helper functions?
4. **If tests fail**: Should I debug locally or on Pi?

---

## Next Phase (Future)

After Phase 3 complete:
- **Configuration UI**: Dynamic forms for Motion's 600+ parameters
- **Media Gallery**: Browse/delete snapshots and videos
- **Real-time Events**: Motion detection notifications
- **Multi-camera Management**: Add/remove cameras via UI

---

## Ready to Execute

All prerequisites met. Follow tasks 3.1 → 3.5 in order. Document everything in sub-agent summary.

**Estimated Time**: 2-4 hours
