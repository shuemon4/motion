# Known Issues

This document tracks known issues that are outside the scope of this project to resolve, typically caused by upstream dependencies or third-party packages.

---

## How to Add New Issues

When documenting a new known issue:

1. **Title**: Brief description of the issue
2. **Status**: Active workaround / No workaround / Resolved
3. **Affects**: Which platforms, versions, or configurations
4. **Date Identified**: When the issue was discovered
5. **Upstream Issue**: Link to external bug tracker if available
6. **Problem**: Clear description of what's wrong
7. **Impact**: What functionality is affected
8. **Temporary Fix**: Any workarounds currently in place
9. **Proper Resolution**: What needs to happen to fix it permanently
10. **Related Files**: Links to code or documentation

## Alpine Linux: OpenEXR/Imath Version Mismatch

**Status**: Active workaround in place
**Affects**: Alpine Linux edge (musl builds)
**Date Identified**: 2026-01-12
**Upstream Issue**: Alpine package version inconsistency

### Problem

Alpine edge repository has a package version mismatch between OpenCV and Imath/OpenEXR:
- OpenCV's `libopencv_imgcodecs.so` was compiled against **Imath 3.1**
- Alpine edge currently ships **OpenEXR 3.4** which expects **Imath 3.4**
- This causes linker errors with mixed symbol versions:
  ```
  undefined reference to `Imf_3_4::Chromaticities::Chromaticities(Imath_3_1::Vec2<float> const&, ...)`
  undefined reference to `Imf_3_4::Header::Header(int, int, float, Imath_3_1::Vec2<float> const&, ...)`
  ```

### Impact

Without the workaround, Motion cannot be built on Alpine Linux using OpenCV features (secondary detection algorithms in `alg_sec.cpp`).

### Temporary Fix

**Location**: `.github/workflows/ci.yml`

The GitHub CI workflow explicitly links against Imath for Alpine/musl builds:
```yaml
env:
  LIBS: ${{ matrix.libc == 'musl' && '-lImath' || '' }}
```

For manual builds on Alpine, add `-lImath` to linker flags:
```bash
./configure LIBS="-lImath"
make
```

### Proper Resolution

This issue will be resolved when Alpine rebuilds their `opencv-dev` package against the current version of Imath. No action required from this project.

### Related Files
- `.github/workflows/ci.yml` - Contains workaround
- `doc/scratchpads/20260112-alpine-openexr-linker-issue.md` - Full analysis

---

## Raspberry Pi 2/3: No Hardware H.264 Encoding Support

**Status**: No workaround (by design)
**Affects**: Raspberry Pi 2, Pi 3, Pi 3B+ (VideoCore IV devices)
**Date Identified**: 2026-01-13
**Upstream Issue**: Raspberry Pi Foundation API deprecation

### Problem

This project only supports hardware H.264 encoding via `h264_v4l2m2m` (V4L2 Memory-to-Memory), which is available on Raspberry Pi 4 but **not** on Pi 2/3.

Pi 2/3 devices use the VideoCore IV GPU with hardware encoders accessible through:
- **OpenMAX IL** (`h264_omx` in FFmpeg)
- **MMAL** (`h264_mmal` in FFmpeg)

These APIs are not supported by this project.

### Why Not Supported

1. **Raspberry Pi Foundation Deprecation**: OpenMAX and MMAL are legacy APIs that the Raspberry Pi Foundation deprecated in favor of libcamera and V4L2 for Pi 4+. They are no longer actively maintained.

2. **Project Target**: This project targets 64-bit Raspberry Pi 4 and newer (per CLAUDE.md). The 64-bit Raspberry Pi OS has limited MMAL/OMX support.

3. **FFmpeg Availability**: Modern FFmpeg builds on Raspberry Pi OS may not include `h264_omx` or `h264_mmal` encoders, as these require special compile-time flags and deprecated libraries.

4. **Maintenance Burden**: Supporting legacy hardware encoding APIs would add maintenance overhead for diminishing returns, as Pi 2/3 are increasingly obsolete for video processing workloads.

### Impact

On Raspberry Pi 2/3:
- Video recording works, but uses **software encoding** (`libx264`)
- CPU usage will be significantly higher (~40-70% vs ~10% on Pi 4 with hardware)
- Continuous recording may cause thermal throttling
- Lower resolutions and frame rates recommended

### Potential Future Support

If Pi 2/3 hardware encoding becomes necessary, implementation would require:

1. Add encoder options to `frontend/src/utils/parameterMappings.ts`:
   ```typescript
   { value: 'mkv:h264_omx', label: 'MKV - H.264 Hardware (OMX)', group: 'hardware' },
   { value: 'mp4:h264_omx', label: 'MP4 - H.264 Hardware (OMX)', group: 'hardware' },
   ```

2. Add encoder-specific settings in `src/movie.cpp` (quality control differs from v4l2m2m)

3. Update `useDeviceInfo` hook to detect Pi 2/3 and OMX encoder availability

4. Verify FFmpeg on target system includes the encoder:
   ```bash
   ffmpeg -encoders | grep h264_omx
   ```

**Estimated effort**: 1-2 hours implementation + testing on actual hardware

### Recommendation

For Pi 2/3 deployments:
- Use software encoding with `ultrafast` preset to minimize CPU
- Reduce resolution (720p or lower)
- Enable `movie_passthrough` if source is already H.264
- Consider upgrading to Pi 4 for sustained video recording workloads

### Related Files
- `src/movie.cpp` - Video encoding implementation
- `frontend/src/utils/parameterMappings.ts` - Container/codec options
- `frontend/src/hooks/useDeviceInfo.ts` - Hardware detection
- `doc/scratchpads/20260113-1030-camera-capability-research.md` - Research on hardware encoding

---

