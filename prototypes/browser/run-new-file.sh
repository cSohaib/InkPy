#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
"${CC:-cc}" -std=c11 -D_FILE_OFFSET_BITS=64 -O2 -Wall -Wextra -Werror -I../../components/ink_browser \
    ../../components/ink_browser/ink_browser.c ../../components/ink_browser/browser_draw.c \
    ../../components/ink_browser/ink_text.c ../../components/ink_browser/ink_keyboard.c \
    new_file_probe.c -o build/new-file-probe
sample_dir=$(mktemp -d)
trap 'rm -r -- "$sample_dir"' EXIT
build/new-file-probe "$sample_dir" build/new-file.pbm
