#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
bash ../../scripts/fetch-math.sh
cmake -S . -B build -G Ninja
cmake --build build --parallel 4
mkdir -p build/results
"${PYTHON:-python3}" ../../scripts/prepare-math-sd.py .deps/MicroTeX build/sd
build/math-probe "$PWD/build/sd/inkpy/math" ../../fixtures/markdown-math.md build/results > build/results.tsv 2> build/metrics.txt
cat build/metrics.txt
"${PYTHON:-python3}" preview.py build/results.tsv build/results build/preview.png

