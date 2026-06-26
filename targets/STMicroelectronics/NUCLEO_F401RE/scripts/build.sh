#!/usr/bin/env bash

# Copyright (c) 2026 Eclipse ThreadX contributors
#
# SPDX-License-Identifier: MIT

# Fail on error
set -e

CONFIG="starter"
CLEAN=false
REBUILD=false

# Directory resolution
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
BOARD_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$BOARD_DIR/build"
NUM_JOBS=4

# Print banner
echo "=========================================="
echo "Nucleo F401RE - Build Script (Bash)"
echo "=========================================="
echo "Board Dir: $BOARD_DIR"
echo "Build Dir: $BUILD_DIR"
echo "Config:    $CONFIG"
echo ""

# Parse options
while [[ $# -gt 0 ]]; do
    case "$1" in
        -c|--config)
            CONFIG="$2"
            shift 2
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --rebuild)
            REBUILD=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--config CONFIG] [--clean] [--rebuild]"
            exit 1
            ;;
    esac
done

# Verify toolchain availability
MISSING_TOOLS=()

if ! command -v cmake &>/dev/null; then
    MISSING_TOOLS+=("cmake")
fi

if ! command -v ninja &>/dev/null; then
    # Try checking standard make as a fallback, but we prefer ninja
    if ! command -v make &>/dev/null; then
        MISSING_TOOLS+=("ninja or make")
    fi
fi

if ! command -v arm-none-eabi-gcc &>/dev/null && [ -z "$ARM_GCC_PATH" ]; then
    MISSING_TOOLS+=("arm-none-eabi-gcc")
fi

if [ ${#MISSING_TOOLS[@]} -ne 0 ]; then
    echo "[ERROR] Required build tools are missing from the system path:"
    for tool in "${MISSING_TOOLS[@]}"; do
        echo "  - $tool"
    done
    echo ""
    echo "Please install these dependencies (e.g. on Ubuntu 24.04: sudo apt update && sudo apt install cmake ninja-build gcc-arm-none-eabi)"
    exit 1
fi

if [ "$CLEAN" = true ] || [ "$REBUILD" = true ]; then
    echo "[INFO] Cleaning build directory..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
    mkdir -p "$BUILD_DIR"
    echo "[OK] Build directory cleaned"
    echo ""
fi

if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Reconfigure if CMakeCache.txt or build.ninja is missing, or if forced
if [ ! -f "CMakeCache.txt" ] || [ ! -f "build.ninja" ] || [ "$REBUILD" = true ]; then
    echo "[INFO] Configuring CMake..."
    
    # Check if Ninja is available, use it if so
    if command -v ninja &>/dev/null; then
        cmake -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DAPP_CONFIG="$CONFIG" \
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
            "$BOARD_DIR"
    else
        cmake \
            -DCMAKE_BUILD_TYPE=Release \
            -DAPP_CONFIG="$CONFIG" \
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
            "$BOARD_DIR"
    fi
    echo "[OK] CMake configured"
    echo ""
fi

echo "[INFO] Building..."
if command -v ninja &>/dev/null && [ -f "build.ninja" ]; then
    ninja -j $NUM_JOBS
else
    cmake --build . --parallel $NUM_JOBS --config Release
fi

echo ""
echo "=========================================="
echo "[OK] Build completed successfully!"
echo "=========================================="
