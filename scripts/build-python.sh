#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/prepare-python-device.sh
bash scripts/build.sh -B build-python -DSDKCONFIG="$PWD/build-python/sdkconfig" \
    -DINKPY_PYTHON_DIAGNOSTIC=ON -DINKPY_BROWSER=OFF -DINKPY_MATH_DIAGNOSTIC=OFF
# Diagnostic only: intentionally do not replace the user-facing firmware.bin.
