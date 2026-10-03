# First hardware feedback — 2026-10-03 (Luxembourg)

User reports: unlocked X4 Pro, InkPy installed through CrossPoint web installer;
boot/browser worked, expected pending features were visible, screen was upside
down. After trying sleep/wake, CrossPoint booted instead. No observed damage.
This is user-reported hardware evidence, not a serial trace or a complete test.

## Corrections

- `main/boot.c`: report reset reason, running app slot/address and OTA state.
  After successful board/display/frame initialization and first full refresh,
  confirm NEW/PENDING_VERIFY firmware using the ESP-IDF API. Earlier startup
  failures do not confirm it. Existing bootloader and other app slot remain intact.
  Compile guard rejects anti-rollback configuration to prevent this confirmation
  path from introducing eFuse secure-version updates.
- `main/orientation.h`: rotate device framebuffer 180 degrees, and apply matching
  panel-to-UI coordinates for short and long touches. Covers browser, keyboard,
  editor, viewer and notices; host rendering coordinates remain unchanged.
- `main/sleep.c`: log returned light-sleep error and wake cause. Boot logs distinguish
  a fresh reset from a successful light-sleep return. Sleep behavior itself is
  unchanged because the reset cause is not yet known.

The installer currently writes OTA state NEW. InkPy previously lacked successful
boot confirmation. With a rollback-enabled installed bootloader, a reset while
PENDING_VERIFY causes fallback to the older app. This is the leading explanation
for CrossPoint returning; actual bootloader configuration and reset reason have
not been captured. It was missed by the earlier offline audit.

Source: https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32s3/api-reference/system/ota.html
Installer inspected: https://crosspointreader.com/assets/index-DasDSmEt.js

## Validation and next device check

Pinned ESP-IDF v5.5.5 / Xtensa GCC 14.2.0 browser build passed with own sources
under warnings-as-errors. New image: 356576 bytes; checksum and SHA-256 trailer
valid. SHA-256 `e123b05a90bb0b58ede9c4c46238f34bc373260183f81dccce5d126739e90fae`.
Logs in `results/boot-orientation/`. The native toolchain was restored into a
workspace cache after runtime-owned tools disappeared; dependency versions unchanged.

`orientation_probe.c` checks every one of the 384000 pixels, corresponding touch
coordinates, and byte-exact restoration after two rotations. Compile/run:

```
cc -std=c11 -Wall -Wextra -Werror -Imain prototypes/browser/orientation_probe.c -o /tmp/inkpy-orientation
/tmp/inkpy-orientation
```

Install the updated application through X4 Pro / Custom .bin, keeping backups.
Check upright text and aligned taps, then reboot to check InkPy remains selected.
Test sleep/wake separately. If it restarts, capture serial boot/reset and sleep
logs through the web tool's serial monitor where available; do not erase flash or
replace the bootloader to suppress fallback. Once confirmed valid, automatic
first-boot fallback no longer applies; ordinary USB recovery remains necessary.
The new image still needs these hardware checks. No claim that sleep reset is fixed.


## Stage 26: confirmed startup crash

The uploaded OTA records and USB log establish rollback of Stage 25 before
app_main, after heap corruption in the MicroTeX symbol constructor. Shared
CMake overlays replace four large initializer-list tables with incremental
insertion, preserving entries and leaving the cached upstream source intact.
The symbol constructor frame is now 80 bytes. Build/image/host reader checks
pass; device verification remains pending. See results/stage26/boot-fix.txt and
the current checkpoint for evidence and the new application hash.
