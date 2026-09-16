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

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BOARD_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

LIB_DIR="${BOARD_DIR}/lib/mcux-sdk"
DEVICE_DIR="${LIB_DIR}/devices/MIMXRT1064"
DRIVERS_DIR="${LIB_DIR}/drivers"
UTILITIES_DIR="${LIB_DIR}/utilities"
COMPONENTS_DIR="${LIB_DIR}/components"
BOARD_FILES_DIR="${LIB_DIR}/board"
CMSIS_INCLUDE_DEST="${LIB_DIR}/CMSIS/Include"
APP_STARTUP_DIR="${BOARD_DIR}/app/startup"
TEMP_DIR="${BOARD_DIR}/temp_fetch"

echo "=========================================="
echo "NXP i.MX RT1064 Standalone Driver Fetcher (POSIX)"
echo "=========================================="
echo "Target Directory: ${LIB_DIR}"
echo ""

# Helper to verify file integrity via sha256sum
verify_sha256() {
    local file="$1"
    local expected_sha="$2"
    echo "${expected_sha}  ${file}" | sha256sum --check --strict >/dev/null 2>&1 || {
        echo "[ERROR] SHA256 checksum mismatch for ${file}!" >&2
        echo "  Expected: ${expected_sha}" >&2
        echo "  Actual:   $(sha256sum "${file}" | awk '{print $1}')" >&2
        exit 1
    }
}

# Helper to download and verify
fetch_and_verify() {
    local url="$1"
    local dest="$2"
    local expected_sha="$3"
    curl --retry 3 --retry-delay 2 -fsSL "${url}" -o "${dest}"
    verify_sha256 "${dest}" "${expected_sha}"
}

# Clean and recreate directories
rm -rf "${LIB_DIR}"
mkdir -p "${DEVICE_DIR}"
mkdir -p "${DRIVERS_DIR}"
mkdir -p "${UTILITIES_DIR}"
mkdir -p "${COMPONENTS_DIR}/uart"
mkdir -p "${BOARD_FILES_DIR}"
mkdir -p "${CMSIS_INCLUDE_DEST}"
mkdir -p "${APP_STARTUP_DIR}"

rm -rf "${TEMP_DIR}"
mkdir -p "${TEMP_DIR}"

clean_temp() {
    if [ -d "${TEMP_DIR}" ]; then
        rm -rf "${TEMP_DIR}"
    fi
}
trap clean_temp EXIT

# 1. Download official NXP MIMXRT1064 DFP pack from NXP repository (pinned v15.1.0)
PACK_URL="https://mcuxpresso.nxp.com/cmsis_pack/repo/NXP.MIMXRT1064_DFP.15.1.0.pack"
PACK_SHA256="14e02f0108beba1cfe9de6b2be7b1f874695614dcc16126c452bde1b68b509f6"
PACK_ZIP="${TEMP_DIR}/dfp.zip"
PACK_EXTRACT="${TEMP_DIR}/dfp_extracted"

echo "[INFO] Downloading official NXP MIMXRT1064 Device Pack (v15.1.0)..."
fetch_and_verify "${PACK_URL}" "${PACK_ZIP}" "${PACK_SHA256}"
echo "[OK] NXP Device Pack verified (SHA256: ${PACK_SHA256})"

echo "[INFO] Extracting Device Pack..."
mkdir -p "${PACK_EXTRACT}"
unzip -q "${PACK_ZIP}" -d "${PACK_EXTRACT}"

# Copy device register headers & system files
for file in MIMXRT1064.h MIMXRT1064_features.h fsl_device_registers.h system_MIMXRT1064.c system_MIMXRT1064.h; do
    cp "${PACK_EXTRACT}/${file}" "${DEVICE_DIR}/"
done

# Copy core peripheral drivers
for file in fsl_clock.c fsl_clock.h fsl_common.c fsl_common.h fsl_common_arm.c fsl_common_arm.h fsl_gpio.c fsl_gpio.h fsl_lpuart.c fsl_lpuart.h fsl_enet.c fsl_enet.h fsl_iomuxc.h; do
    cp "${PACK_EXTRACT}/drivers/${file}" "${DRIVERS_DIR}/"
done

# Copy utilities (debug console & string formatting)
for file in utilities/debug_console_lite/fsl_debug_console.h \
            utilities/debug_console_lite/fsl_debug_console.c \
            utilities/debug_console_lite/fsl_assert.c \
            utilities/debug_console/fsl_debug_console_conf.h \
            utilities/str/fsl_str.c \
            utilities/str/fsl_str.h; do
    cp "${PACK_EXTRACT}/${file}" "${UTILITIES_DIR}/"
done

# Copy UART component adapter
for file in components/uart/fsl_adapter_uart.h components/uart/fsl_adapter_lpuart.c; do
    cp "${PACK_EXTRACT}/${file}" "${COMPONENTS_DIR}/uart/"
done

# Copy XIP flexspi boot headers
cp -r "${PACK_EXTRACT}/xip/"* "${DEVICE_DIR}/"
echo "[OK] NXP Device, Driver, Utility, and Component files copied"
echo ""

