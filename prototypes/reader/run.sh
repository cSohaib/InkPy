#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
rev=c7ba975c34d714966ea910c58a97524e5d674ff7
if [[ ! -d .deps/md4c/.git ]]; then
    mkdir -p .deps/md4c
    git -C .deps/md4c init -q
    git -C .deps/md4c fetch -q --depth 1 https://github.com/mity/md4c.git "$rev"
    git -C .deps/md4c checkout -q --detach FETCH_HEAD
fi
[[ "$(git -C .deps/md4c rev-parse HEAD)" == "$rev" ]]
[[ -z "$(git -C .deps/md4c status --porcelain --untracked-files=no)" ]]
mkdir -p build
cc=${CC:-cc}
flags=(-std=c11 -O2 -g -D_FILE_OFFSET_BITS=64 -Wall -Wextra -Werror)
core=../../components/ink_layout
"$cc" "${flags[@]}" -Wno-unused-parameter -I"$core" -include "$core/md_budget.h" \
    -Dmalloc=ink_md_malloc -Drealloc=ink_md_realloc -Dfree=ink_md_free \
    -c .deps/md4c/src/md4c.c -o build/md4c.o
"$cc" "${flags[@]}" -I"$core" -I.deps/md4c/src \
    -I"$core/vendor/fribidi" -DDONT_HAVE_FRIBIDI_CONFIG_H -DHAVE_STDLIB_H -DHAVE_STRING_H -DHAVE_STRINGS_H -DHAVE_STRINGIZE -DSTDC_HEADERS=1 \
    "$core/ink_layout.c" "$core/ink_bidi.c" "$core/md_budget.c" \
    "$core"/vendor/fribidi/*.c probe.c build/md4c.o -o build/reader-probe
"${PYTHON:-python3}" tests.py
