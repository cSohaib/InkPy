#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
# Sources stay disposable; every build verifies exact revisions and no tracked edits.
fetch() {
    local name="$1" url="$2" rev="$3"
    if [[ ! -d ".deps/$name/.git" ]]; then
        mkdir -p ".deps/$name"
        git -C ".deps/$name" init -q
        git -C ".deps/$name" fetch -q --depth 1 "$url" "$rev"
        git -C ".deps/$name" checkout -q --detach FETCH_HEAD
    fi
    [[ "$(git -C ".deps/$name" rev-parse HEAD)" == "$rev" ]] || { echo "Wrong revision: $name" >&2; exit 1; }
    [[ -z "$(git -C ".deps/$name" status --porcelain --untracked-files=no)" ]] || { echo "Modified dependency: $name" >&2; exit 1; }
}
fetch MicroTeX https://github.com/NanoMichael/MicroTeX.git 0e3707f6dafebb121d98b53c64364d16fefe481d
fetch freetype https://github.com/freetype/freetype.git 42608f77f20749dd6ddc9e0536788eaad70ea4b5
fetch tinyxml2 https://github.com/leethomason/tinyxml2.git 321ea883b7190d4e85cae5512a12e5eaa8f8731f
fetch md4c https://github.com/mity/md4c.git c7ba975c34d714966ea910c58a97524e5d674ff7
cmake -S . -B build -G Ninja
cmake --build build --parallel 4
mkdir -p build/results build/res
# Test without optional Greek/Cyrillic language resources.
cp -R .deps/MicroTeX/res/fonts build/res/
build/math-probe "$PWD/build/res" ../../fixtures/markdown-math.md build/results > build/results.tsv 2> build/metrics.txt
cat build/metrics.txt
"${PYTHON:-python3}" preview.py build/results.tsv build/results build/preview.png
