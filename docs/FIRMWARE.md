# Stage 42 UTF-8 SD filenames firmware

The feature set is integrated: browser/text/editor, Python console/scripts,
Markdown/math, StarDict, Unicode fonts/selection, Power controls and differential
refresh, EPUB chapters and raster images. The user tested an earlier image on an unlocked
X4 Pro; Stage 27 boots on that device. Stage 29 plain Markdown, TTF, Python file writes and Wi-Fi/HTTP have now
worked on-device. Stage 30 input(), JSON and sleep/wake are also confirmed on-device.
Stage 31 math and Stage 32 AI script are now confirmed working on-device.
Stage 41 Arabic, Stage 40 justification/math and earlier reader/UI changes are
build/host checked; device testing is pending. Stage 34 Power uses two light rows plus
font/refresh/contrast tiles; only Python inkpy.set_time changes the clock.

Stage 42 enables UTF-8 FatFs paths for Arabic/non-ASCII filenames. Filename
display remains unchanged. User reports Stage 41 Arabic still separated/LTR
on-device; this remains unresolved despite passing host reader tests. A failing
document is needed. This build contains reader-only Arabic shaping: each line uses its first
logical letter to choose RTL (Arabic) or LTR (Latin/no letters), while preserving
mixed Latin/numbers and original words for dictionary lookup. The bundled font
supports Arabic; no new SD files are needed. Fixed-cell typography remains.
It justifies wrapped EPUB/Markdown prose by default, keeping final lines
naturally aligned. The requested math commands and matrix/bmatrix/vmatrix work
through the existing engine. EPUB caches rebuild on first opening after upgrading,
then reuse normally; SD resources are unchanged. Loading still shows only an
hourglass, with no temporary EPUB debugging. Device testing of this stage is needed.
EPUB has no page-number jump; Markdown retains it.

Use the same X4 Pro application-image web installer that worked previously.
firmware.bin is an **application-only ESP32-S3 image**, without bootloader or
partition table. Keep the existing recovery route and partition map. No flashing
was performed here; other generated build outputs are not supplied for installation.

Extract inkpy-sd-resources.zip and copy its inkpy folder onto the microSD root.
Math fonts belong at inkpy/math/fonts/; missing assets preserve formula source.
The bundled text font needs no SD assets. Optional text fonts: SD-root fonts/ and family subfolders (TTF or
TrueType-outline OTF; cpfont and CFF OTF unsupported).
StarDict companions: dictionaries/, supplied by the user. Resources include font
licences/hash manifest. TrueType outlines work; CFF OTF faces are unsupported.
Python API: [PYTHON-DEVICE.md](PYTHON-DEVICE.md).

Stage 25 rolled back on-device due to excessive math-library constructor stack
usage before app_main. Stage 26 replaces four large table initializers; this
fix passed the original crash on-device, exposing main-task allocation failure.
Stage 27 defers heavy math-table allocation and now boots on-device. Stage 29
initializes math at the first formula, after reader cache preflight, and logs
cache filesystem errors. Plain Markdown now opens on-device; math separately crashes at preflight
initialization, which Stage 30 simplifies.

## Build identification

- ESP-IDF v5.5.5: b774170ff46c393eeb5e495ea37936038d3f4f4f, pinned submodules.
- Xtensa GCC 14.2.0, esp-14.2.0_20260121; ESP32-S3, 16 MiB DIO.
- Browser ON, diagnostics OFF; 32 KiB main stack.
- Application: 2,729,536 bytes; slot 8,257,536 bytes.
- SHA-256: 874c9054ec0837120d10571345f19ed278a9653d4ca98d5b16b800543fd8c379.
- Build: bash scripts/build-browser.sh; output build-browser/firmware.bin.
- Evidence: results/stage42/image.txt, valid esptool checksum and validation hash.

## Physical checks and limits

Check touch/alignment, typing, Power controls, reader/math/translation, Python
files/imports, Wi-Fi/TLS, Stop/Close and sleep/wake. Runtime heap/stack/current draw
and refresh quality remain unmeasured. Three panel paths are source/build checked,
not physically tested. Full refresh only at first paint/first redraw after controller
sleep, or manual Refresh screen; normal draws are differential. Some ghosting is
expected without periodic clears. Landscape exists only in Markdown book pages, toggled from the Home menu.
All other screens and menus stay portrait. Reflow retains the source anchor. Power settings are session-only; dictionary choice persists.

RTC writes update system time for TLS. Battery readout requires a running CW2017
and verified resident OEM profile; failures show --, with no profile writes.
Wi-Fi starts only from Python and stops on Close. Sleep pauses Python and stops/
restarts Wi-Fi; remote connections may expire. SDK DNS/handshake delays still need
measurement. No script runtime limit; only a worker that has acknowledged VM/I/O
cleanup is deleted on Close. Editor Save still uses
backup/replacement; power-loss/stale-backup cleanup and cache optimizations are polish.

Stage 33 releases the Python worker heap/task/mutex on acknowledged Close;
repeated reopen and internal-heap recovery must be retested on-device. UI error detail is logged to serial
while the screen displays a circled cross. SD resources are unchanged.

Stage 30 removes the first-formula guarded whitelist initializer, adds whole-line
input(), reserves the standard JSON module against SD overrides, and holds battery
and SD rail outputs through light sleep. All require device retesting. Existing SD
math assets remain unchanged. Test script: fixtures/python-device-test.py.

Stage 31 ordinary malloc/new/realloc allocations prefer PSRAM at every size,
including small math-library objects. Explicit RTOS/DMA/internal allocations and
the 64 KiB internal reserve remain unchanged. Heap metrics are logged around math
initialization/font open; critically low font-open headroom uses source fallback.
Device math acceptance and actual heap savings remain unmeasured. Test math and
then Python; the broader allocation policy also merits a Wi-Fi/HTTP regression.

Stage 36 supports the requested math commands and bordered/aligned/wrapped pipe
tables, including inline formulas. Test fixtures/markdown-tables-math.md. Existing
SD math assets are unchanged; see READER-MATH.md for table/parser limits.
