#!/bin/bash

# Exit on error
set -e

# Install required packages for cross-compilation
sudo apt-get update
sudo apt-get install -y \
    cmake \
    build-essential \
    git \
    crossbuild-essential-armhf \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf

# Create build directory
mkdir -p build
cd build

# Configure CMake for cross-compilation
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../toolchain-rpi.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr

# Build
make -j$(nproc)

# Create deployment package
mkdir -p deploy/boot
mkdir -p deploy/opt/stealth
mkdir -p deploy/etc/systemd/system

# Copy binary and configuration files
cp stealth_system deploy/opt/stealth/
cp ../build_scripts/config.txt deploy/boot/
cp ../build_scripts/cmdline.txt deploy/boot/
cp ../build_scripts/stealth.service deploy/etc/systemd/system/

# Create tarball
tar czf stealth_system.tar.gz -C deploy .
