#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
"${CC:-cc}" -std=c11 -D_FILE_OFFSET_BITS=64 -O2 -Wall -Wextra -Werror -I../../components/ink_browser \
    ../../components/ink_browser/ink_browser.c ../../components/ink_browser/browser_draw.c ../../components/ink_browser/ink_text.c ../../components/ink_browser/ink_keyboard.c probe.c -o build/browser-probe
sample_dir=$(mktemp -d)
trap 'rm -r -- "$sample_dir"' EXIT
mkdir "$sample_dir/Books"
printf '# A book\n' > "$sample_dir/Books/book.md"
printf 'print(1)\n' > "$sample_dir/Books/script.py"
printf '\000\001' > "$sample_dir/Books/binary.bin"
for ((i=1;i<=19;i++)); do printf 'hello\n' > "$sample_dir/Notes-$i.txt"; done
build/browser-probe "$sample_dir" build/browser.pbm
