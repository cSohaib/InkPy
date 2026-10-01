#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
test_bin=$(mktemp)
trap 'rm -f "$test_bin"' EXIT
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I main main/input.c tests/input_test.c -o "$test_bin"
"$test_bin"
