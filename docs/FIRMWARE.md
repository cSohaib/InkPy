# Stage 19 firmware checkpoint

`firmware.bin` is an ESP32-S3 **application image** for the X4 Pro. It does not
contain a bootloader or partition table and is not a merged full-flash image.
It has compiled and passed image checksum/hash validation; it has not been flashed
or tested on physical hardware. The installation gate in BRINGUP.md still applies.

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
- Image: 344544 bytes; configured app slot 8257536 bytes, 96% free.
- SHA-256: `3d8b7eac5b1b69cf63d1c387ff735f399003677e2ae97022bc55e08ae5e14471`.

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
