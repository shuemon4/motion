# Docker Testing Guide for Motion

This guide explains how to test Motion using Docker containers on your Mac, Windows, or Linux development machine.

## Prerequisites

- Docker Desktop installed ([download here](https://www.docker.com/products/docker-desktop))
- Motion source code cloned locally

## Quick Start

### Test on Single Distribution

```bash
# Test on Debian 12 (FFmpeg 5.x) - default
./scripts/docker-test.sh

# Test on Ubuntu 24.04 (FFmpeg 6.x)
./scripts/docker-test.sh ubuntu2404

# Test on Fedora 40 (FFmpeg 6.x)
./scripts/docker-test.sh fedora40
```

### Test on All Distributions

```bash
./scripts/docker-test.sh all
```

### Interactive Development Shell

```bash
# Start interactive shell on Debian 12
./scripts/docker-interactive.sh

# Start interactive shell on Ubuntu 24.04
./scripts/docker-interactive.sh ubuntu2404

# Start interactive shell on Fedora 40
./scripts/docker-interactive.sh fedora40
```

## What Gets Tested

### Automated Tests (`docker-test.sh`)

1. **Dependency Installation** - Verifies all build dependencies can be installed
2. **FFmpeg Version Detection** - Confirms FFmpeg version meets requirements
3. **Build Process** - Full build with `autoreconf`, `configure`, `make`
4. **Installation** - Tests `make install` with DESTDIR
5. **Directory Structure** - Verifies hybrid directory layout:
   - `/var/lib/motion/user-config/` created
   - `/var/lib/motion/profiles/` created
   - `/var/lib/motion/runtime/` created
   - `/var/lib/motion/webui/` created
   - Profile templates installed (day.conf, night.conf, away.conf)
   - User config template installed (local.conf)
6. **Binary Validation** - Confirms motion binary runs

### Manual Interactive Testing

In the interactive shell, you can test:

#### 1. Layered Configuration

```bash
# Build Motion
autoreconf -fiv
./configure --with-sqlite3
make -j$(nproc)

# Create test config structure
mkdir -p /tmp/test-config/user-config
mkdir -p /etc/motion

# Create system config
cat > /etc/motion/motion.conf <<EOF
daemon off
threshold 2500
framerate 15
EOF

# Create user override
cat > /tmp/test-config/user-config/local.conf <<EOF
threshold 3000
framerate 20
EOF

# Run Motion (will detect layered mode)
./motion -n -d
# Look for: "Using layered configuration mode (Motion 5.0)"
# Look for: "Loading config layer [system]: /etc/motion/motion.conf"
# Look for: "Loading config layer [user]: /tmp/test-config/user-config/local.conf"
```

#### 2. Profile Switching

```bash
# Install Motion to test directory
make install DESTDIR=/tmp/motion-install

# Create active profile
mkdir -p /tmp/motion-install/var/lib/motion/runtime
echo "day" > /tmp/motion-install/var/lib/motion/runtime/active-profile.txt

# Run with profile
./motion -n -d
# Look for: "Active profile detected: day"
# Look for: "Loading config layer [profile:day]"
```

#### 3. FFmpeg Compatibility

```bash
# Check FFmpeg version
pkg-config --modversion libavformat

# Build with FFmpeg version info
make -j$(nproc)

# Check for FFmpeg compatibility warnings
grep -i "ffmpeg" config.log
```

## Testing Task Coverage

These Docker tests cover the following pending implementation tasks:

### Task #12: Test FFmpeg Compatibility Across Versions

**Tested by:** `docker-test.sh all`

- ✅ Debian 12: FFmpeg 5.x
- ✅ Ubuntu 24.04: FFmpeg 6.x
- ✅ Fedora 40: FFmpeg 6.x

**What to verify:**
- Build completes without errors on all versions
- No FFmpeg-related warnings
- Codec compatibility macros work correctly

### Task #13: Test Hybrid Directory Layout Installation

**Tested by:** `docker-test.sh` installation verification

- ✅ `/var/lib/motion/user-config/` directory created
- ✅ `/var/lib/motion/user-config/local.conf` template installed
- ✅ `/var/lib/motion/profiles/` directory created
- ✅ Day/night/away profile templates installed
- ✅ `/var/lib/motion/runtime/` directory created
- ✅ `/var/lib/motion/webui/` directory created
- ✅ React UI files installed

**What to verify:**
- All directories have correct permissions
- Template files have example content
- Directory structure matches documentation

### Task #15: Platform Detection (Non-Pi Testing)

**Tested by:** Docker containers (not Raspberry Pi)

- ✅ Build succeeds on non-Pi platforms
- ✅ No Pi-specific code required for basic build
- ✅ Pi tools not installed on non-Pi systems

**What to verify:**
- Build completes without Pi detection
- No Pi-specific errors or warnings
- Core functionality works on generic Linux

## CI/CD Integration

The GitHub Actions workflow (`.github/workflows/build-test.yml`) automatically runs these tests on every push and pull request.

### Workflow Matrix

- Debian 12 (FFmpeg 5.x)
- Ubuntu 24.04 (FFmpeg 6.x)
- Fedora 40 (FFmpeg 6.x)

### What CI Tests

1. Checkout code
2. Install dependencies
3. Check FFmpeg version
4. Run autoreconf
5. Configure build
6. Compile with `-j$(nproc)`
7. Test installation with DESTDIR
8. Verify directory structure
9. Test motion binary execution
10. Check for compiler warnings

### Viewing CI Results

1. Go to GitHub repository
2. Click "Actions" tab
3. Select latest workflow run
4. View results for each distribution

## Troubleshooting

### Docker Not Found

```bash
# Install Docker Desktop
# Mac: https://docs.docker.com/desktop/install/mac-install/
# Windows: https://docs.docker.com/desktop/install/windows-install/
```

### Permission Denied

```bash
# Make scripts executable
chmod +x scripts/docker-test.sh
chmod +x scripts/docker-interactive.sh
```

### Container Fails to Start

```bash
# Check Docker is running
docker ps

# Pull base images manually
docker pull debian:12
docker pull ubuntu:24.04
docker pull fedora:40
```

### Build Fails in Container

```bash
# Start interactive shell for debugging
./scripts/docker-interactive.sh [distro]

# Try build steps manually
autoreconf -fiv
./configure --with-sqlite3
make -j$(nproc)
```

### FFmpeg Version Mismatch

```bash
# Check installed FFmpeg version
pkg-config --modversion libavformat

# Verify configure detection
./configure --with-sqlite3
grep FFMPEG config.log
```

## Limitations

### What Docker Testing CANNOT Test

1. **Camera Hardware** (V4L2, libcamera)
   - Requires real camera devices
   - Test on actual Pi hardware

2. **Pi-Specific Detection** (Task #15 partial)
   - Requires `/proc/device-tree/model` from real Pi
   - Test `make install-pi-tools` on actual Pi

3. **Systemd Service**
   - Docker containers typically don't run systemd
   - Test service installation on VM or Pi

4. **Performance Benchmarks**
   - Container performance differs from bare metal
   - Benchmark on actual hardware

### Hardware Testing Required

For complete validation of Task #15, test these on Raspberry Pi:

```bash
# On Raspberry Pi 4 or 5
sudo make install

# Should see Pi detection:
# "Raspberry Pi detected: Raspberry Pi 4 Model B"
# "Install Pi extensions: sudo make install-pi-tools"

# Test Pi tools installation
sudo make install-pi-tools

# Should install:
# - /usr/local/bin/motion-pi-tools/pi-build.sh
# - /usr/local/bin/motion-pi-tools/pi-configure.sh
# - /usr/local/bin/motion-pi-tools/pi-setup.sh
# - /usr/local/bin/motion-setup-pi (symlink)
```

## Best Practices

1. **Run tests before committing**
   ```bash
   ./scripts/docker-test.sh all
   ```

2. **Test interactively for debugging**
   ```bash
   ./scripts/docker-interactive.sh
   ```

3. **Verify CI passes before PR**
   - Check GitHub Actions results
   - Fix any distribution-specific issues

4. **Test layered config manually**
   - Use interactive shell
   - Create test configs
   - Verify loading order

5. **Document new features**
   - Update tests if adding functionality
   - Add test scenarios to this guide

## See Also

- [UPGRADE_5.0.md](../UPGRADE_5.0.md) - Migration guide
- [GitHub Actions Workflow](../../.github/workflows/build-test.yml)
- [Docker Test Script](../../scripts/docker-test.sh)
- [Interactive Shell Script](../../scripts/docker-interactive.sh)
