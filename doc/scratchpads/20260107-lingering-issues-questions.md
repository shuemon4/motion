# Lingering Issues and Questions - Version 5.0.0

**Date**: 2026-01-07
**Purpose**: Document unresolved issues, deferred features, and compatibility questions identified during changelog review

---

## Deferred Features

### 1. NV12 to YUV420 Conversion (Low Priority)

**Status**: Deferred
**Source**: `doc/plans/20251223-trixie-64bit-migration.md` Phase 2.3
**Description**: NV12 pixel format is now accepted by libcamera, but conversion to YUV420 is not implemented.

**Rationale for Deferral**:
- NV12 and YUV420 are both YUV 4:2:0 formats
- The Y plane (luminance) is identical between formats
- Motion detection algorithm (`cls_alg`) primarily uses luminance
- May work correctly without conversion

**Required Testing**:
- [ ] Test motion detection accuracy with NV12 on Pi 5
- [ ] Compare detection performance: NV12 vs YUV420
- [ ] Verify no false positives/negatives introduced

**Implementation Notes** (if needed later):
- NV12 has interleaved UV, YUV420 has separate U/V planes
- Conversion is a simple UV plane deinterleave
- Add `needs_nv12_conversion` flag and conversion function in `src/libcam.cpp`

---

### 2. V4L2 Tuner Code Deprecation Documentation

**Status**: Not Implemented
**Source**: `doc/plans/20251223-legacy-code-cleanup.md` Phase 4
**Description**: Add deprecation comments to analog TV tuner functionality

**Files Affected**:
- `src/video_v4l2.cpp` lines 334-397 (`set_norm()`)
- `src/video_v4l2.cpp` lines 400-448 (`set_frequency()`)

**Reason for Deferral**: Low priority; V4L2 tuner code is already isolated

**Recommended Action**: Add header comments when touching these functions for other reasons

---

### 3. AfWindows Documentation Update

**Status**: Code updated, documentation may lag
**Source**: `doc/plans/20251221-autofocus-implementation-plan.md` Issue 4, Step 4.2
**Description**: Update `doc/motion_config.html` to match AfWindows format changes

**Current Code Format**: `x|y|width|height[;x|y|width|height;...]` (up to 4 windows)
**Documentation Status**: May still show old format `x | y | h | w`

**Required Action**:
- [ ] Verify current state of `doc/motion_config.html`
- [ ] Update AfWindows section if outdated
- [ ] Add examples for multiple window syntax

---

## Unverified Features

### 4. Autofocus Multiple Windows Implementation

**Status**: Unclear from commit history
**Source**: `doc/plans/20251221-autofocus-implementation-plan.md` Issue 3
**Description**: Support for up to 4 AF windows with semicolon separation

**Questions**:
- Is the multi-window parsing code (Step 3.1) fully implemented?
- Has it been tested with Camera Module v3?
- Does the format `x|y|w|h;x|y|w|h` work correctly?

**Verification Needed**:
- [ ] Test with single window: `libcam_params AfWindows=100|100|200|200`
- [ ] Test with multiple windows: `libcam_params AfWindows=0|0|320|240;320|0|320|240`
- [ ] Verify AF uses specified windows when `AfMetering=1`

---

### 5. Autofocus Hot-Reload Web API

**Status**: Backend likely implemented, web UI integration unclear
**Source**: `doc/plans/20251221-autofocus-implementation-plan.md` Issue 5, Step 5.6
**Description**: Web API handlers for AF parameters in `webu_json.cpp`

**Expected Parameters**:
- `libcam_af_mode` (0=Manual, 1=Auto, 2=Continuous)
- `libcam_lens_position` (dioptres, 0=infinity)
- `libcam_af_range` (0=Normal, 1=Macro, 2=Full)
- `libcam_af_speed` (0=Normal, 1=Fast)
- `libcam_af_trigger` (trigger AF scan in Auto mode)

**Verification Needed**:
- [ ] Test web API endpoints for AF parameter changes
- [ ] Verify hot-reload works without camera restart
- [ ] Confirm MotionEye UI integration (if applicable)

---

## Testing & Validation

### 6. Trixie System Testing

**Status**: Code is Trixie-ready, but actual Trixie testing unclear
**Source**: `doc/plans/20251223-trixie-64bit-migration.md` Phase 5
**Description**: Validation on actual Raspberry Pi OS Trixie (Debian 13) installation

**Build Validation Completed**: Bookworm 64-bit (from plan notes)
**Trixie Validation**: Recommended but status unclear

**Critical Tests**:
- [ ] Build on Trixie with libcamera 0.4.0
- [ ] Build on Trixie with libcamera 0.5.x (backports)
- [ ] Pi 5 + Camera v3 startup and streaming
- [ ] Draft controls behavior on Trixie libcamera
- [ ] NV12 pixel format handling
- [ ] 24+ hour stability test

---

### 7. Long-Term Stability Testing