# 2. Download EVK-MIMXRT1064 Board Support Files (pinned to commit 2a340e10 from nxp-mcuxpresso/mcuxsdk-examples)
MCUX_EXAMPLES_COMMIT="2a340e10a1105bc0af8e7176bc19148911f4cf12"
RAW_BASE="https://raw.githubusercontent.com/nxp-mcuxpresso/mcuxsdk-examples/${MCUX_EXAMPLES_COMMIT}/_boards/evkmimxrt1064"
echo "[INFO] Downloading EVK-MIMXRT1064 board support files (pinned: ${MCUX_EXAMPLES_COMMIT:0:8})..."

fetch_and_verify "${RAW_BASE}/board.c" "${BOARD_FILES_DIR}/board.c" "f28885b9ac349a06a6b38f0376f6872a25ec131ab7d37947a0576255579cd959"
fetch_and_verify "${RAW_BASE}/board.h" "${BOARD_FILES_DIR}/board.h" "9c25debd61b7fc153eeedd569155dfc8f9d1349192c6de2b431742ee6f9bb17e"
fetch_and_verify "${RAW_BASE}/project_template/clock_config.c" "${BOARD_FILES_DIR}/clock_config.c" "ca20b253229ef02e74a9d0041173c5e1fc0c5c3df048a53be8b44e1e4218ef03"
fetch_and_verify "${RAW_BASE}/project_template/clock_config.h" "${BOARD_FILES_DIR}/clock_config.h" "52036470ef08b16daf7ed1382a8c3c0bcc123336afc2b07df807e793365b3e80"
fetch_and_verify "${RAW_BASE}/project_template/pin_mux.c" "${BOARD_FILES_DIR}/pin_mux.c" "4bf784e2685555e6297adccb78a27de5754e5320911bc503b5ab563577599c39"
fetch_and_verify "${RAW_BASE}/project_template/pin_mux.h" "${BOARD_FILES_DIR}/pin_mux.h" "f696267090a271e12d9c9f3ccb0e2dfb721025ddd3728b32ab33a9fb79acbc70"
fetch_and_verify "${RAW_BASE}/dcd.c" "${BOARD_FILES_DIR}/dcd.c" "798cd3fffea9b3b1917d6750d40735b6bc890e770f8d8b9167238220b0fae21f"
fetch_and_verify "${RAW_BASE}/dcd.h" "${BOARD_FILES_DIR}/dcd.h" "3a5268f0ccdc02aa6df55b3fca87171df35161cca0c3e15971ac082cdefde7a3"
fetch_and_verify "${RAW_BASE}/xip/evkmimxrt1064_flexspi_nor_config.c" "${BOARD_FILES_DIR}/evkmimxrt1064_flexspi_nor_config.c" "f6fa3d1e3a09c1a4a9d3fc44aed23513e12341e6db96aa3427c923e6b41c6e46"
fetch_and_verify "${RAW_BASE}/xip/evkmimxrt1064_flexspi_nor_config.h" "${BOARD_FILES_DIR}/evkmimxrt1064_flexspi_nor_config.h" "4073f8c6e09fccc879dcedb6fe79f679bc8c9840bb90a7f527279a9a021813d3"

echo "[OK] Board support files verified & downloaded"
echo ""

# 3. Fetch CMSIS Core headers (pinned ARM.CMSIS 5.9.0 release pack from ARM-software/CMSIS_5)
CMSIS_VERSION="5.9.0"
CMSIS_PACK_URL="https://github.com/ARM-software/CMSIS_5/releases/download/${CMSIS_VERSION}/ARM.CMSIS.${CMSIS_VERSION}.pack"
CMSIS_PACK_SHA256="14b366f2821ee5d32f0d3bf48ef9657ca45347261d0531263580848e9d36f8f4"
CMSIS_PACK_ZIP="${TEMP_DIR}/cmsis.zip"
CMSIS_EXTRACT="${TEMP_DIR}/cmsis_extracted"

echo "[INFO] Downloading official ARM CMSIS Pack (v${CMSIS_VERSION})..."
fetch_and_verify "${CMSIS_PACK_URL}" "${CMSIS_PACK_ZIP}" "${CMSIS_PACK_SHA256}"
echo "[OK] ARM CMSIS Pack verified (SHA256: ${CMSIS_PACK_SHA256})"

echo "[INFO] Extracting CMSIS Core headers..."
mkdir -p "${CMSIS_EXTRACT}"
unzip -q "${CMSIS_PACK_ZIP}" "CMSIS/Core/Include/*" -d "${CMSIS_EXTRACT}"
cp -r "${CMSIS_EXTRACT}/CMSIS/Core/Include/"* "${CMSIS_INCLUDE_DEST}/"
echo "[OK] CMSIS Core headers copied"
echo ""

echo "[INFO] Note: NetX Duo Ethernet driver and KSZ8081 PHY driver are vendored in lib/netx_driver and lib/phyksz8081."
echo ""
echo "=========================================="
echo "[SUCCESS] NXP i.MX RT1064 SDK dependencies successfully fetched & verified!"
echo "=========================================="
