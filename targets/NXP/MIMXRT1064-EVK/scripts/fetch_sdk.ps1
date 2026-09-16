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

$ErrorActionPreference = "Stop"
$BoardDir = Resolve-Path "$PSScriptRoot/.."
$LibDir = Join-Path $BoardDir "lib/mcux-sdk"
$DeviceDir = Join-Path $LibDir "devices/MIMXRT1064"
$DriversDir = Join-Path $LibDir "drivers"
$UtilitiesDir = Join-Path $LibDir "utilities"
$ComponentsDir = Join-Path $LibDir "components"
$BoardFilesDir = Join-Path $LibDir "board"
$CmsisIncludeDest = Join-Path $LibDir "CMSIS/Include"
$TempDir = Join-Path $BoardDir "temp_fetch"

Write-Host "=========================================="
Write-Host "NXP i.MX RT1064 Standalone Driver Fetcher"
Write-Host "=========================================="
Write-Host "Target Directory: $LibDir"
Write-Host ""

# Clean and create target directories
if (Test-Path $LibDir) { Remove-Item -Path $LibDir -Recurse -Force }
New-Item -ItemType Directory -Path $DeviceDir -Force | Out-Null
New-Item -ItemType Directory -Path $DriversDir -Force | Out-Null
New-Item -ItemType Directory -Path $UtilitiesDir -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $ComponentsDir "uart") -Force | Out-Null
New-Item -ItemType Directory -Path $BoardFilesDir -Force | Out-Null
New-Item -ItemType Directory -Path $CmsisIncludeDest -Force | Out-Null
$AppStartupDir = Join-Path $BoardDir "app/startup"
New-Item -ItemType Directory -Path $AppStartupDir -Force | Out-Null

if (Test-Path $TempDir) { Remove-Item -Path $TempDir -Recurse -Force }
New-Item -ItemType Directory -Path $TempDir -Force | Out-Null

function Clean-Temp {
    if (Test-Path $TempDir) {
        Remove-Item -Path $TempDir -Recurse -Force
    }
}

function Verify-Sha256 {
    param(
        [string]$FilePath,
        [string]$ExpectedHash
    )
    $actualHash = (Get-FileHash -Path $FilePath -Algorithm SHA256).Hash.ToLower()
    if ($actualHash -ne $ExpectedHash.ToLower()) {
        throw "SHA256 checksum mismatch for $FilePath! Expected: $ExpectedHash, Got: $actualHash"
    }
}

