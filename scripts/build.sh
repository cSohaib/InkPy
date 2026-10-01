#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${IDF_PATH:?Source the pinned ESP-IDF export.sh first}"
expected=b774170ff46c393eeb5e495ea37936038d3f4f4f
actual=$(git -C "$IDF_PATH" rev-parse HEAD)
if [[ "$actual" != "$expected" ]]; then
    echo "InkPy requires ESP-IDF v5.5.5 at $expected; found $actual" >&2
    exit 1
fi
# This stage has no managed components; keep dependency discovery deterministic.
export IDF_COMPONENT_MANAGER=0
idf.py -DIDF_TARGET=esp32s3 build
