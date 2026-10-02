#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
audit_dir=$(mktemp -d)
trap 'rm -rf -- "$audit_dir"' EXIT
cd "$audit_dir"
for platform in posix esp-branch; do
    flags=()
    if [[ "$platform" == esp-branch ]]; then flags+=(-DESP_PLATFORM); fi
    "${CC:-cc}" -std=c11 -O1 -g -Wall -Wextra -Werror \
        -fsanitize=address,undefined -fno-omit-frame-pointer "${flags[@]}" \
        -I"$root/components/ink_browser" "$root/components/ink_browser/ink_editor.c" \
        "$root/components/ink_browser/ink_keyboard.c" "$root/prototypes/browser/editor_audit.c" \
        -Wl,--wrap=rename -o audit
    # This execution environment cannot inspect /proc for LeakSanitizer.
    # AddressSanitizer and UndefinedBehaviorSanitizer remain enabled.
    ASAN_OPTIONS=detect_leaks=0 ./audit
done
