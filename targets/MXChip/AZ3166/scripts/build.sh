#!/bin/bash

set -e

# Get the absolute path to the AZ3166 directory (go up 1 level from scripts dir)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AZ3166_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${AZ3166_DIR}/build"
NUM_JOBS=$(nproc || echo 4)
CONFIG=${1:-starter}

echo "=========================================="
echo "IoT DevKit - Build Script"
echo "=========================================="
echo "AZ3166 Dir: ${AZ3166_DIR}"
echo "Build Dir: ${BUILD_DIR}"
echo "Config: ${CONFIG}"
echo "Parallel Jobs: ${NUM_JOBS}"
echo ""

# Check for ARM GCC compiler
if ! command -v arm-none-eabi-gcc &> /dev/null && [ -z "$ARM_GCC_PATH" ]; then
    echo "[WARNING] arm-none-eabi-gcc not found on PATH and ARM_GCC_PATH not set."
    echo "[WARNING] Build will likely fail unless CMake can find it automatically."
    echo ""
fi

# Parse arguments for clean
if [ "$2" == "clean" ] || [ "$2" == "rebuild" ]; then
    echo "[INFO] Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
    mkdir -p "${BUILD_DIR}"
    echo "[OK] Build directory cleaned"
    echo ""
fi

# Create build directory if it doesn't exist
if [ ! -d "${BUILD_DIR}" ]; then
    mkdir -p "${BUILD_DIR}"
fi

# Navigate to build directory
cd "${BUILD_DIR}"

# Check if CMakeCache exists for incremental build
if [ ! -f "CMakeCache.txt" ] || [ ! -f "build.ninja" ] || [ "$2" == "rebuild" ]; then
    echo "[INFO] Configuring CMake..."
    cmake -G Ninja \
        "-DCMAKE_BUILD_TYPE=Release" \
        "-DAPP_CONFIG=${CONFIG}" \
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5" \
        ..
    echo "[OK] CMake configured"
    echo ""
fi

# Build with parallel jobs
echo "[INFO] Building with ${NUM_JOBS} parallel jobs..."
START_TIME=$(date +%s)

if command -v ninja &> /dev/null; then
    ninja -j ${NUM_JOBS}
else
    cmake --build . --parallel ${NUM_JOBS} --config Release
fi

END_TIME=$(date +%s)
BUILD_TIME=$((END_TIME - START_TIME))

echo ""
echo "=========================================="
echo "[OK] Build completed successfully!"
echo "Build time: ${BUILD_TIME}s"
echo "=========================================="
