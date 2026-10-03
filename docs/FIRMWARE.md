# Stage 20 firmware checkpoint

The user confirms an unlocked X4 Pro and reports a successful Stage 19 installation
through CrossPoint's web installer. Stage 19.1 corrects upside-down orientation and
adds successful-first-boot confirmation plus reset/slot/sleep logging; see
[the hardware feedback and fixes](BOOT-FIX.md). Sleep caused a return to CrossPoint
on the previous image; the underlying reset cause remains unknown. This updated
image still needs hardware checking. Do not install on USB-locked devices.

`firmware.bin` is an ESP32-S3 **application image** for the X4 Pro. It does not
contain a bootloader or partition table and is not a merged full-flash image.
This revised image has compiled and passed checksum/hash validation; its predecessor
was tested by the user, but these corrections still need physical verification.
Use the X4 Pro custom application-image installer, retaining the existing bootloader.

Stage 20 incorporates device feedback: dot-files/folders hidden, InkPy title removed,
clean editor with bottom keyboard and filename only in its Save/Discard/Cancel menu.
Power menu now opens over every existing device screen, preserves its state, and
offers brightness/warmth +/-5, light toggle, night mode and Refresh screen. Date/time
appears here when RTC is valid; battery readout is explicitly unavailable. Orientation,
time-setting and fonts remain pending controls. Routine fast/partial refresh is
still pending: all current updates use the full-refresh driver. Manual Refresh
screen closes the menu and refreshes the underlying view without saving or navigating.

Included: filename browser/folders/paging, New file with arbitrary extension,
plain-text viewer, long-press Edit/Execute menu, disk-backed editor with keyboard,
tap cursor and Home Save/Discard/Cancel, long Home discard, light toggle and sleep.

Pending on device: Markdown/math reader, Python Console/Execute, StarDict, fonts,
orientation/time-setting/font controls, validated battery service, fast refresh and EPUB.
Console/Execute currently show a pending screen. All draws already use the
existing full-refresh panel driver; a manual refresh-menu control is now present.

## Source progress after this downloadable image

Stage 22 source/builds connect Python Console/Execute and queue touch/button
input independently during refresh. Stage 23 also connects Markdown/math and reader
page/chapter navigation. The download described here remains Stage 20;
it has not been replaced by the integration build. Rebuilding the latest source
therefore produces a different image from the hash below. See MICROPYTHON.md and
results/stage22 for current build evidence and physical-test limitations.

## Build identification

- SDK: ESP-IDF v5.5.5, `b774170ff46c393eeb5e495ea37936038d3f4f4f`, clean pinned submodules.
- Compiler: Xtensa GCC 14.2.0, esp-14.2.0_20260121.
- Build: browser ON, math diagnostic OFF; 20 KiB main-task stack.
- Image: 371504 bytes; configured app slot 8257536 bytes, 95% free.
- SHA-256: `cc65982b9f36a9c6c0a8b41fbc67a45af897f45d13e005c5c878d62ab071a932`.

With the pinned SDK installed and its `export.sh` sourced:
```
bash scripts/build-browser.sh
```
Output: `build-browser/firmware.bin` (identical to `inkpy.bin`), ELF/map and
bootloader/partition build outputs. The script builds only; it never flashes.

Stage 20 download limits: panel refresh blocks its input loop; SD latency, battery behavior,
sleep, touch, panel variants and editor stack margin need physical checking.
Device Save uses a backup/replacement sequence because FatFs does not overwrite
rename destinations. Power-loss recovery and stale backup cleanup are deferred.
