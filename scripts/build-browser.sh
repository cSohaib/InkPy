#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/build.sh -B build-browser -DINKPY_BROWSER=ON -DINKPY_MATH_DIAGNOSTIC=OFF
