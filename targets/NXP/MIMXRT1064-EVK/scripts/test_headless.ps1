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
    [string]$Demo = "threadx_basic",
    [int]$TimeoutSeconds = 300,
    [Nullable[int]]$Seed
)

$scriptPath = Join-Path $PSScriptRoot "test_renode.py"

$pythonArgs = @($scriptPath, "--demo", $Demo, "--timeout", $TimeoutSeconds)
if ($null -ne $Seed) {
    $pythonArgs += @("--seed", $Seed)
}

$pythonExe = (Get-Command python3 -ErrorAction SilentlyContinue).Source
if (-not $pythonExe) {
    $pythonExe = (Get-Command python -ErrorAction SilentlyContinue).Source
}
if (-not $pythonExe) {
    Write-Error "[FAIL] Python was not found in PATH."
    exit 1
}

& $pythonExe @pythonArgs
exit $LASTEXITCODE
