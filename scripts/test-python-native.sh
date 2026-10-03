#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# Reuses device-generated headers; no upstream regeneration or download.
mkdir -p prototypes/python/build/native-sd
cp fixtures/python-files.py prototypes/python/build/native-sd/test.py
printf 'answer = 42\n' > prototypes/python/build/native-sd/local_helper.py
root="$PWD/prototypes/python/build/native-sd"
mapfile -t vendor < <(find generated-python/embed/py generated-python/embed/port generated-python/embed/shared -name '*.c' ! -name mphalport.c)
"${CC:-cc}" -std=gnu11 -O2 -Wall -Wextra -Wno-unused-parameter -DNDEBUG \
    -DINK_PY_NATIVE=1 -DINK_PY_ROOT="\"$root\"" \
    -Imain/python_port -Igenerated-python/embed -Igenerated-python/embed/port -Icomponents/ink_python \
    "${vendor[@]}" generated-python/embed/extmod/modjson.c components/ink_python/ink_python.c \
    main/python_port/files.c main/python_port/bindings.c prototypes/python/native_stubs.c \
    -UNDEBUG prototypes/python/native_probe.c -lm -o prototypes/python/build/native-files
prototypes/python/build/native-files "$root/test.py"
