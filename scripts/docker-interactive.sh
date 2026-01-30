#!/bin/bash
#
# Motion Interactive Docker Shell
#
# Starts an interactive Docker container with Motion source mounted
# and all dependencies installed for development and testing.
#
# Usage:
#   ./scripts/docker-interactive.sh [distro]
#
# Distros: debian12, ubuntu2404, fedora40
# Default: debian12

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MOTION_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

DISTRO="${1:-debian12}"

echo "Starting interactive Docker shell on $DISTRO..."
echo "Motion source mounted at /motion"
echo ""

case "$DISTRO" in
    debian12)
        docker run -it --rm -v "${MOTION_ROOT}:/motion" -w /motion debian:12 bash -c '
            apt-get update -qq
            apt-get install -y -qq \
                build-essential autoconf automake libtool pkg-config gettext \
                libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libavdevice-dev \
                libmicrohttpd-dev libjpeg-dev libsqlite3-dev \
                vim less tree

            echo ""
            echo "==================================================================="
            echo "  Motion Development Environment - Debian 12 (FFmpeg 5.x)"
            echo "==================================================================="
            echo ""
            echo "FFmpeg version: $(pkg-config --modversion libavformat)"
            echo ""
            echo "Quick start:"
            echo "  autoreconf -fiv"
            echo "  ./configure --with-sqlite3"
            echo "  make -j$(nproc)"
            echo "  make install DESTDIR=/tmp/motion-install"
            echo ""
            echo "Test layered config:"
            echo "  mkdir -p /tmp/test-config/user-config"
            echo "  echo \"threshold 3000\" > /tmp/test-config/user-config/local.conf"
            echo "  ./motion -c /tmp/test-config/motion.conf -n -d"
            echo ""
            echo "==================================================================="
            echo ""

            exec bash
        '
        ;;
    ubuntu2404)
        docker run -it --rm -v "${MOTION_ROOT}:/motion" -w /motion ubuntu:24.04 bash -c '
            apt-get update -qq
            DEBIAN_FRONTEND=noninteractive apt-get install -y -qq \
                build-essential autoconf automake libtool pkg-config gettext \
                libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libavdevice-dev \
                libmicrohttpd-dev libjpeg-dev libsqlite3-dev \
                vim less tree

            echo ""
            echo "==================================================================="
            echo "  Motion Development Environment - Ubuntu 24.04 (FFmpeg 6.x)"
            echo "==================================================================="
            echo ""
            echo "FFmpeg version: $(pkg-config --modversion libavformat)"
            echo ""
            echo "Quick start:"
            echo "  autoreconf -fiv"
            echo "  ./configure --with-sqlite3"
            echo "  make -j$(nproc)"
            echo "  make install DESTDIR=/tmp/motion-install"
            echo ""
            echo "==================================================================="
            echo ""

            exec bash
        '
        ;;
    fedora40)
        docker run -it --rm -v "${MOTION_ROOT}:/motion" -w /motion fedora:40 bash -c '
            dnf install -y -q \
                gcc gcc-c++ make autoconf automake libtool pkgconfig gettext \
                ffmpeg-free-devel libmicrohttpd-devel libjpeg-turbo-devel sqlite-devel \
                vim less tree

            echo ""
            echo "==================================================================="
            echo "  Motion Development Environment - Fedora 40 (FFmpeg 6.x)"
            echo "==================================================================="
            echo ""
            echo "FFmpeg version: $(pkg-config --modversion libavformat)"
            echo ""
            echo "Quick start:"
            echo "  autoreconf -fiv"
            echo "  ./configure --with-sqlite3"
            echo "  make -j$(nproc)"
            echo "  make install DESTDIR=/tmp/motion-install"
            echo ""
            echo "==================================================================="
            echo ""

            exec bash
        '
        ;;
    *)
        echo "Unknown distro: $DISTRO"
        echo "Available: debian12, ubuntu2404, fedora40"
        exit 1
        ;;
esac
