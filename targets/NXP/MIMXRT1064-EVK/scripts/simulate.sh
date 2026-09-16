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

DEMO_ARG=""
RESC_ARG=""
SEED_ARG=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--demo)
            DEMO_ARG="$2"
            shift 2
            ;;
        -s|--seed)
            SEED_ARG="$2"
            shift 2
            ;;
        -r|--resc)
            RESC_ARG="$2"
            shift 2
            ;;
        *)
            if [ -z "${RESC_ARG}" ] && [[ "$1" == *.resc ]]; then
                RESC_ARG="$1"
            else
                echo "Unknown parameter: $1"
                exit 1
            fi
            shift
            ;;
    esac
done

# 1. Resolve which demo to simulate
SELECTED_DEMO="${DEMO_ARG}"
if [ -z "${SELECTED_DEMO}" ] && [ -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    CACHED_DEMO=$(grep -E "^ACTIVE_DEMO:STRING=" "${BUILD_DIR}/CMakeCache.txt" | cut -d'=' -f2 | tr -d ' \r\n')
    if [ -n "${CACHED_DEMO}" ] && [ "${CACHED_DEMO}" != "all" ]; then
        SELECTED_DEMO="${CACHED_DEMO}"
    fi
fi

if [ -z "${SELECTED_DEMO}" ]; then
    if [ -f "${BUILD_DIR}/app/demos/threadx_basic/mimxrt1064_threadx.elf" ]; then
        SELECTED_DEMO="threadx_basic"
    elif [ -f "${BUILD_DIR}/app/demos/netx_echo/mimxrt1064_threadx.elf" ]; then
        SELECTED_DEMO="netx_echo"
    elif [ -f "${BUILD_DIR}/app/demos/netx_trng_console/mimxrt1064_threadx.elf" ]; then
        SELECTED_DEMO="netx_trng_console"
    else
        SELECTED_DEMO="threadx_basic"
    fi
fi

# 2. Locate firmware binaries for the selected demo
DEMO_DIR="${BUILD_DIR}/app/demos/${SELECTED_DEMO}"
SERVER_ELF="${DEMO_DIR}/mimxrt1064_threadx.elf"
CLIENT_ELF="${DEMO_DIR}/mimxrt1064_client.elf"

# Fallback to root build dir if per-demo subfolder does not exist
if [ ! -f "${SERVER_ELF}" ] && [ -f "${BUILD_DIR}/mimxrt1064_threadx.elf" ]; then
    SERVER_ELF="${BUILD_DIR}/mimxrt1064_threadx.elf"
    CLIENT_ELF="${BUILD_DIR}/mimxrt1064_client.elf"
fi

if [ ! -f "${SERVER_ELF}" ]; then
    echo "[ERROR] Firmware binary for demo '${SELECTED_DEMO}' not found at:"
    echo "  ${SERVER_ELF}"
    echo ""
    echo "Please build the demo first using:"
    echo "  ./scripts/build.sh -d ${SELECTED_DEMO}"
    exit 1
fi

# 3. Select Renode script and verification mode
if [ -n "${RESC_ARG}" ]; then
    RESC_REL_PATH="${RESC_ARG}"
    MODE="Custom Script (${RESC_ARG})"
elif [ "$SELECTED_DEMO" = "netx_trng_console" ]; then
    RESC_REL_PATH="renode/mimxrt1064-trng-console.resc"
    MODE="Hardware TRNG Console (Server: 192.168.0.100, Client: 192.168.0.101)"
elif [ "$SELECTED_DEMO" = "netx_echo" ]; then
    RESC_REL_PATH="renode/mimxrt1064-network-multinode.resc"
    MODE="Multi-Node Network Echo (Server: 192.168.0.100, Client: 192.168.0.101)"
else
    RESC_REL_PATH="renode/mimxrt1064-evk.resc"
    MODE="ThreadX Core Basic Demo (Single-Node)"
fi

# 4. Find Renode executable
RENODE_CMD="renode"
if ! command -v renode &> /dev/null; then
    if [ -f "/opt/renode/renode" ]; then
        RENODE_CMD="/opt/renode/renode"
    else
        echo "[ERROR] Renode was not found in PATH."
        exit 1
    fi
fi

echo "=========================================="
echo "Starting Renode Simulation"
echo "=========================================="
echo "Renode:      ${RENODE_CMD}"
echo "Demo:        ${SELECTED_DEMO}"
echo "Mode:        ${MODE}"
echo "Script:      ${BOARD_DIR}/${RESC_REL_PATH}"
if [ -n "${SEED_ARG}" ]; then
    echo "Seed:        ${SEED_ARG} (Deterministic)"
fi
echo "Server ELF:  ${SERVER_ELF}"
if [ -f "${CLIENT_ELF}" ]; then
    echo "Client ELF:  ${CLIENT_ELF}"
fi
echo ""

cd "${BOARD_DIR}"

# 5. Build Renode execution command passing explicit binary paths
RENODE_EXEC_CMD=""
if [ -n "${SEED_ARG}" ]; then
    RENODE_EXEC_CMD="emulation SetSeed ${SEED_ARG}; "
fi
RENODE_EXEC_CMD="${RENODE_EXEC_CMD}\$bin = @\"${SERVER_ELF}\"; \$bin_server = @\"${SERVER_ELF}\"; "
if [ -f "${CLIENT_ELF}" ]; then
    RENODE_EXEC_CMD="${RENODE_EXEC_CMD}\$bin_client = @\"${CLIENT_ELF}\"; "
fi
RENODE_EXEC_CMD="${RENODE_EXEC_CMD}include @\"${RESC_REL_PATH}\""

"${RENODE_CMD}" -e "${RENODE_EXEC_CMD}"
