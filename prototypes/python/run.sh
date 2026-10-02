#!/usr/bin/env bash
set -euo pipefail
start_dir=$PWD
cd "$(dirname "$0")"
rev=19e685eca906a5a602135a485976253e705297d0
deps=.deps/micropython
if [[ ! -d "$deps/.git" ]]; then
    mkdir -p "$deps"
    git -C "$deps" init -q
    git -C "$deps" fetch -q --depth 1 https://github.com/micropython/micropython.git "$rev"
    git -C "$deps" checkout -q --detach FETCH_HEAD
fi
[[ "$(git -C "$deps" rev-parse HEAD)" == "$rev" ]]
[[ -z "$(git -C "$deps" status --porcelain --untracked-files=no)" ]]
mkdir -p build
# Fresh generated headers avoid stale module registrations after config changes.
gen_dir=$(mktemp -d "$PWD/build/gen-XXXXXX")
trap 'rm -r -- "$gen_dir"' EXIT
make -f generate.mk BUILD="$gen_dir" PACKAGE_DIR=build/micropython_embed >/dev/null
rm -r -- "$gen_dir"
trap - EXIT
make -j 4 >/dev/null
program="$PWD/build/inkpy-python"
cd "$start_dir"
exec "$program" "$@"
