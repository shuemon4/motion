# Code Review Issues — /comment run

Files reviewed: `src/video_loopback.cpp`

## Potential Issues

### fd leak in vlp_open_vidpipe() on success path
- **File**: `src/video_loopback.cpp`
- **Line**: ~143–152
- **Severity**: medium
- **Description**: When `tfd = open(buffer, O_RDWR|O_CLOEXEC)` succeeds, the code calls `break` to exit the while loop. The sysfs name file descriptor `fd` (opened with `open(buffer, O_RDONLY|O_CLOEXEC)` at ~line 109) is never closed on this path. `close(fd)` at line 152 is only reached when the `tfd` open fails, so every successful auto-detection leaks one fd.
- **Code snippet**:
  ```cpp
  if ((tfd = open(buffer, O_RDWR|O_CLOEXEC)) >= 0) {
      strncpy(pipepath, buffer, sizeof(pipepath));
      if (pipe_fd >= 0) {
          close(pipe_fd);
      }
      pipe_fd = tfd;
      break;          // fd from the sysfs name file is never closed here
  }
      close(fd);      // only reached when tfd open failed
  ```

### dev fd leaked on ioctl failures in vlp_startpipe()
- **File**: `src/video_loopback.cpp`
- **Line**: ~223–256
- **Severity**: medium
- **Description**: Three early-return error paths after `dev` is opened do not call `close(dev)` before returning -1: the `VIDIOC_QUERYCAP` failure, the `VIDIOC_G_FMT` failure, and the `VIDIOC_S_FMT` failure. Each failure leaks the open device fd.
- **Code snippet**:
  ```cpp
  if (ioctl(dev, VIDIOC_QUERYCAP, &vc) == -1) {
      MOTION_LOG(ERR, TYPE_VIDEO, SHOW_ERRNO, "ioctl (VIDIOC_QUERYCAP)");
      return -1;   // dev not closed
  }
  ...
  if (ioctl(dev, VIDIOC_G_FMT, &v) == -1) {
      MOTION_LOG(ERR, TYPE_VIDEO, SHOW_ERRNO, "ioctl (VIDIOC_G_FMT)");
      return -1;   // dev not closed
  }
  ...
  if (ioctl(dev,VIDIOC_S_FMT, &v) == -1) {
      MOTION_LOG(ERR, TYPE_VIDEO, SHOW_ERRNO, "ioctl (VIDIOC_S_FMT)");
      return -1;   // dev not closed
  }
  ```
