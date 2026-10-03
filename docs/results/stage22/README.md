# Stage 22 validation

- Product browser + MicroPython + input capture: native build/link passed.
- Separate Python diagnostic: native build/link passed after the NLR change.
- `cc -std=c11 -Wall -Wextra -Werror -Imain scripts/test-input-events.c main/input_events.c main/input.c` and resulting binary passed.
- `python prototypes/python/smoke.py` passed against the cached host embedding.
- Host console/browser-console targets rebuilt; both workflow programs passed.
- Esptool image_info verified application checksum and validation hash; symbol report confirms VM/UI/input paths linked.
- `git diff --check` passed.

These are compilation and desktop checks, not execution of the native FreeRTOS
adapter. No device was connected/flashed. Runtime memory/stack, physical rapid
input, panel refresh and sleep/resume still require physical testing. Binaries
remain in disposable build folders; the existing downloadable image is unchanged.
