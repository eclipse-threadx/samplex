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

$BoardDir = Resolve-Path "$PSScriptRoot/.."
$LibDir = Join-Path $BoardDir "lib/stm32cubef7"
$DriversDir = Join-Path $LibDir "Drivers"
$HalDest = Join-Path $DriversDir "STM32F7xx_HAL_Driver"
$CmsisDeviceDest = Join-Path $DriversDir "CMSIS/Device/ST/STM32F7xx"
$CmsisIncludeDest = Join-Path $DriversDir "CMSIS/Include"
$NetxDriverDest = Join-Path $BoardDir "lib/netxduo"

Write-Host "=========================================="
Write-Host "STM32CubeF7 Standalone Driver Fetcher"
Write-Host "=========================================="
Write-Host "Fetching to: $LibDir"
Write-Host ""

# Create destination directories
if (Test-Path $HalDest) { Remove-Item -Path $HalDest -Recurse -Force }
if (Test-Path $CmsisDeviceDest) { Remove-Item -Path $CmsisDeviceDest -Recurse -Force }
if (Test-Path $CmsisIncludeDest) { Remove-Item -Path $CmsisIncludeDest -Recurse -Force }
if (Test-Path $NetxDriverDest) { Remove-Item -Path $NetxDriverDest -Recurse -Force }

New-Item -ItemType Directory -Path $HalDest -Force | Out-Null
New-Item -ItemType Directory -Path $CmsisDeviceDest -Force | Out-Null
New-Item -ItemType Directory -Path $CmsisIncludeDest -Force | Out-Null
New-Item -ItemType Directory -Path $NetxDriverDest -Force | Out-Null

$TempDir = Join-Path $BoardDir "temp_clone"

# Helper function to clean up temp dir if exists
function CleanTemp {
    if (Test-Path $TempDir) {
        Remove-Item -Path $TempDir -Recurse -Force
    }
}

CleanTemp

# 1. Fetch HAL Drivers
Write-Host "[INFO] Cloning STM32F7xx HAL Driver (depth=1)..."
git clone --depth 1 https://github.com/STMicroelectronics/stm32f7xx_hal_driver.git $TempDir
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Failed to clone HAL Driver repository!" -ForegroundColor Red
    CleanTemp
    exit 1
}
Copy-Item -Path "$TempDir/Inc" -Destination $HalDest -Recurse -Force
Copy-Item -Path "$TempDir/Src" -Destination $HalDest -Recurse -Force
CleanTemp
Write-Host "[OK] HAL Drivers copied"
Write-Host ""

# 2. Fetch CMSIS Device F7
Write-Host "[INFO] Cloning CMSIS Device F7 (depth=1)..."
git clone --depth 1 https://github.com/STMicroelectronics/cmsis_device_f7.git $TempDir
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Failed to clone CMSIS Device repository!" -ForegroundColor Red
    CleanTemp
    exit 1
}
Copy-Item -Path "$TempDir/Include" -Destination $CmsisDeviceDest -Recurse -Force
Copy-Item -Path "$TempDir/Source" -Destination $CmsisDeviceDest -Recurse -Force
CleanTemp
Write-Host "[OK] CMSIS Device files copied"
Write-Host ""

# 3. Fetch up-to-date CMSIS Core headers from ST's official repository
Write-Host "[INFO] Cloning up-to-date STM32 CMSIS Core (depth=1)..."
git clone --depth 1 https://github.com/STMicroelectronics/cmsis-core.git $TempDir
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Failed to clone CMSIS Core repository!" -ForegroundColor Red
    CleanTemp
    exit 1
}
Copy-Item -Path "$TempDir/CMSIS/Core/Include/*" -Destination $CmsisIncludeDest -Recurse -Force
CleanTemp
Write-Host "[OK] Up-to-date CMSIS Core Include headers copied"
Write-Host ""

# 4. Fetch NetX Duo STM32 middleware drivers
Write-Host "[INFO] Cloning STM32 NetX Duo middleware drivers (depth=1)..."
git clone --depth 1 https://github.com/STMicroelectronics/stm32-mw-netxduo.git $TempDir
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Failed to clone NetX Duo middleware repository!" -ForegroundColor Red
    CleanTemp
    exit 1
}
Copy-Item -Path "$TempDir/common/drivers/ethernet/nx_stm32_eth_driver.c" -Destination $NetxDriverDest -Force
Copy-Item -Path "$TempDir/common/drivers/ethernet/nx_stm32_eth_driver.h" -Destination $NetxDriverDest -Force
Copy-Item -Path "$TempDir/common/drivers/ethernet/lan8742/nx_stm32_phy_driver.c" -Destination $NetxDriverDest -Force
Copy-Item -Path "$TempDir/common/drivers/ethernet/nx_stm32_phy_driver.h" -Destination $NetxDriverDest -Force
CleanTemp
Write-Host "[OK] NetX Duo STM32 Ethernet drivers copied"
Write-Host ""

# 5. Fetch LAN8742 PHY driver
Write-Host "[INFO] Cloning LAN8742 PHY driver (depth=1)..."
git clone --depth 1 https://github.com/STMicroelectronics/stm32-lan8742.git $TempDir
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Failed to clone LAN8742 PHY repository!" -ForegroundColor Red
    CleanTemp
    exit 1
}
Copy-Item -Path "$TempDir/lan8742.c" -Destination $NetxDriverDest -Force
Copy-Item -Path "$TempDir/lan8742.h" -Destination $NetxDriverDest -Force
CleanTemp
Write-Host "[OK] LAN8742 PHY drivers copied"
Write-Host ""

Write-Host "=========================================="
Write-Host "[SUCCESS] STM32CubeF7 drivers successfully fetched!"
Write-Host "=========================================="
