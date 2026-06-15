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

# Determine directory paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BOARD_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

LIB_DIR="${BOARD_DIR}/lib/stm32cubef7"
DRIVERS_DIR="${LIB_DIR}/Drivers"
HAL_DEST="${DRIVERS_DIR}/STM32F7xx_HAL_Driver"
CMSIS_DEVICE_DEST="${DRIVERS_DIR}/CMSIS/Device/ST/STM32F7xx"
CMSIS_INCLUDE_DEST="${DRIVERS_DIR}/CMSIS/Include"
NETX_DRIVER_DEST="${BOARD_DIR}/lib/netxduo"

echo "=========================================="
echo "STM32CubeF7 Standalone Driver Fetcher (Linux)"
echo "=========================================="
echo "Fetching to: ${LIB_DIR}"
echo ""

# Clean existing destination directories
rm -rf "${HAL_DEST}"
rm -rf "${CMSIS_DEVICE_DEST}"
rm -rf "${CMSIS_INCLUDE_DEST}"
rm -rf "${NETX_DRIVER_DEST}"

# Re-create directories
mkdir -p "${HAL_DEST}"
mkdir -p "${CMSIS_DEVICE_DEST}"
mkdir -p "${CMSIS_INCLUDE_DEST}"
mkdir -p "${NETX_DRIVER_DEST}"

TEMP_DIR="${BOARD_DIR}/temp_clone"

# Helper function to clear temp clone workspace
clean_temp() {
    if [ -d "${TEMP_DIR}" ]; then
        rm -rf "${TEMP_DIR}"
    fi
}

# Set up exit trap to ensure temp directory is always purged
trap clean_temp EXIT
clean_temp

# 1. Fetch HAL Drivers
echo "[INFO] Cloning STM32F7xx HAL Driver (depth=1)..."
git clone --depth 1 https://github.com/STMicroelectronics/stm32f7xx_hal_driver.git "${TEMP_DIR}"
cp -r "${TEMP_DIR}/Inc" "${HAL_DEST}/"
cp -r "${TEMP_DIR}/Src" "${HAL_DEST}/"
clean_temp
echo "[OK] HAL Drivers copied"
echo ""

# 2. Fetch CMSIS Device F7
echo "[INFO] Cloning CMSIS Device F7 (depth=1)..."
git clone --depth 1 https://github.com/STMicroelectronics/cmsis_device_f7.git "${TEMP_DIR}"
cp -r "${TEMP_DIR}/Include" "${CMSIS_DEVICE_DEST}/"
cp -r "${TEMP_DIR}/Source" "${CMSIS_DEVICE_DEST}/"
clean_temp
echo "[OK] CMSIS Device files copied"
echo ""

# 3. Fetch up-to-date CMSIS Core headers from ST's official repository
echo "[INFO] Cloning up-to-date STM32 CMSIS Core (depth=1)..."
git clone --depth 1 https://github.com/STMicroelectronics/cmsis-core.git "${TEMP_DIR}"
cp -r "${TEMP_DIR}/CMSIS/Core/Include/"* "${CMSIS_INCLUDE_DEST}/"
clean_temp
echo "[OK] Up-to-date CMSIS Core Include headers copied"
echo ""

# 4. Fetch NetX Duo STM32 middleware drivers
echo "[INFO] Cloning STM32 NetX Duo middleware drivers (depth=1)..."
git clone --depth 1 https://github.com/STMicroelectronics/stm32-mw-netxduo.git "${TEMP_DIR}"
cp -f "${TEMP_DIR}/common/drivers/ethernet/nx_stm32_eth_driver.c" "${NETX_DRIVER_DEST}/"
cp -f "${TEMP_DIR}/common/drivers/ethernet/nx_stm32_eth_driver.h" "${NETX_DRIVER_DEST}/"
cp -f "${TEMP_DIR}/common/drivers/ethernet/lan8742/nx_stm32_phy_driver.c" "${NETX_DRIVER_DEST}/"
cp -f "${TEMP_DIR}/common/drivers/ethernet/nx_stm32_phy_driver.h" "${NETX_DRIVER_DEST}/"
clean_temp
echo "[OK] NetX Duo STM32 Ethernet drivers copied"
echo ""

# 5. Fetch LAN8742 PHY driver
echo "[INFO] Cloning LAN8742 PHY driver (depth=1)..."
git clone --depth 1 https://github.com/STMicroelectronics/stm32-lan8742.git "${TEMP_DIR}"
cp -f "${TEMP_DIR}/lan8742.c" "${NETX_DRIVER_DEST}/"
cp -f "${TEMP_DIR}/lan8742.h" "${NETX_DRIVER_DEST}/"
clean_temp
echo "[OK] LAN8742 PHY drivers copied"
echo ""

echo "=========================================="
echo "[SUCCESS] STM32CubeF7 drivers successfully fetched!"
echo "=========================================="
