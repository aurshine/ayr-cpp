#!/usr/bin/env bash

set -euo pipefail

config="${1:-Debug}"

case "$config" in
    Debug|Release)
        ;;
    *)
        echo "Usage: $0 [Debug|Release]" >&2
        exit 2
        ;;
esac

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
repo_dir="$(cd -- "$script_dir/.." && pwd -P)"
build_dir="$repo_dir/out/build"

if [[ -e "$build_dir" ]]; then
    echo "Deleting: $build_dir"
    rm -rf -- "$build_dir"
fi

cmake \
    -S "$repo_dir" \
    -B "$build_dir" \
    -DBUILD_TESTING=ON \
    -DCMAKE_BUILD_TYPE="$config"

cmake --build "$build_dir" --config "$config" --parallel

ctest \
    --test-dir "$build_dir" \
    -C "$config" \
    --output-on-failure
