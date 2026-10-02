# Stage 19.1 firmware checkpoint

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

Included: filename browser/folders/paging, New file with arbitrary extension,
plain-text viewer, long-press Edit/Execute menu, disk-backed editor with keyboard,
tap cursor and Home Save/Discard/Cancel, long Home discard, light toggle and sleep.

Pending on device: Markdown/math reader, Python Console/Execute, StarDict, fonts,
full power menu (including the newly requested Refresh screen) and EPUB.
Console/Execute currently show a pending screen. All draws already use the
existing full-refresh panel driver; no additional refresh-menu control yet.

## Build identification

- SDK: ESP-IDF v5.5.5, `b774170ff46c393eeb5e495ea37936038d3f4f4f`, clean pinned submodules.
- Compiler: Xtensa GCC 14.2.0, esp-14.2.0_20260121.
- Build: browser ON, math diagnostic OFF; 20 KiB main-task stack.
- Image: 356576 bytes; configured app slot 8257536 bytes, 96% free.
- SHA-256: `e123b05a90bb0b58ede9c4c46238f34bc373260183f81dccce5d126739e90fae`.

With the pinned SDK installed and its `export.sh` sourced:
```
bash scripts/build-browser.sh
```
Output: `build-browser/firmware.bin` (identical to `inkpy.bin`), ELF/map and
bootloader/partition build outputs. The script builds only; it never flashes.

Current limits: panel refresh blocks the input loop; SD latency, battery behavior,
sleep, touch, panel variants and editor stack margin need physical checking.
Device Save uses a backup/replacement sequence because FatFs does not overwrite
rename destinations. Power-loss recovery and stale backup cleanup are deferred.
