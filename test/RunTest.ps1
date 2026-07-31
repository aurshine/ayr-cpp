param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"

$repoDir = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoDir "out/build"

if (Test-Path $buildDir) {
    Write-Host "Deleting: $buildDir"
    Remove-Item $buildDir -Recurse -Force
}

cmake -S $repoDir -B $buildDir -DBUILD_TESTING=ON
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

cmake --build $buildDir --config $Config --parallel
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

ctest --test-dir $buildDir -C $Config --output-on-failure
exit $LASTEXITCODE
