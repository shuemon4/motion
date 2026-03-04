# Code Review Issues — /comment run

Files reviewed: `src/netcam.cpp`

## Potential Issues

### read_image(): nodata limit of 1000 is undocumented and may be too large
- **File**: `src/netcam.cpp`
- **Line**: ~1450 (`read_image()`)
- **Severity**: low
- **Description**: When `decode_packet()` returns 0 (no frame yet), the loop retries up to 1000 times with no delay. On a stream that consistently delivers non-video packets (e.g., audio-only bursts), this could busy-spin 1000 iterations before giving up. The comment says "The 1000 is arbitrary." If this is hit under high audio traffic the CPU cost may be non-trivial on a Pi. A short sleep or lower threshold could reduce impact.
- **Code snippet**:
  ```cpp
  nodata++;
  if (nodata > 1000) {   // arbitrary, no delay between retries
      context_close();
      return -1;
  }
  ```

### handler_shutdown(): pthread_kill with SIGVTALRM may cause undefined behavior
- **File**: `src/netcam.cpp`
- **Line**: ~2248 (`handler_shutdown()`)
- **Severity**: medium
- **Description**: When both watchdog timeouts expire, `pthread_kill(handler_thread, SIGVTALRM)` is used as a last-resort thread termination. `SIGVTALRM` is not a termination signal — it is typically used for virtual timers and its delivery to a thread may not stop it. The expected effect depends on whether the process has a signal handler installed. The log message warns "Memory leaks will occur" but the thread may continue running, leaving mutexes locked and dangling state. Using `pthread_cancel` with appropriate cancellation points would be safer.
- **Code snippet**:
  ```cpp
  MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO, _("Memory leaks will occur."));
  pthread_kill(handler_thread, SIGVTALRM);   // may not terminate the thread
  ```

### pktarray_resize(): newsize can be negative if idnbr_first < idnbr_last
- **File**: `src/netcam.cpp`
- **Line**: ~454 (`pktarray_resize()`)
- **Severity**: low
- **Description**: `newsize = (int)(((idnbr_first - idnbr_last) * 1) + ((idnbr - idnbr_last) * 2))`. If the image ring wraps (ring_out is ahead of ring_in), `idnbr_first - idnbr_last` can be negative. The `if (newsize < 30)` clamp handles this gracefully, but the intent is not obvious without the ring-buffer context documented.
- **Code snippet**:
  ```cpp
  newsize =(int)(((idnbr_first - idnbr_last) * 1 ) +
      ((idnbr - idnbr_last ) * 2));
  if (newsize < 30) {
      newsize = 30;   // clamp handles negative, but reason is undocumented
  }
  ```

### url_match(): caller must free() returned string but no ownership is documented
- **File**: `src/netcam.cpp`
- **Line**: ~246 (`url_match()`)
- **Severity**: low
- **Description**: `url_match()` returns a `mymalloc`-allocated string. The caller in `url_parse()` correctly calls `free(s)` after use, but this ownership contract is not documented anywhere. If a future caller omits the `free()` there will be a leak with no warning from the compiler.
- **Code snippet**:
  ```cpp
  char *cls_netcam::url_match(regmatch_t m, const char *input) {
      // ... returns mymalloc'd string — caller must free()
  }
  ```

### netcam_start(): context_close() discards codec context opened in this thread
- **File**: `src/netcam.cpp`
- **Line**: ~2320 (`netcam_start()`)
- **Severity**: low
- **Description**: After the initial connect and first image read, `context_close()` is explicitly called before starting the handler thread. The comment explains this is to avoid cross-thread codec contamination between norm and high instances. However, this means the startup sequence does a full open + read + close just to validate connectivity, then the handler immediately opens again. If reconnection fails in the handler, the camera enters the reconnect back-off path. This is by design but the relationship between the startup close and handler reconnect path is subtle and easy to misread as a bug.
- **Code snippet**:
  ```cpp
  context_close();          /* Close in this thread to open it again within handler thread */
  status = NETCAM_RECONNECTING;   /* Set as reconnecting to avoid excess messages when starting */
  first_image = false;
  ```
