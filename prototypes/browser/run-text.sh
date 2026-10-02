#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
"${CC:-cc}" -std=c11 -D_FILE_OFFSET_BITS=64 -O2 -Wall -Wextra -Werror -I../../components/ink_browser \
    ../../components/ink_browser/ink_browser.c ../../components/ink_browser/browser_draw.c \
    ../../components/ink_browser/ink_text.c text_probe.c -o build/text-probe
build/text-probe ../../fixtures/plain-text.txt build/text.pbm
