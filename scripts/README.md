# Motion Test Scripts

This directory contains scripts for testing Motion across different platforms and configurations.

## Docker Testing Scripts

### Quick Start

```bash
# Test on all distributions (recommended before committing)
./docker-test.sh all

# Test on specific distribution
./docker-test.sh debian12    # FFmpeg 5.x
./docker-test.sh ubuntu2404  # FFmpeg 6.x
./docker-test.sh fedora40    # FFmpeg 6.x

# Interactive development shell
./docker-interactive.sh              # Debian 12 (default)
./docker-interactive.sh ubuntu2404   # Ubuntu 24.04
./docker-interactive.sh fedora40     # Fedora 40
```

### Available Scripts

| Script | Purpose | Usage |
|--------|---------|-------|
| `docker-test.sh` | Automated build and installation tests | `./docker-test.sh [distro]` |
| `docker-interactive.sh` | Interactive shell for manual testing | `./docker-interactive.sh [distro]` |

### What Gets Tested

**Automated Tests (docker-test.sh):**

1. **FFmpeg API Compatibility (5.x, 6.x, 7.x)**
   - Profile constants (`AV_PROFILE_*` vs `FF_PROFILE_*`)
   - AVIO callback signatures (const vs non-const buffers)
   - Codec ID macros across FFmpeg versions
   - Header availability and API changes

2. **Build System Validation**
   - `autoreconf -fiv` - autotools generation
   - `./configure --with-sqlite3` - dependency detection
   - `make -j$(nproc)` - parallel compilation
   - C++ compilation without warnings/errors
   - Successful linking of motion binary

3. **Installation Verification (DESTDIR)**
   - Binary installation: `/usr/local/bin/motion`
   - Setup tool: `/usr/local/bin/motion-setup`
   - Directory structure creation under `/usr/local/var/lib/motion/`

4. **Hybrid Directory Layout**
   - ✅ User config directory: `user-config/`
   - ✅ Config template: `user-config/local.conf`
   - ✅ Camera config directory: `user-config/cameras/`
   - ✅ Profiles directory: `profiles/`
   - ✅ Profile templates: `day.conf`, `night.conf`, `away.conf`
   - ✅ Runtime directory: `runtime/`
   - ✅ WebUI directory and assets: `webui/index.html`, `webui/assets/`

5. **Binary Functionality**
   - Motion binary is executable
   - Help screen displays correctly (`motion -h`)
   - No runtime library dependency errors

6. **Distribution-Specific Testing**
   - **Debian 12**: FFmpeg 5.x, APT package management
   - **Ubuntu 24.04**: FFmpeg 6.x, APT package management
   - **Fedora 40**: FFmpeg 6.x, DNF package management

**Interactive Manual Tests (docker-interactive.sh):**
- Layered configuration loading
- Profile switching
- Config source tracking
- Security restrictions
- Manual compilation debugging
- Dependency troubleshooting

See [../doc/DOCKER_TESTING.md](../doc/DOCKER_TESTING.md) for detailed guide.

## Platform-Specific Scripts

### Raspberry Pi

- `pi-build.sh` - Optimized build for Pi (installed via `make install-pi-tools`)
- `pi-configure.sh` - Camera auto-detection (installed via `make install-pi-tools`)
- `pi-setup.sh` - Interactive setup wizard (installed via `make install-pi-tools`)

**Note:** Pi scripts are only installed on Raspberry Pi systems via:
```bash
sudo make install-pi-tools
```

### Test Coverage by Distribution

| Distribution | FFmpeg Version | Package Manager | Primary Test Focus |
|--------------|----------------|-----------------|-------------------|
| **Debian 12** | 5.x (59.27.100) | APT | Legacy FFmpeg API compatibility |
| **Ubuntu 24.04** | 6.x (60.16.100) | APT | Modern FFmpeg API, LTS stability |
| **Fedora 40** | 6.x (60.16.100) | DNF | RPM-based systems, bleeding edge |

**Why These Distributions?**
- **Debian 12**: Stable/conservative FFmpeg versions, represents older enterprise systems
- **Ubuntu 24.04**: LTS release, most common desktop/server Linux
- **Fedora 40**: Latest packages, early warning system for API changes

## CI/CD

GitHub Actions workflow automatically runs tests on every push:
- `.github/workflows/build-test.yml`

