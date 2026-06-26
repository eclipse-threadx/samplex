param(
    [string]$Destination = "D:"
)

$AZ3166_DIR = Resolve-Path "$PSScriptRoot/.."
$BUILD_DIR = Join-Path $AZ3166_DIR "build"

if (!(Test-Path $Destination)) {
    Write-Host "[ERROR] Destination $Destination does not exist!" -ForegroundColor Red
    exit 1
}

$Binary = Get-ChildItem -Path "$BUILD_DIR/app/*.bin" | Select-Object -First 1
if ($Binary) {
    Write-Host "[INFO] Copying $($Binary.Name) to $Destination..."
    Copy-Item -Path $Binary.FullName -Destination $Destination -Force
    Write-Host "[OK] Deployment successful!"
} else {
    Write-Host "[ERROR] No binary found in $BUILD_DIR/app/*.bin" -ForegroundColor Red
    exit 1
}
