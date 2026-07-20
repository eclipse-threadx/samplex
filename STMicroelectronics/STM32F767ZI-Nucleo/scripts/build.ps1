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

param(
    [switch]$Clean,
    [switch]$Rebuild
)

$BoardDir = Resolve-Path "$PSScriptRoot/.."
$BUILD_DIR = Join-Path $BoardDir "build"
$NUM_JOBS = 4

Write-Host "=========================================="
Write-Host "STM32F767ZI-Nucleo - Build Script"
Write-Host "=========================================="
Write-Host "Board Dir: $BoardDir"
Write-Host "Build Dir: $BUILD_DIR"
Write-Host ""

# Check for ARM GCC compiler
$armGcc = Get-Command "arm-none-eabi-gcc" -ErrorAction SilentlyContinue
if (!$armGcc -and !$env:ARM_GCC_PATH) {
    Write-Host "[WARNING] arm-none-eabi-gcc not found on PATH and ARM_GCC_PATH not set." -ForegroundColor Yellow
    Write-Host ""
}

if ($Clean -or $Rebuild) {
    Write-Host "[INFO] Cleaning build directory..."
    if (Test-Path $BUILD_DIR) {
        Remove-Item -Path $BUILD_DIR -Recurse -Force
    }
    New-Item -ItemType Directory -Path $BUILD_DIR -Force
    Write-Host "[OK] Build directory cleaned"
    Write-Host ""
}

if (!(Test-Path $BUILD_DIR)) {
    New-Item -ItemType Directory -Path $BUILD_DIR -Force
}

Push-Location $BUILD_DIR

# Reconfigure if CMakeCache.txt or build.ninja is missing, or if forced
if (!(Test-Path "CMakeCache.txt") -or !(Test-Path "build.ninja") -or $Rebuild) {
    Write-Host "[INFO] Configuring CMake..."
    cmake -G Ninja `
        "-DCMAKE_BUILD_TYPE=Release" `
        ..
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] CMake configuration failed!" -ForegroundColor Red
        Pop-Location
        exit 1
    }
    Write-Host "[OK] CMake configured"
    Write-Host ""
}

Write-Host "[INFO] Building with $NUM_JOBS parallel jobs..."
if (Get-Command ninja -ErrorAction SilentlyContinue) {
    ninja -j $NUM_JOBS
} else {
    cmake --build . --parallel $NUM_JOBS --config Release
}

$buildExitCode = $LASTEXITCODE
Pop-Location

if ($buildExitCode -ne 0) {
    Write-Host "[ERROR] Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "=========================================="
Write-Host "[OK] Build completed successfully!"
Write-Host "=========================================="
