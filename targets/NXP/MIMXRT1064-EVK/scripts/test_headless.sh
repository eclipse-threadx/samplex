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

PYTHON_BIN=""
if command -v python3 &>/dev/null && python3 --version &>/dev/null; then
    PYTHON_BIN="python3"
elif command -v python &>/dev/null && python --version &>/dev/null; then
    PYTHON_BIN="python"
elif command -v py &>/dev/null && py -3 --version &>/dev/null; then
    PYTHON_BIN="py -3"
else
    echo "[FAIL] Python was not found in PATH."
    exit 1
fi

exec ${PYTHON_BIN} "${SCRIPT_DIR}/test_renode.py" "$@"
