#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${IDF_PATH:?Source the pinned ESP-IDF export.sh first}"
expected=b774170ff46c393eeb5e495ea37936038d3f4f4f
[[ "$(git -C "$IDF_PATH" rev-parse HEAD)" == "$expected" ]] || { echo 'Wrong ESP-IDF revision' >&2; exit 1; }
bash scripts/fetch-math.sh
export IDF_COMPONENT_MANAGER=0
export IDF_PY_BUILD_JOBS="${IDF_PY_BUILD_JOBS:-4}"
# A separate sdkconfig/build prevents diagnostic allocator settings leaking into normal builds.
idf.py -B build-math -DIDF_TARGET=esp32s3 -DINKPY_MATH_DIAGNOSTIC=ON \
    -DSDKCONFIG="$PWD/build-math/sdkconfig" \
    -DSDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.math.defaults' build
