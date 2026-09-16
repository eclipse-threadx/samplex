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
    [Alias("d")]
    [string]$Demo,
    [string]$Resc,
    [Nullable[int]]$Seed
)

$BoardDir = Resolve-Path "$PSScriptRoot/.."
$BuildDir = Join-Path $BoardDir "build"

# 1. Resolve which demo to simulate
$selectedDemo = $Demo
if (-not $selectedDemo) {
    # Check CMakeCache.txt for ACTIVE_DEMO
    $cacheFile = Join-Path $BuildDir "CMakeCache.txt"
    if (Test-Path $cacheFile) {
        $match = Select-String -Path $cacheFile -Pattern "^ACTIVE_DEMO:STRING=(.*)$"
        if ($match) {
            $cached = $match.Matches.Groups[1].Value.Trim()
            if ($cached -and $cached -ne "all") {
                $selectedDemo = $cached
            }
        }
    }
}

# If still undetermined, check existing built demo directories or default to threadx_basic
if (-not $selectedDemo) {
    if (Test-Path (Join-Path $BuildDir "app/demos/threadx_basic/mimxrt1064_threadx.elf")) {
        $selectedDemo = "threadx_basic"
    } elseif (Test-Path (Join-Path $BuildDir "app/demos/netx_echo/mimxrt1064_threadx.elf")) {
        $selectedDemo = "netx_echo"
    } elseif (Test-Path (Join-Path $BuildDir "app/demos/netx_trng_console/mimxrt1064_threadx.elf")) {
        $selectedDemo = "netx_trng_console"
    } else {
        $selectedDemo = "threadx_basic"
    }
}

# 2. Locate firmware binaries for the selected demo
$serverElfRel = "build/app/demos/$selectedDemo/mimxrt1064_threadx.elf"
$clientElfRel = "build/app/demos/$selectedDemo/mimxrt1064_client.elf"

# Fallback to root build dir if per-demo subfolder does not exist
if (-not (Test-Path (Join-Path $BoardDir $serverElfRel)) -and (Test-Path (Join-Path $BoardDir "build/mimxrt1064_threadx.elf"))) {
    $serverElfRel = "build/mimxrt1064_threadx.elf"
    $clientElfRel = "build/mimxrt1064_client.elf"
}

$ServerElf = Join-Path $BoardDir $serverElfRel
$ClientElf = Join-Path $BoardDir $clientElfRel

if (-not (Test-Path $ServerElf)) {
    Write-Host "[ERROR] Firmware binary for demo '$selectedDemo' not found at:" -ForegroundColor Red
    Write-Host "  $ServerElf" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please build the demo first using:" -ForegroundColor Yellow
    Write-Host "  .\scripts\build.ps1 -Demo $selectedDemo" -ForegroundColor Yellow
    exit 1
}

# 3. Select Renode script and verification mode
if ($Resc) {
    $RescRelPath = $Resc
    $Mode = "Custom Script ($Resc)"
} elseif ($selectedDemo -eq "netx_trng_console") {
    $RescRelPath = "renode/mimxrt1064-trng-console.resc"
    $Mode = "Hardware TRNG Console (Server: 192.168.0.100, Client: 192.168.0.101)"
} elseif ($selectedDemo -eq "netx_echo") {
    $RescRelPath = "renode/mimxrt1064-network-multinode.resc"
    $Mode = "Multi-Node Network Echo (Server: 192.168.0.100, Client: 192.168.0.101)"
} else {
    $RescRelPath = "renode/mimxrt1064-evk.resc"
    $Mode = "ThreadX Core Basic Demo (Single-Node)"
}
$RescFullPath = Join-Path $BoardDir $RescRelPath

# 4. Find Renode executable
$RenodeExe = (Get-Command renode -ErrorAction SilentlyContinue).Source
if (-not $RenodeExe -and (Test-Path "C:\Program Files\Renode\renode.exe")) {
    $RenodeExe = "C:\Program Files\Renode\renode.exe"
}

if (-not $RenodeExe) {
    Write-Error "Renode was not found in PATH or at 'C:\Program Files\Renode\renode.exe'."
    exit 1
}

Write-Host "=========================================="
Write-Host "Starting Renode Simulation"
Write-Host "=========================================="
Write-Host "Renode:      $RenodeExe"
Write-Host "Demo:        $selectedDemo"
Write-Host "Mode:        $Mode"
Write-Host "Script:      $RescFullPath"
if ($null -ne $Seed) {
    Write-Host "Seed:        $Seed (Deterministic)"
}
Write-Host "Server ELF:  $ServerElf"
if (Test-Path $ClientElf) {
    Write-Host "Client ELF:  $ClientElf"
}
Write-Host ""
Write-Host "Opening Renode Monitor and LPUART1 terminal analyzer(s)..."
Write-Host "To exit Renode, type 'quit' in the Renode Monitor or close the window."
Write-Host "=========================================="

Set-Location $BoardDir

# 5. Build Renode execution command passing clean relative binary paths
$renodeCmd = ""
if ($null -ne $Seed) {
    $renodeCmd += "emulation SetSeed $Seed; "
}
$renodeCmd += "`$bin = @`"$serverElfRel`"; `$bin_server = @`"$serverElfRel`"; "
if (Test-Path $ClientElf) {
    $renodeCmd += "`$bin_client = @`"$clientElfRel`"; "
}
$renodeCmd += "include @`"$RescRelPath`""

# Pass relative script path with quotes to avoid tokenization errors when workspace contains spaces
& $RenodeExe -e "$renodeCmd"
