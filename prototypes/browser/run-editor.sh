#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
"${CC:-cc}" -std=c11 -D_FILE_OFFSET_BITS=64 -O2 -Wall -Wextra -Werror \
    -I../../components/ink_browser ../../components/ink_browser/ink_editor.c \
    ../../components/ink_browser/ink_keyboard.c \
    editor_probe.c -o build/editor-probe
build/editor-probe
# Exercise the device backup/replacement branch on host too (not FatFs runtime).
"${CC:-cc}" -std=c11 -D_FILE_OFFSET_BITS=64 -DESP_PLATFORM -O2 -Wall -Wextra -Werror \
    -I../../components/ink_browser ../../components/ink_browser/ink_editor.c \
    ../../components/ink_browser/ink_keyboard.c editor_probe.c -o build/editor-device-save-probe
build/editor-device-save-probe
