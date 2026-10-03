#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/prepare-python-device.sh
# Separate config avoids silently inheriting diagnostic/old stack defaults.
bash scripts/build.sh -B build-browser -DSDKCONFIG="$PWD/build-browser/sdkconfig" -DINKPY_PYTHON_DIAGNOSTIC=OFF -DINKPY_BROWSER=ON -DINKPY_MATH_DIAGNOSTIC=OFF
cp build-browser/inkpy.bin build-browser/firmware.bin
sha256sum build-browser/firmware.bin
