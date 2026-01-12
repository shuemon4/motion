# Known Issues

This document tracks known issues that are outside the scope of this project to resolve, typically caused by upstream dependencies or third-party packages.

---

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
