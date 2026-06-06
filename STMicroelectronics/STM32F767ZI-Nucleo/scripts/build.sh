#!/usr/bin/env bash
#  Copyright (c) 2026 Eclipse ThreadX contributors
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/license/mit.
# 
#  SPDX-License-Identifier: MIT
# 
#  Contributors: 
#     Ali Eissa - 2026 version.

set -e

# Parse command line options
CLEAN=false
REBUILD=false

for arg in "$@"; do
    case $arg in
        --clean)
            CLEAN=true
            shift
            ;;
        --rebuild)
            REBUILD=true
            shift
            ;;
        *)
            ;;
    esac
done

# Set directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BOARD_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${BOARD_DIR}/build"
NUM_JOBS=4

echo "=========================================="
echo "STM32F767ZI-Nucleo - Build Script (Linux)"
echo "=========================================="
echo "Board Dir: ${BOARD_DIR}"
echo "Build Dir: ${BUILD_DIR}"
echo ""

# Check for ARM GCC compiler toolchain
if ! command -v arm-none-eabi-gcc &> /dev/null && [ -z "${ARM_GCC_PATH}" ]; then
    echo "[WARNING] arm-none-eabi-gcc not found on PATH and ARM_GCC_PATH not set."
    echo ""
fi

# Perform cleaning actions if requested
if [ "${CLEAN}" = true ] || [ "${REBUILD}" = true ]; then
    echo "[INFO] Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
    mkdir -p "${BUILD_DIR}"
    echo "[OK] Build directory cleaned"
    echo ""
fi

if [ ! -d "${BUILD_DIR}" ]; then
    mkdir -p "${BUILD_DIR}"
fi

cd "${BUILD_DIR}"

# Run CMake configuration if files are missing or a rebuild is requested
if [ ! -f "CMakeCache.txt" ] || [ ! -f "build.ninja" ] || [ "${REBUILD}" = true ]; then
    echo "[INFO] Configuring CMake..."
    cmake -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        ..
    echo "[OK] CMake configured"
    echo ""
fi

# Execute compilation
echo "[INFO] Building with ${NUM_JOBS} parallel jobs..."
if command -v ninja &> /dev/null; then
    ninja -j ${NUM_JOBS}
else
    cmake --build . --parallel ${NUM_JOBS} --config Release
fi

echo ""
echo "=========================================="
echo "[OK] Build completed successfully!"
echo "=========================================="