try {
    # 1. Download official NXP MIMXRT1064 DFP pack from NXP repository (pinned v15.1.0)
    $packUrl = "https://mcuxpresso.nxp.com/cmsis_pack/repo/NXP.MIMXRT1064_DFP.15.1.0.pack"
    $packSha256 = "14e02f0108beba1cfe9de6b2be7b1f874695614dcc16126c452bde1b68b509f6"
    $packZip = Join-Path $TempDir "dfp.zip"
    $packExtract = Join-Path $TempDir "dfp_extracted"

    Write-Host "[INFO] Downloading official NXP MIMXRT1064 Device Pack (v15.1.0)..."
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    if (Get-Command curl.exe -ErrorAction SilentlyContinue) {
        & curl.exe --retry 3 --retry-delay 2 -fsSL $packUrl -o $packZip
    } else {
        Invoke-WebRequest -Uri $packUrl -OutFile $packZip -UseBasicParsing
    }
    Verify-Sha256 -FilePath $packZip -ExpectedHash $packSha256
    Write-Host "[OK] NXP Device Pack verified (SHA256: $packSha256)"

    Write-Host "[INFO] Extracting Device Pack..."
    Expand-Archive -Path $packZip -DestinationPath $packExtract -Force

    # Copy device register headers & system files
    $deviceFiles = @(
        "MIMXRT1064.h",
        "MIMXRT1064_features.h",
        "fsl_device_registers.h",
        "system_MIMXRT1064.c",
        "system_MIMXRT1064.h"
    )
    foreach ($file in $deviceFiles) {
        $source = Join-Path $packExtract $file
        Copy-Item -Path $source -Destination $DeviceDir -Force
    }

    # Copy core peripheral drivers
    $driverList = @(
        "fsl_clock.c", "fsl_clock.h",
        "fsl_common.c", "fsl_common.h",
        "fsl_common_arm.c", "fsl_common_arm.h",
        "fsl_gpio.c", "fsl_gpio.h",
        "fsl_lpuart.c", "fsl_lpuart.h",
        "fsl_enet.c", "fsl_enet.h",
        "fsl_iomuxc.h"
    )
    foreach ($file in $driverList) {
        $source = Join-Path $packExtract "drivers/$file"
        Copy-Item -Path $source -Destination $DriversDir -Force
    }

    # Copy utilities (debug console & string formatting)
    $utilFiles = @(
        "utilities/debug_console_lite/fsl_debug_console.h",
        "utilities/debug_console_lite/fsl_debug_console.c",
        "utilities/debug_console_lite/fsl_assert.c",
        "utilities/debug_console/fsl_debug_console_conf.h",
        "utilities/str/fsl_str.c",
        "utilities/str/fsl_str.h"
    )
    foreach ($file in $utilFiles) {
        $source = Join-Path $packExtract $file
        Copy-Item -Path $source -Destination $UtilitiesDir -Force
    }

    # Copy UART component adapter
    $compUartDest = Join-Path $ComponentsDir "uart"
    $compUartFiles = @(
        "components/uart/fsl_adapter_uart.h",
        "components/uart/fsl_adapter_lpuart.c"
    )
    foreach ($file in $compUartFiles) {
        $source = Join-Path $packExtract $file
        Copy-Item -Path $source -Destination $compUartDest -Force
    }

    # Copy XIP flexspi boot header from pack
    $xipSource = Join-Path $packExtract "xip"
    Copy-Item -Path "$xipSource/*" -Destination $DeviceDir -Recurse -Force
    Write-Host "[OK] NXP Device, Driver, Utility, and Component files copied"
    Write-Host ""

    # Helper function to download with retries and SHA256 checksum verification
    function Download-WithRetry {
        param(
            [string]$Uri,
            [string]$OutFile,
            [string]$ExpectedHash = "",
            [int]$MaxAttempts = 4
        )
        for ($i = 1; $i -le $MaxAttempts; $i++) {
            try {
                if (Get-Command curl.exe -ErrorAction SilentlyContinue) {
                    & curl.exe --retry 3 --retry-delay 2 -fsSL $Uri -o $OutFile
                    if ($LASTEXITCODE -eq 0 -and (Test-Path $OutFile) -and ((Get-Item $OutFile).Length -gt 0)) {
                        if ($ExpectedHash) {
                            Verify-Sha256 -FilePath $OutFile -ExpectedHash $ExpectedHash
                        }
                        return
                    }
                }
                Invoke-WebRequest -Uri $Uri -OutFile $OutFile -UseBasicParsing -TimeoutSec 30
                if ($ExpectedHash) {
                    Verify-Sha256 -FilePath $OutFile -ExpectedHash $ExpectedHash
                }
                return
            }
            catch {
                if ($i -eq $MaxAttempts) { throw $_ }
                Start-Sleep -Seconds 2
            }
        }
    }

    # 2. Download EVK-MIMXRT1064 Board Support Files (pinned to commit 2a340e10 from nxp-mcuxpresso/mcuxsdk-examples)
    $mcuxExamplesCommit = "2a340e10a1105bc0af8e7176bc19148911f4cf12"
    $rawBase = "https://raw.githubusercontent.com/nxp-mcuxpresso/mcuxsdk-examples/$mcuxExamplesCommit/_boards/evkmimxrt1064"
    $boardFiles = @(
        @{ Remote = "$rawBase/board.c"; Local = "board.c"; Hash = "f28885b9ac349a06a6b38f0376f6872a25ec131ab7d37947a0576255579cd959" },
        @{ Remote = "$rawBase/board.h"; Local = "board.h"; Hash = "9c25debd61b7fc153eeedd569155dfc8f9d1349192c6de2b431742ee6f9bb17e" },
        @{ Remote = "$rawBase/project_template/clock_config.c"; Local = "clock_config.c"; Hash = "ca20b253229ef02e74a9d0041173c5e1fc0c5c3df048a53be8b44e1e4218ef03" },
        @{ Remote = "$rawBase/project_template/clock_config.h"; Local = "clock_config.h"; Hash = "52036470ef08b16daf7ed1382a8c3c0bcc123336afc2b07df807e793365b3e80" },
        @{ Remote = "$rawBase/project_template/pin_mux.c"; Local = "pin_mux.c"; Hash = "4bf784e2685555e6297adccb78a27de5754e5320911bc503b5ab563577599c39" },
        @{ Remote = "$rawBase/project_template/pin_mux.h"; Local = "pin_mux.h"; Hash = "f696267090a271e12d9c9f3ccb0e2dfb721025ddd3728b32ab33a9fb79acbc70" },
        @{ Remote = "$rawBase/dcd.c"; Local = "dcd.c"; Hash = "798cd3fffea9b3b1917d6750d40735b6bc890e770f8d8b9167238220b0fae21f" },
        @{ Remote = "$rawBase/dcd.h"; Local = "dcd.h"; Hash = "3a5268f0ccdc02aa6df55b3fca87171df35161cca0c3e15971ac082cdefde7a3" },
        @{ Remote = "$rawBase/xip/evkmimxrt1064_flexspi_nor_config.c"; Local = "evkmimxrt1064_flexspi_nor_config.c"; Hash = "f6fa3d1e3a09c1a4a9d3fc44aed23513e12341e6db96aa3427c923e6b41c6e46" },
        @{ Remote = "$rawBase/xip/evkmimxrt1064_flexspi_nor_config.h"; Local = "evkmimxrt1064_flexspi_nor_config.h"; Hash = "4073f8c6e09fccc879dcedb6fe79f679bc8c9840bb90a7f527279a9a021813d3" }
    )

    Write-Host "[INFO] Downloading EVK-MIMXRT1064 board support files (pinned: $($mcuxExamplesCommit.Substring(0,8)))..."
    foreach ($item in $boardFiles) {
        $dest = Join-Path $BoardFilesDir $item.Local
        Download-WithRetry -Uri $item.Remote -OutFile $dest -ExpectedHash $item.Hash
    }

    Write-Host "[OK] Board support files verified & downloaded"
    Write-Host ""

    # 3. Fetch CMSIS Core headers (pinned ARM.CMSIS 5.9.0 release pack from ARM-software/CMSIS_5)
    $cmsisVersion = "5.9.0"
    $cmsisPackUrl = "https://github.com/ARM-software/CMSIS_5/releases/download/$cmsisVersion/ARM.CMSIS.$cmsisVersion.pack"
    $cmsisPackSha256 = "14b366f2821ee5d32f0d3bf48ef9657ca45347261d0531263580848e9d36f8f4"
    $cmsisPackZip = Join-Path $TempDir "cmsis.zip"
    $cmsisExtract = Join-Path $TempDir "cmsis_extracted"

    Write-Host "[INFO] Downloading official ARM CMSIS Pack (v$cmsisVersion)..."
    Download-WithRetry -Uri $cmsisPackUrl -OutFile $cmsisPackZip -ExpectedHash $cmsisPackSha256
    Write-Host "[OK] ARM CMSIS Pack verified (SHA256: $cmsisPackSha256)"

    Write-Host "[INFO] Extracting CMSIS Core headers..."
    New-Item -ItemType Directory -Path $cmsisExtract -Force | Out-Null
    if (Get-Command tar.exe -ErrorAction SilentlyContinue) {
        & tar.exe -xf $cmsisPackZip -C $cmsisExtract "CMSIS/Core/Include"
    } else {
        Expand-Archive -Path $cmsisPackZip -DestinationPath $cmsisExtract -Force
    }
    $cmsisSource = Join-Path $cmsisExtract "CMSIS/Core/Include"
    Copy-Item -Path "$cmsisSource/*" -Destination $CmsisIncludeDest -Recurse -Force
    Write-Host "[OK] CMSIS Core headers copied"
    Write-Host ""

    Write-Host "[INFO] Note: NetX Duo Ethernet driver and KSZ8081 PHY driver are vendored in lib/netx_driver and lib/phyksz8081."
    Write-Host ""

    Write-Host "=========================================="
    Write-Host "[SUCCESS] NXP i.MX RT1064 SDK dependencies successfully fetched & verified!"
    Write-Host "=========================================="
}
finally {
    Clean-Temp
}
