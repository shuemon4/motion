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

**Automated Tests:**
- ✅ FFmpeg version compatibility (5.x, 6.x)
- ✅ Build process (autoreconf, configure, make)
- ✅ Installation (make install DESTDIR)
- ✅ Hybrid directory layout
- ✅ Profile templates
- ✅ Binary execution

**Interactive Manual Tests:**
- Layered configuration loading
- Profile switching
- Config source tracking
- Security restrictions

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

## CI/CD

GitHub Actions workflow automatically runs tests on every push:
- `.github/workflows/build-test.yml`

Tests run on:
- Debian 12 (FFmpeg 5.x)
- Ubuntu 24.04 (FFmpeg 6.x)
- Fedora 40 (FFmpeg 6.x)

## Requirements

- Docker Desktop installed
- Mac, Windows, or Linux host
- Motion source code

## Coverage

These tests cover implementation tasks:
- **Task #12:** FFmpeg compatibility across versions ✅
- **Task #13:** Hybrid directory layout installation ✅
- **Task #15:** Platform detection (partial - Pi hardware needed for full test) ⏸️

## Limitations

Docker testing cannot verify:
- Camera hardware (V4L2, libcamera)
- Pi-specific detection on actual Pi
- Systemd service integration
- Performance benchmarks

For complete Task #15 validation, test on actual Raspberry Pi hardware.
