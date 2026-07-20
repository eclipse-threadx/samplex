#!/usr/bin/env bash
# /***************************************************************************/
# /* Copyright (C) 2026 Eclipse ThreadX contributors
#  *
#  * This program and the accompanying materials are made available under the
#  * terms of the MIT License which is available at
#  * https://opensource.org/licenses/MIT.
#  *
#  * SPDX-License-Identifier: MIT
#  ***************************************************************************/

# update-libs.sh -- Advance shared Eclipse ThreadX library submodules to the
#                   latest commit on their tracked branch.
#
# During development the tracked branch is 'dev' (set in .gitmodules).
# At release time, change the branch entries in .gitmodules to 'master' and
# run this script once to pin the released versions.
#
# Usage:
#   bash scripts/update-libs.sh
#
# After running, review the diff and commit:
#   git add libs/
#   git commit -m "chore: bump libs to latest dev"

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${REPO_ROOT}"

echo "[INFO] Updating shared library submodules to latest tracked branch..."
git submodule update --remote libs/threadx libs/netxduo libs/usbx libs/filex

echo ""
echo "[INFO] Current submodule state:"
git submodule status libs/

echo ""
echo "[INFO] Done. Review changes with 'git diff --submodule libs/' then commit."
