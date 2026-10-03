#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
deps="$root/prototypes/python/.deps/micropython"
rev=19e685eca906a5a602135a485976253e705297d0
if [[ ! -d "$deps/.git" ]]; then
    mkdir -p "$deps"
    git -C "$deps" init -q
    git -C "$deps" fetch -q --depth 1 https://github.com/micropython/micropython.git "$rev"
    git -C "$deps" checkout -q --detach FETCH_HEAD
fi
[[ "$(git -C "$deps" rev-parse HEAD)" == "$rev" ]]
[[ -z "$(git -C "$deps" status --porcelain --untracked-files=no)" ]]
mkdir -p "$root/generated-python"
gen=$(mktemp -d "$root/generated-python/gen-XXXXXX")
trap 'python3 -c "import shutil,sys; shutil.rmtree(sys.argv[1])" "$gen"' EXIT
make -s -C "$root/main/python_port" -f generate.mk MICROPYTHON_TOP="$deps" \
    BUILD="$gen" PACKAGE_DIR="$root/generated-python/embed"
cp "$deps/extmod/modjson.c" "$root/generated-python/embed/extmod/modjson.c"

mkdir -p "$root/generated-python/embed/shared/readline"
cp "$deps/shared/readline/readline.h" "$root/generated-python/embed/shared/readline/readline.h"
