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

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BOARD_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${BOARD_DIR}/build"
NUM_JOBS=4

CLEAN=0
REBUILD=0
DEMO="all"

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --clean) CLEAN=1 ;;
        --rebuild) REBUILD=1 ;;
        -d|--demo) DEMO="$2"; shift ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

echo "=========================================="
echo "NXP MIMXRT1064-EVK - Build Script (POSIX)"
echo "=========================================="
echo "Board Dir:   ${BOARD_DIR}"
echo "Build Dir:   ${BUILD_DIR}"
echo "Active Demo: ${DEMO}"
echo ""

# Check for ARM GCC compiler
if ! command -v arm-none-eabi-gcc &> /dev/null && [ -z "${ARM_GCC_PATH}" ]; then
    echo "[WARNING] arm-none-eabi-gcc not found on PATH and ARM_GCC_PATH not set."
    echo ""
fi

if [ "${CLEAN}" -eq 1 ] || [ "${REBUILD}" -eq 1 ]; then
    echo "[INFO] Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
    mkdir -p "${BUILD_DIR}"
    echo "[OK] Build directory cleaned"
    echo ""
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Reconfigure if CMakeCache.txt or build.ninja is missing, or demo changed, or if forced
NEED_CONFIG=0
if [ ! -f "CMakeCache.txt" ] || [ ! -f "build.ninja" ] || [ "${REBUILD}" -eq 1 ]; then
    NEED_CONFIG=1
else
    CACHED_DEMO=$(grep "^ACTIVE_DEMO:STRING=" CMakeCache.txt 2>/dev/null | cut -d'=' -f2 | tr -d '[:space:]')
    if [ "${CACHED_DEMO}" != "${DEMO}" ]; then
        NEED_CONFIG=1
    fi
fi

if [ "${NEED_CONFIG}" -eq 1 ]; then
    echo "[INFO] Configuring CMake for demo: ${DEMO}..."
    cmake -G Ninja \
        "-DCMAKE_BUILD_TYPE=Release" \
        "-DACTIVE_DEMO=${DEMO}" \
        ..
    echo "[OK] CMake configured"
    echo ""
fi

# Run build using Ninja
echo "[INFO] Building target with Ninja (${NUM_JOBS} parallel jobs)..."
ninja -j ${NUM_JOBS}

echo ""
echo "[SUCCESS] Build finished successfully!"

if [ "${DEMO}" = "all" ]; then
    DEMOS_TO_REPORT=("threadx_basic" "netx_echo" "netx_trng_console")
else
    DEMOS_TO_REPORT=("${DEMO}")
fi

for d in "${DEMOS_TO_REPORT[@]}"; do
    DEMO_DIR="${BUILD_DIR}/app/demos/${d}"
    if [ -d "${DEMO_DIR}" ]; then
        echo "[${d}] Output Binaries in ${DEMO_DIR}:"
        if [ -f "${DEMO_DIR}/mimxrt1064_threadx.elf" ]; then
            echo "  - Server ELF: ${DEMO_DIR}/mimxrt1064_threadx.elf"
            echo "  - Server BIN: ${DEMO_DIR}/mimxrt1064_threadx.bin"
        fi
        if [ -f "${DEMO_DIR}/mimxrt1064_client.elf" ]; then
            echo "  - Client ELF: ${DEMO_DIR}/mimxrt1064_client.elf"
            echo "  - Client BIN: ${DEMO_DIR}/mimxrt1064_client.bin"
        fi
    fi
done