Tests run on:
- Debian 12 (FFmpeg 5.x)
- Ubuntu 24.04 (FFmpeg 6.x)
- Fedora 40 (FFmpeg 6.x)

**CI Workflow validates:**
- All 3 distributions in parallel
- Build success across FFmpeg versions
- Installation directory structure
- Warning count monitoring (informational)
- Build logs saved as artifacts on failure

## Requirements

- Docker Desktop installed
- Mac, Windows, or Linux host
- Motion source code

## Coverage

These tests cover implementation tasks:
- **Task #12:** FFmpeg compatibility across versions ✅
- **Task #13:** Hybrid directory layout installation ✅
- **Task #15:** Platform detection (partial - Pi hardware needed for full test) ⏸️

## Common Test Failures

### Missing Dependencies
**Symptom**: `autoreconf` fails with "command not found" errors
**Solution**: Check dependency installation in `docker-test.sh` matches distro

Required packages:
- **Debian/Ubuntu**: `build-essential`, `autoconf`, `automake`, `autoconf-archive`, `libtool`, `pkg-config`, `gettext`, `autopoint`, `git`
- **Fedora**: `gcc`, `gcc-c++`, `make`, `autoconf`, `automake`, `autoconf-archive`, `libtool`, `pkgconfig`, `gettext-devel`, `git`

### FFmpeg Compatibility Issues
**Symptom**: Compilation errors referencing FFmpeg constants or types
**Solution**: Check `src/util.hpp` compatibility macros are up to date

Common issues:
- Profile constants renamed (`AV_PROFILE_*` → `FF_PROFILE_*`)
- AVIO callback signature changes (const qualifiers)
- Codec ID renames

### Installation Path Mismatches
**Symptom**: Tests fail at "Verifying directory structure"
**Solution**: Verify test paths match actual install paths

Default install prefix: `/usr/local`
- Binaries: `/usr/local/bin/`
- Libraries: `/usr/local/var/lib/motion/`
- Configs: `/usr/local/etc/motion/`

### Docker Issues
**Symptom**: "Cannot connect to Docker daemon"
**Solution**: Start Docker Desktop

**Symptom**: Container hangs during package installation
**Solution**: Check network connectivity, try pulling image manually:
```bash
docker pull debian:12
docker pull ubuntu:24.04
docker pull fedora:40
```

## Limitations

Docker testing cannot verify:
- Camera hardware (V4L2, libcamera)
- Pi-specific detection on actual Pi
- Systemd service integration
- Performance benchmarks
- Real-time motion detection algorithms

**What Docker tests DO verify:**
- ✅ Code compiles without camera hardware
- ✅ Stub implementations for non-Pi systems
- ✅ Installation directory structure
- ✅ FFmpeg API compatibility
- ✅ Cross-platform package dependencies

For complete Task #15 validation, test on actual Raspberry Pi hardware.

## Debugging Test Failures

### Run Individual Test Steps

```bash
# Get a shell in the test container
docker run --rm -it -v "${PWD}:/motion" -w /motion debian:12 bash

# Then run steps manually:
apt-get update && apt-get install -y build-essential autoconf automake ...
autoreconf -fiv
./configure --with-sqlite3
make -j$(nproc)
make install DESTDIR=/tmp/motion-install

# Check what was installed
find /tmp/motion-install -type f | head -20
ls -la /tmp/motion-install/usr/local/var/lib/motion/
```

### Check Build Logs

```bash
# Run with full output (no redirection)
docker run --rm -v "${PWD}:/motion" -w /motion debian:12 bash -c '
  apt-get update && apt-get install -y build-essential ...
  autoreconf -fiv
  ./configure --with-sqlite3
  make -j$(nproc) 2>&1 | tee build.log
  cat build.log | grep -i error
'
```

### Interactive Debugging

Use `docker-interactive.sh` for manual testing:
```bash
./scripts/docker-interactive.sh debian12
# You're now in a shell with all dependencies installed
# Source code is mounted at /motion
cd /motion
autoreconf -fiv
./configure --with-sqlite3
make
```

### Check FFmpeg Version

```bash
docker run --rm debian:12 bash -c '
  apt-get update -qq && apt-get install -y -qq libavformat-dev pkg-config
  pkg-config --modversion libavformat
  pkg-config --cflags libavformat | grep -o "include/[^ ]*" | head -1
'
```
