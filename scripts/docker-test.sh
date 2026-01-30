#!/bin/bash
#
# Motion Docker Test Script
#
# Tests Motion build and configuration across multiple Linux distributions
# with different FFmpeg versions.
#
# Usage:
#   ./scripts/docker-test.sh [distro]
#
# Distros: debian12, ubuntu2404, fedora40, all
# Default: debian12

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MOTION_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

# Test function for Debian 12 (FFmpeg 5.x)
test_debian12() {
    log_info "Testing on Debian 12 (FFmpeg 5.x)..."

    docker run --rm -v "${MOTION_ROOT}:/motion" -w /motion debian:12 bash -c '
        set -e

        echo "[INFO] Installing dependencies..."
        apt-get update -qq
        apt-get install -y -qq \
            build-essential autoconf automake libtool pkg-config gettext \
            libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libavdevice-dev \
            libmicrohttpd-dev libjpeg-dev libsqlite3-dev \
            >/dev/null 2>&1

        echo "[INFO] Checking FFmpeg version..."
        FFMPEG_VER=$(pkg-config --modversion libavformat)
        echo "[INFO] FFmpeg version: $FFMPEG_VER"

        echo "[INFO] Running autoreconf..."
        autoreconf -fiv >/dev/null 2>&1

        echo "[INFO] Configuring..."
        ./configure --with-sqlite3 >/dev/null 2>&1

        echo "[INFO] Building..."
        make -j$(nproc) >/dev/null 2>&1

        echo "[INFO] Testing installation (DESTDIR)..."
        make install DESTDIR=/tmp/motion-install >/dev/null 2>&1

        echo "[INFO] Verifying directory structure..."
        test -d /tmp/motion-install/usr/local/bin || exit 1
        test -f /tmp/motion-install/usr/local/bin/motion || exit 1
        test -d /tmp/motion-install/var/lib/motion/user-config || exit 1
        test -d /tmp/motion-install/var/lib/motion/profiles || exit 1
        test -d /tmp/motion-install/var/lib/motion/runtime || exit 1
        test -d /tmp/motion-install/var/lib/motion/webui || exit 1
        test -f /tmp/motion-install/var/lib/motion/user-config/local.conf || exit 1
        test -f /tmp/motion-install/var/lib/motion/profiles/day.conf || exit 1
        test -f /tmp/motion-install/var/lib/motion/profiles/night.conf || exit 1
        test -f /tmp/motion-install/var/lib/motion/profiles/away.conf || exit 1

        echo "[INFO] Checking motion binary..."
        /tmp/motion-install/usr/local/bin/motion --help >/dev/null 2>&1 || exit 1

        echo "[SUCCESS] Debian 12 test passed!"
    '

    if [ $? -eq 0 ]; then
        log_success "Debian 12 (FFmpeg 5.x) - PASSED"
        return 0
    else
        log_error "Debian 12 (FFmpeg 5.x) - FAILED"
        return 1
    fi
}

# Test function for Ubuntu 24.04 (FFmpeg 6.x)
test_ubuntu2404() {
    log_info "Testing on Ubuntu 24.04 (FFmpeg 6.x)..."

    docker run --rm -v "${MOTION_ROOT}:/motion" -w /motion ubuntu:24.04 bash -c '
        set -e

        echo "[INFO] Installing dependencies..."
        apt-get update -qq
        DEBIAN_FRONTEND=noninteractive apt-get install -y -qq \
            build-essential autoconf automake libtool pkg-config gettext \
            libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libavdevice-dev \
            libmicrohttpd-dev libjpeg-dev libsqlite3-dev \
            >/dev/null 2>&1

        echo "[INFO] Checking FFmpeg version..."
        FFMPEG_VER=$(pkg-config --modversion libavformat)
        echo "[INFO] FFmpeg version: $FFMPEG_VER"

        echo "[INFO] Running autoreconf..."
        autoreconf -fiv >/dev/null 2>&1

        echo "[INFO] Configuring..."
        ./configure --with-sqlite3 >/dev/null 2>&1

        echo "[INFO] Building..."
        make -j$(nproc) >/dev/null 2>&1

        echo "[INFO] Testing installation (DESTDIR)..."
        make install DESTDIR=/tmp/motion-install >/dev/null 2>&1

        echo "[INFO] Verifying directory structure..."
        test -d /tmp/motion-install/usr/local/bin || exit 1
        test -f /tmp/motion-install/usr/local/bin/motion || exit 1
        test -d /tmp/motion-install/var/lib/motion/user-config || exit 1
        test -d /tmp/motion-install/var/lib/motion/profiles || exit 1
        test -d /tmp/motion-install/var/lib/motion/runtime || exit 1
        test -d /tmp/motion-install/var/lib/motion/webui || exit 1

        echo "[INFO] Checking motion binary..."
        /tmp/motion-install/usr/local/bin/motion --help >/dev/null 2>&1 || exit 1

        echo "[SUCCESS] Ubuntu 24.04 test passed!"
    '

    if [ $? -eq 0 ]; then
        log_success "Ubuntu 24.04 (FFmpeg 6.x) - PASSED"
        return 0
    else
        log_error "Ubuntu 24.04 (FFmpeg 6.x) - FAILED"
        return 1
    fi
}

