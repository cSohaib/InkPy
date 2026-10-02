#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
bash scripts/fetch-math.sh
cmake_bin=${CMAKE:-cmake}
"$cmake_bin" -S prototypes/reader -B prototypes/reader/build/math
"$cmake_bin" --build prototypes/reader/build/math -j 4
export INK_MATH_RESOURCES="$PWD/prototypes/math/.deps/MicroTeX/res"
# Run with SOURCE NEW_OUTPUT_DIR, optionally WIDTH HEIGHT READ_BYTES.
prototypes/reader/build/math/reader-math "$@"
