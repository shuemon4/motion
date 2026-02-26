#!/bin/bash
# Build libdatachannel v0.24.x for ARM64 (Raspberry Pi)
# Required for Motion WebRTC streaming support (--with-webrtc)
set -e

VERSION="v0.24.1"
VERSION_NUM="0.24.1"
BUILD_DIR="/tmp/libdatachannel"
PREFIX="/usr/local"

# Clean previous build if present
if [ -d "$BUILD_DIR" ]; then
    echo "Removing previous build directory..."
    rm -rf "$BUILD_DIR"
fi

echo "Cloning libdatachannel $VERSION..."
git clone --depth 1 --branch $VERSION --recurse-submodules \
    https://github.com/paullouisageneau/libdatachannel.git "$BUILD_DIR"

cd "$BUILD_DIR"

echo "Configuring build..."
cmake -B build \
    -DCMAKE_INSTALL_PREFIX=$PREFIX \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_GNUTLS=0 \
    -DUSE_NICE=0 \
    -DNO_MEDIA=0 \
    -DNO_WEBSOCKET=1 \
    -DNO_EXAMPLES=1 \
    -DNO_TESTS=1

echo "Building (this may take several minutes on Pi)..."
cmake --build build -j$(nproc)

echo "Installing..."
sudo cmake --install build
sudo ldconfig

# Create pkgconf .pc file (libdatachannel cmake install does not provide one)
echo "Creating pkgconf file..."
sudo mkdir -p ${PREFIX}/lib/pkgconfig
sudo tee ${PREFIX}/lib/pkgconfig/LibDataChannel.pc > /dev/null << PCEOF
prefix=${PREFIX}
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: LibDataChannel
Description: C/C++ WebRTC network library
Version: ${VERSION_NUM}
Requires: openssl
Libs: -L\${libdir} -ldatachannel
Cflags: -I\${includedir}
PCEOF

echo "Verifying installation..."
pkgconf --modversion LibDataChannel && echo "libdatachannel installed successfully" || echo "WARNING: pkgconf cannot find LibDataChannel"

# Clean up
echo "Cleaning up build directory..."
rm -rf "$BUILD_DIR"

echo "Done."
