#!/bin/bash

# Change the path to where the AZ3166 is mounted on your system.
DESTINATION=${1:-/Volumes/AZ3166}

# Get the absolute path to the AZ3166 directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$(cd "${SCRIPT_DIR}/../build" && pwd)"

if [ ! -d "${DESTINATION}" ]; then
    echo "[ERROR] Destination ${DESTINATION} does not exist!"
    exit 1
fi

BINARY=$(ls ${BUILD_DIR}/app/*.bin | head -n 1)

if [ -f "${BINARY}" ]; then
    echo "[INFO] Copying ${BINARY} to ${DESTINATION}..."
    cp "${BINARY}" "${DESTINATION}"
    echo "[OK] Deployment successful!"
else
    echo "[ERROR] No binary found in ${BUILD_DIR}/app/*.bin"
    exit 1
fi
