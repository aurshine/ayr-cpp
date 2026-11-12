#!/usr/bin/env bash

set -euo pipefail

config="${1:-Release}"
case "$config" in
    Debug|Release) ;;
    *)
        echo "Usage: $0 [Debug|Release] [build-directory]" >&2
        exit 2
        ;;
esac

repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
build_dir="${2:-$repo_dir/out/branchmark-build}"

cmake -S "$repo_dir" -B "$build_dir" \
    -DBUILD_TESTING=OFF -DAYR_BUILD_BRANCHMARKS=ON -DCMAKE_BUILD_TYPE="$config"
cmake --build "$build_dir" --config "$config" --target branchmarks --parallel

shopt -s nullglob
benchmarks=("$build_dir/branchmark/$config/"*_benchmark)
if (( ${#benchmarks[@]} == 0 )); then
    echo "No benchmark executables found in $build_dir/branchmark/$config" >&2
    exit 1
fi
report_dir="$repo_dir/branchmark"
mkdir -p -- "$report_dir"
stamp="$(date +%Y%m%d-%H%M%S)"
report_path="$report_dir/$stamp.md"
log_path="$report_dir/$stamp.log"
cat > "$report_path" <<EOF
# AYR performance report

- Run: $(date '+%Y-%m-%d %H:%M:%S %z')
- Configuration: $config
- Times are milliseconds; comparisons use sample medians.
- STD/AYR > 1 means AYR is faster; negative AYR time change means less time than STD.
- Min/max show observed sample ranges. Faster labels describe this run, not statistical significance.

EOF
export AYR_BRANCHMARK_REPORT="$stamp.md"
finish_report() {
    result=$?
    if (( result == 0 )); then
        printf '\nOverall run: **PASS**.\n' >> "$report_path"
    else
        printf '\nOverall run: **FAIL / incomplete**.\n' >> "$report_path"
    fi
    cp -- "$report_path" "$report_dir/latest.md"
    echo "Report: $report_path"
    echo "Log:    $log_path"
}
trap finish_report EXIT
for benchmark in "${benchmarks[@]}"; do
    echo "Running: $(basename -- "$benchmark")"
    "$benchmark" 2>&1 | tee -a "$log_path"
done
