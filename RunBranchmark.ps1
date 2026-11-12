param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    [string]$BuildDir = (Join-Path $PSScriptRoot "out/branchmark-build")
)

$ErrorActionPreference = "Stop"
$repoDir = $PSScriptRoot
$buildDir = [System.IO.Path]::GetFullPath($BuildDir)

cmake -S $repoDir -B $buildDir -DBUILD_TESTING=OFF -DAYR_BUILD_BRANCHMARKS=ON "-DCMAKE_BUILD_TYPE=$Config"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build $buildDir --config $Config --target branchmarks --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$executableDir = Join-Path $buildDir "branchmark/$Config"
$benchmarks = @(Get-ChildItem -LiteralPath $executableDir -Filter '*_benchmark.exe' -File | Sort-Object Name)
if ($benchmarks.Count -eq 0) { throw "No benchmark executables found in $executableDir" }

$reportDir = Join-Path $repoDir "branchmark"
New-Item -ItemType Directory -Force -Path $reportDir | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$reportPath = Join-Path $reportDir "$stamp.md"
$logPath = Join-Path $reportDir "$stamp.log"
$header = @"
# AYR performance report

- Run: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')
- Configuration: $Config
- Times are milliseconds; comparisons use sample medians.
- STD/AYR > 1 means AYR is faster; negative AYR time change means less time than STD.
- Min/max show observed sample ranges. Faster labels describe this run, not statistical significance.

"@
[System.IO.File]::WriteAllText($reportPath, $header, [System.Text.UTF8Encoding]::new($false))
$previousReport = $env:AYR_BRANCHMARK_REPORT
$completed = $false
try {
    $env:AYR_BRANCHMARK_REPORT = "$stamp.md"
    foreach ($benchmark in $benchmarks) {
        Write-Host "Running: $($benchmark.Name)"
        & $benchmark.FullName 2>&1 | Tee-Object -FilePath $logPath -Append
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    $completed = $true
}
finally {
    $env:AYR_BRANCHMARK_REPORT = $previousReport
    $status = if ($completed) { 'PASS' } else { 'FAIL / incomplete' }
    Add-Content -LiteralPath $reportPath -Value "`nOverall run: **$status**." -Encoding utf8
    Copy-Item -LiteralPath $reportPath -Destination (Join-Path $reportDir 'latest.md') -Force
    Write-Host "Report: $reportPath"
    Write-Host "Log:    $logPath"
}