# Test function for Fedora 40 (FFmpeg 6.x)
test_fedora40() {
    log_info "Testing on Fedora 40 (FFmpeg 6.x)..."

    docker run --rm -v "${MOTION_ROOT}:/motion" -w /motion fedora:40 bash -c '
        set -e

        echo "[INFO] Installing dependencies..."
        dnf install -y -q \
            gcc gcc-c++ make autoconf automake libtool pkgconfig gettext \
            ffmpeg-free-devel libmicrohttpd-devel libjpeg-turbo-devel sqlite-devel \
            >/dev/null 2>&1

        echo "[INFO] Checking FFmpeg version..."
        FFMPEG_VER=$(pkg-config --modversion libavformat)
        echo "[INFO] FFmpeg version: $FFMPEG_VER"

        echo "[INFO] Running autoreconf..."
        autoreconf -fiv >/dev/null 2>&1

        echo "[INFO] Configuring..."
        ./configure --with-sqlite3 >/dev/null 2>&1

        echo "[INFO] Building..."
        make -j$(nproc) >/dev/null 2>&1

        echo "[INFO] Testing installation (DESTDIR)..."
        make install DESTDIR=/tmp/motion-install >/dev/null 2>&1

        echo "[INFO] Verifying directory structure..."
        test -d /tmp/motion-install/usr/local/bin || exit 1
        test -f /tmp/motion-install/usr/local/bin/motion || exit 1
        test -d /tmp/motion-install/var/lib/motion/user-config || exit 1
        test -d /tmp/motion-install/var/lib/motion/profiles || exit 1

        echo "[INFO] Checking motion binary..."
        /tmp/motion-install/usr/local/bin/motion --help >/dev/null 2>&1 || exit 1

        echo "[SUCCESS] Fedora 40 test passed!"
    '

    if [ $? -eq 0 ]; then
        log_success "Fedora 40 (FFmpeg 6.x) - PASSED"
        return 0
    else
        log_error "Fedora 40 (FFmpeg 6.x) - FAILED"
        return 1
    fi
}

# Main test dispatcher
run_tests() {
    local distro="${1:-debian12}"
    local failed=0

    log_info "Motion Docker Test Suite"
    log_info "Testing distro: $distro"
    echo ""

    case "$distro" in
        debian12)
            test_debian12 || failed=$((failed + 1))
            ;;
        ubuntu2404)
            test_ubuntu2404 || failed=$((failed + 1))
            ;;
        fedora40)
            test_fedora40 || failed=$((failed + 1))
            ;;
        all)
            log_info "Running all distribution tests..."
            echo ""
            test_debian12 || failed=$((failed + 1))
            echo ""
            test_ubuntu2404 || failed=$((failed + 1))
            echo ""
            test_fedora40 || failed=$((failed + 1))
            ;;
        *)
            log_error "Unknown distro: $distro"
            log_info "Available: debian12, ubuntu2404, fedora40, all"
            exit 1
            ;;
    esac

    echo ""
    if [ $failed -eq 0 ]; then
        log_success "All tests passed!"
        exit 0
    else
        log_error "$failed test(s) failed"
        exit 1
    fi
}

# Check Docker availability
if ! command -v docker &> /dev/null; then
    log_error "Docker is not installed or not in PATH"
    log_info "Install Docker Desktop: https://www.docker.com/products/docker-desktop"
    exit 1
fi

# Run tests
run_tests "${1:-debian12}"
