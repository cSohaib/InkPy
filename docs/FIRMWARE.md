# Stage 27 deferred-math exploratory firmware

The non-EPUB feature set is integrated: browser/text/editor, Python console/scripts,
Markdown/math, StarDict, Unicode fonts/selection, Power controls and differential
refresh. EPUB remains deferred. The user tested an earlier image on an unlocked
X4 Pro; this combined build has passed compilation/host checks, not device tests.

Use the same X4 Pro application-image web installer that worked previously.
firmware.bin is an **application-only ESP32-S3 image**, without bootloader or
partition table. Keep the existing recovery route and partition map. No flashing
was performed here; other generated build outputs are not supplied for installation.

Extract inkpy-sd-resources.zip and copy its inkpy folder onto the microSD root.
Math fonts belong at inkpy/math/fonts/; missing assets preserve formula source.
The bundled text font needs no SD assets. Optional text fonts: SD-root fonts/.
StarDict companions: dictionaries/, supplied by the user. Resources include font
licences/hash manifest. TrueType outlines work; CFF OTF faces are unsupported.
Python API: [PYTHON-DEVICE.md](PYTHON-DEVICE.md).

Stage 25 rolled back on-device due to excessive math-library constructor stack
usage before app_main. Stage 26 replaces four large table initializers; this
fix passed the original crash on-device, exposing main-task allocation failure.
Stage 27 defers heavy math-table allocation until reader initialization; build
and host checks pass, with device verification pending.

## Build identification

- ESP-IDF v5.5.5: b774170ff46c393eeb5e495ea37936038d3f4f4f, pinned submodules.
- Xtensa GCC 14.2.0, esp-14.2.0_20260121; ESP32-S3, 16 MiB DIO.
- Browser ON, diagnostics OFF; 32 KiB main stack.
- Application: 2,583,904 bytes; slot 8,257,536 bytes, 66% free.
- SHA-256: a36fdfeeae8e9c1968b97889b828f96b9d679e8008a24fa2cab073c166ce1b9c.
- Build: bash scripts/build-browser.sh; output build-browser/firmware.bin.
- Evidence: results/stage25/final-check.txt, valid esptool checksum and validation hash.

## Physical checks and limits

Check touch/alignment, typing, Power controls, reader/math/translation, Python
files/imports, Wi-Fi/TLS, Stop/Close and sleep/wake. Runtime heap/stack/current draw
and refresh quality remain unmeasured. Three panel paths are source/build checked,
not physically tested. Full refresh only at first paint/first redraw after controller
sleep, or manual Refresh screen; normal draws are differential. Some ghosting is
expected without periodic clears. Landscape Markdown reflows at 800x480; other
screens retain compact fixed hit grids, with unstretched glyphs. Reflow retains
nearest numeric page. Power settings are session-only; dictionary choice persists.

RTC writes update system time for TLS. Battery readout requires a running CW2017
and verified resident OEM profile; failures show unavailable, with no profile writes.
Wi-Fi starts only from Python and stops on Close. Sleep pauses Python and stops/
restarts Wi-Fi; remote connections may expire. SDK DNS/handshake delays still need
measurement. No task deletion or script runtime limit. Editor Save still uses
backup/replacement; power-loss/stale-backup cleanup and cache optimizations are polish.
