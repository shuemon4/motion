# Code Review Issues — /comment run

Files reviewed: `src/allcam.cpp`

## Potential Issues

### getimg_src increments all_cnct but never decrements it
- **File**: `src/allcam.cpp`
- **Line**: ~91–93 (in `getimg_src`)
- **Severity**: low
- **Description**: When waiting for `img_data` to become non-null, the function increments `strm_c->all_cnct` (only if it is currently 0) to signal the camera thread that there is a consumer. However, `getimg_src` never decrements `all_cnct` when it returns — not on success, not on timeout. If the all-camera view stops actively consuming a stream type (e.g., because no clients are connected to the all-camera MJPEG endpoint), the underlying camera stream's connection counter remains ≥ 1, preventing the camera from idling that stream type. Whether this is intentional (always-on for the all-camera view) is not clear from context.
- **Code snippet**:
  ```cpp
  if (strm_c->img_data == nullptr) {
      if (strm_c->all_cnct == 0){
          strm_c->all_cnct++;   // incremented here
      }
      // ... unlock, sleep, re-lock ...
  }
  // function returns — all_cnct is never decremented
  ```

### getsizes_scale casts integer division result to float losing precision
- **File**: `src/allcam.cpp`
- **Line**: ~278 (in `getsizes_scale`)
- **Severity**: low
- **Description**: The scale computation uses integer division before the cast to float: `(int)((float)(mx_h*100 / p_cam->all_sizes.src_h))`. Because `mx_h*100 / src_h` is integer division, any fractional component is truncated before the float cast, rounding down the scale. For example, if mx_h=720 and src_h=1080, the intended scale is 66.67%, but integer division yields 66, not 66.67. The enclosing `(float)` cast is a no-op because the value is already an integer. This systematic floor-rounding of the auto-scale may cause cameras to appear slightly smaller than intended.
- **Code snippet**:
  ```cpp
  p_cam->all_loc.scale = (int)((float)(mx_h*100 / p_cam->all_sizes.src_h));
  // Integer division happens first; (float) cast does nothing useful
  // Should be: (int)((float)(mx_h * 100) / p_cam->all_sizes.src_h)
  ```

### stream_free and stream_alloc use a magic number for stream slot count
- **File**: `src/allcam.cpp`
- **Line**: ~207, ~230 (in `stream_free` and `stream_alloc`)
- **Severity**: low
- **Description**: Both functions hard-code `indx < 5` to iterate the five stream slots (norm, motion, secondary, source, sub). If a new stream type is added to `ctx_stream`, this magic number must be updated manually in both places, and the new `if (indx == N)` branch must be added. There is no compile-time guarantee they stay in sync.
- **Code snippet**:
  ```cpp
  for (indx=0;indx<5;indx++) {   // magic number — must match stream type count
      if (indx == 0) { strm = &stream.norm; }
      else if (indx == 1) { strm = &stream.motion; }
      // ...
  ```
