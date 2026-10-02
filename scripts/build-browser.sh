#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# Separate config avoids silently inheriting diagnostic/old stack defaults.
bash scripts/build.sh -B build-browser -DSDKCONFIG="$PWD/build-browser/sdkconfig" -DINKPY_BROWSER=ON -DINKPY_MATH_DIAGNOSTIC=OFF
cp build-browser/inkpy.bin build-browser/firmware.bin
sha256sum build-browser/firmware.bin
