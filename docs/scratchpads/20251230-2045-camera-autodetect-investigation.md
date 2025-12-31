# Camera Auto-Detection Investigation
**Date**: 2025-12-30
**Issue**: Motion not auto-detecting libcamera devices on startup

## Current Behavior

When Motion starts with no camera config specified:
1. Logs: "No camera or sound configuration files specified."
2. Logs: "Adding a camera configuration file."
3. Creates a default camera with all device fields empty
4. Fails with: "Unable to determine camera type"
5. Logs: "No Camera device specified"

## Root Cause

### Code Flow

**src/camera.cpp:482-497** (`init_camera_type()`):
```cpp
void cls_camera::init_camera_type()
{
    if (cfg->libcam_device != "") {
        camera_type = CAMERA_TYPE_LIBCAM;
    } else if (cfg->netcam_url != "") {
        camera_type = CAMERA_TYPE_NETCAM;
    } else if (cfg->v4l2_device != "") {
        camera_type = CAMERA_TYPE_V4L2;
    } else {
        MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO
            , _("Unable to determine camera type"));
        camera_type = CAMERA_TYPE_UNKNOWN;
    }
}
```

**Problem**: All three device fields (`libcam_device`, `netcam_url`, `v4l2_device`) are empty strings by default.

### Expected vs Actual

| Expectation | Reality |
|-------------|---------|
| Auto-detect libcamera on RPi | No auto-detection - requires explicit config |
| Default `libcam_device` = "auto" | Default = "" (empty string) |
| Auto-create working camera config | Auto-creates broken camera config |

## Additional Findings

### Movie Container Default
- Current default: `"mkv"`
- Desired default: `"mp4"`
- Location: `src/conf.cpp` (movie_container parameter)

### API Endpoint Correction
- Old API (deprecated): `/config.json`, `/movies.json`, `/status.json`
- New API (working): `/0/api/config`, `/0/api/cameras`, `/0/api/system/status`

## Proposed Solutions

### Option 1: Default libcam_device to "auto"
Change default value of `libcam_device` from `""` to `"auto"` on Raspberry Pi systems.

**Pros**:
- Simple fix
- Matches documented behavior in libcam.cpp (line 327: "auto" means first Pi camera)
- Works for most RPi use cases

**Cons**:
- Assumes libcamera is always desired on RPi
- May fail on systems without cameras

### Option 2: Auto-detect at runtime
Add platform detection in `init_camera_type()`:
- Check if libcamera is available
- Check `/dev/video*` devices
- Set appropriate camera type automatically

**Pros**:
- Most robust solution
- Works across different platforms
- Graceful fallback

**Cons**:
- More complex implementation
- Runtime overhead

### Option 3: Require explicit configuration
Keep current behavior, document requirement.

**Pros**:
- No code changes
- Explicit > implicit

**Cons**:
- Poor user experience
- Contradicts "auto-detect" expectation

## Recommendations

1. **Immediate fix**: Change `libcam_device` default to `"auto"` in conf.cpp
2. **Change movie_container** default from `"mkv"` to `"mp4"`
3. **Future enhancement**: Add runtime camera detection