**Status**: Unknown
**Source**: Multiple plan documents mention 24+ hour stability tests
**Description**: Extended runtime testing to verify no memory leaks, crashes, or degradation

**Test Scenarios**:
- [ ] 24+ hour continuous recording on Pi 5
- [ ] Motion detection accuracy over extended period
- [ ] Memory usage stability (check for leaks)
- [ ] Hot-reload stability (multiple parameter changes)
- [ ] Multi-camera stress test (if applicable)

**Metrics to Monitor**:
- CPU usage over time
- Memory usage over time
- Frame drop rate
- Motion detection false positive/negative rates
- Camera restart events

---

### 8. Diagnostic Logging Cleanup

**Status**: Extensive DIAG markers added, may need cleanup
**Source**: `doc/plans/20251222-resolution-bug-fix.md`
**Description**: Resolution troubleshooting added many DIAG log statements

**Questions**:
- Should DIAG markers remain in production code?
- Should they be converted to DBG level?
- Should they be removed after resolution bug is confirmed fixed?

**Affected Files**:
- `src/libcam.cpp` (multiple DIAG entries for resolution tracing)
- `src/webu_getimg.cpp` (stream encode DIAG entry)

**Recommended Action**:
- Keep critical DIAG markers at DBG level for troubleshooting
- Remove verbose/redundant DIAG markers
- Document which DIAG markers are intentionally kept

---

## Compatibility Questions

### 9. Raspberry Pi 3 and Older Models

**Status**: Not addressed in recent changes
**Description**: Impact of 64-bit migration on 32-bit Pi models (Pi 3, Pi Zero, etc.)

**Questions**:
- Are 32-bit builds still supported?
- Do type safety changes (size_t) maintain 32-bit compatibility?
- Should there be separate build profiles for 32-bit vs 64-bit?

**Verification Needed**:
- [ ] Test build on 32-bit Raspberry Pi OS
- [ ] Verify libcamera detection on Pi 3
- [ ] Confirm V4L2 fallback on older models

---

### 10. USB Camera Support on Pi 5

**Status**: Implied support via libcamera, needs verification
**Source**: `doc/plans/20251223-trixie-64bit-migration.md` mentions libcamera UVC support
**Description**: USB camera support on Pi 5 (V4L2 disabled in pi5-build.sh)

**Implementation**:
- Pi 5 build has `--without-v4l2`
- Comment states "USB cameras can still work via libcamera's UVC support"

**Questions**:
- Has this been tested with actual USB cameras on Pi 5?
- Does libcamera UVC support all USB camera features?
- Are there any limitations vs native V4L2?

**Verification Needed**:
- [ ] Test USB webcam on Pi 5 with libcamera
- [ ] Compare feature parity with V4L2 on Pi 4
- [ ] Document any limitations or differences

---

### 11. FFmpeg Version Compatibility Range

**Status**: Guards added for FFmpeg 7+, but minimum version unclear
**Source**: Multiple files reference MYFFVER, LIBAVCODEC_VERSION_MAJOR
**Description**: Minimum and maximum supported FFmpeg versions

**Questions**:
- What is the minimum FFmpeg version required?
- Are FFmpeg 4.x, 5.x, 6.x, 7.x all supported?
- Do the simplified shims in util.hpp break older FFmpeg?

**Current Shims** (after simplification in `src/util.hpp`):
```cpp
typedef const AVCodec myAVCodec;
typedef const uint8_t myuint;
#define MY_PROFILE_H264_HIGH   AV_PROFILE_H264_HIGH
```

**Compatibility Concern**: These assume FFmpeg 6.0+; may break on older versions

**Recommended Action**:
- Document minimum FFmpeg version in configure.ac
- Add version check to fail gracefully on unsupported versions
- Or restore conditional compilation for broader compatibility

---

## Security Considerations

### 12. CSRF Token Session Management

**Status**: CSRF protection implemented, session handling unclear
**Source**: Commit 8eb3685 "Implement comprehensive security improvements"
**Description**: CSRF token generation and validation

**Questions**:
- How are CSRF tokens stored (cookie, session, header)?
- What is the token lifetime/expiration?
- How are tokens invalidated on logout?
- Is token generation truly cryptographically secure?

**Verification Needed**:
- [ ] Review CSRF token storage mechanism
- [ ] Test token expiration behavior
- [ ] Verify token randomness (entropy source)
- [ ] Test CSRF protection with concurrent sessions

---

### 13. HA1 Digest Security

**Status**: HA1 digest storage implemented
**Source**: Commit 5075791 "Add Phase 5: Credential Management"
**Description**: Secure credential storage using HA1 digests

**Questions**:
- Are plaintext passwords still supported (backwards compatibility)?
- How are HA1 digests generated (salt, algorithm)?
- Is there a migration path from plaintext to HA1?
- What happens if both plaintext and HA1 are configured?

**Recommended Action**:
- Document HA1 digest format and generation
- Provide migration guide for existing deployments
- Add warnings if plaintext passwords detected

---

## MotionEye Integration

