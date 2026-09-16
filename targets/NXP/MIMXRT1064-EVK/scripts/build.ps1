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
    [switch]$Rebuild,
    [string]$Demo = "all"
)

$BoardDir = Resolve-Path "$PSScriptRoot/.."
$BUILD_DIR = Join-Path $BoardDir "build"
$NUM_JOBS = 4

Write-Host "=========================================="
Write-Host "NXP MIMXRT1064-EVK - Build Script"
Write-Host "=========================================="
Write-Host "Board Dir:   $BoardDir"
Write-Host "Build Dir:   $BUILD_DIR"
Write-Host "Active Demo: $Demo"
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
    New-Item -ItemType Directory -Path $BUILD_DIR -Force | Out-Null
    Write-Host "[OK] Build directory cleaned"
    Write-Host ""
}

if (!(Test-Path $BUILD_DIR)) {
    New-Item -ItemType Directory -Path $BUILD_DIR -Force | Out-Null
}

Push-Location $BUILD_DIR

# Reconfigure if CMakeCache.txt or build.ninja is missing, or demo changed, or if forced
$needConfig = !(Test-Path "CMakeCache.txt") -or !(Test-Path "build.ninja") -or $Rebuild
if (!$needConfig -and (Test-Path "CMakeCache.txt")) {
    $cachedDemo = (Select-String -Path "CMakeCache.txt" -Pattern "^ACTIVE_DEMO:STRING=(.*)$" | ForEach-Object { $_.Matches.Groups[1].Value.Trim() })
    if ($cachedDemo -ne $Demo) {
        $needConfig = $true
    }
}

if ($needConfig) {
    Write-Host "[INFO] Configuring CMake for demo: $Demo..."
    cmake -G Ninja `
        "-DCMAKE_BUILD_TYPE=Release" `
        "-DACTIVE_DEMO=$Demo" `
        ..
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] CMake configuration failed!" -ForegroundColor Red
        Pop-Location
        exit 1
    }
    Write-Host "[OK] CMake configured"
    Write-Host ""
}

# Run build using Ninja
Write-Host "[INFO] Building target with Ninja ($NUM_JOBS parallel jobs)..."
ninja -j $NUM_JOBS
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Build failed!" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host ""
Write-Host "[SUCCESS] Build finished successfully!" -ForegroundColor Green

$demosToReport = @()
if ($Demo -eq "all") {
    $demosToReport = @("threadx_basic", "netx_echo", "netx_trng_console")
} else {
    $demosToReport = @($Demo)
}

foreach ($d in $demosToReport) {
    $demoDir = Join-Path $BUILD_DIR "app/demos/$d"
    if (Test-Path $demoDir) {
        Write-Host "[$d] Output Binaries in $demoDir :" -ForegroundColor Cyan
        $serverElf = Join-Path $demoDir "mimxrt1064_threadx.elf"
        $clientElf = Join-Path $demoDir "mimxrt1064_client.elf"
        if (Test-Path $serverElf) {
            Write-Host "  - Server ELF: $serverElf"
            Write-Host "  - Server BIN: $(Join-Path $demoDir 'mimxrt1064_threadx.bin')"
        }
        if (Test-Path $clientElf) {
            Write-Host "  - Client ELF: $clientElf"
            Write-Host "  - Client BIN: $(Join-Path $demoDir 'mimxrt1064_client.bin')"
        }
    }
}

Pop-Location