### 14. MotionEye Configuration Compatibility

**Status**: Motion changes may require MotionEye updates
**Description**: New parameters and features may not be exposed in MotionEye UI

**New Parameters Needing UI**:
- `libcam_buffer_count`
- `libcam_af_mode`, `libcam_lens_position`, `libcam_af_range`, `libcam_af_speed`
- `libcam_awb_mode` (if not already exposed)
- `movie_encoder_preset`

**Questions**:
- Does MotionEye need updates to support new parameters?
- Are hot-reload controls accessible via MotionEye API?
- Is there a MotionEye release roadmap aligned with Motion 5.0.0?

---

### 15. MotionEye Hot-Reload Integration

**Status**: Motion supports hot-reload, MotionEye integration unclear
**Description**: Real-time parameter updates via MotionEye UI

**Hot-Reload Parameters**:
- Brightness, Contrast
- AWB mode, colour gains, colour temperature
- AF mode, lens position, AF range, AF speed
- AF trigger/cancel

**Questions**:
- Does MotionEye use the hot-reload API or traditional config file updates?
- Are hot-reload changes reflected in MotionEye UI immediately?
- Does MotionEye trigger camera restart on parameter changes?

**Recommended Action**:
- Test hot-reload parameters via MotionEye UI
- Document which parameters support hot-reload vs require restart
- Update MotionEye UI to use hot-reload API where applicable

---

## Performance & Optimization

### 16. CPU Usage Impact of New Features

**Status**: Features added, CPU impact not documented
**Source**: CLAUDE.md rule: "Minimize CPU usage - Pi has limited CPU, generates heat, may run on battery"
**Description**: CPU usage implications of new features

**Features with Potential CPU Impact**:
- Hot-reload framework (periodic polling?)
- CSRF token generation/validation (every request)
- Multiple AF windows (increased processing)
- Mutex synchronization overhead
- Diagnostic logging (if not properly gated)

**Verification Needed**:
- [ ] Benchmark CPU usage: baseline vs 5.0.0
- [ ] Profile hot-reload overhead
- [ ] Measure security feature overhead (CSRF, headers)
- [ ] Optimize if CPU usage increased significantly

---

### 17. Memory Footprint Changes

**Status**: Configuration system refactor claims "reduced memory footprint"
**Source**: Changelog - "Add scoped parameter structs for reduced memory footprint"
**Description**: Actual memory savings not quantified

**Questions**:
- How much memory was saved by scoped structs?
- What is the total memory footprint of Motion 5.0.0 vs 4.7.0?
- Are there memory leaks from new features?

**Verification Needed**:
- [ ] Memory profiling on Pi 5 (baseline vs current)
- [ ] Valgrind analysis for leaks
- [ ] Long-term memory usage monitoring (24+ hours)

---

## Documentation Gaps

### 18. Migration Guide for Users

**Status**: Code changes documented, user migration guide missing
**Description**: Guide for users upgrading from Motion 4.x to 5.0.0

**Topics Needed**:
- Breaking changes (if any)
- New configuration parameters
- Recommended settings for Pi 5 vs Pi 4
- Trixie migration steps
- Security features and how to enable them
- Performance tuning guide

---

### 19. Build Documentation Updates

**Status**: Scripts updated, documentation may lag
**Description**: Build instructions for different platforms

**Files Updated**:
- `scripts/pi4-build.sh` (NEW)
- `scripts/pi5-build.sh` (updated comments)
- `scripts/pi5-setup.sh` (rpicam detection)

**Documentation Needing Updates**:
- `doc/motion_build.html` (may reference old tools/procedures)
- README.md build instructions
- Install guides for Trixie vs Bookworm

---

### 20. API Documentation for Hot-Reload

**Status**: Implementation exists, API documentation unclear
**Description**: Developer documentation for hot-reload framework

**Needed Documentation**:
- How to add new hot-reload parameters
- API endpoints for runtime control changes
- Web API reference (JSON format, expected responses)
- Integration guide for third-party UIs

---

## Summary

**Total Issues Identified**: 20

**Priority Breakdown**:
- **High Priority** (need resolution before release):
  - #6: Trixie system testing
  - #7: Long-term stability testing
  - #18: Migration guide for users

- **Medium Priority** (should address soon):
  - #4: Autofocus multiple windows verification
  - #5: Autofocus hot-reload web API verification
  - #8: Diagnostic logging cleanup
  - #11: FFmpeg version compatibility clarification
  - #16: CPU usage impact assessment

- **Low Priority** (can defer or address as needed):
  - #1: NV12 to YUV420 conversion (only if testing shows issues)
  - #2: V4L2 tuner deprecation documentation
  - #3: AfWindows documentation update
  - All others

---

**Next Steps**:
1. Conduct Trixie system testing on actual Pi 5 hardware
2. Run 24+ hour stability test
3. Create user migration guide
4. Verify autofocus multi-window and hot-reload features
5. Assess CPU usage impact of new features
6. Clean up diagnostic logging based on testing results
