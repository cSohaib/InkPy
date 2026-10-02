# InkPy checkpoint

Updated: 2026-10-02. Stage 5 host pagination complete; hardware validation pending.
Committed stage boundary: pause before starting further implementation.

## Read first
AGENTS.md and this checkpoint; docs/UI.md owns the current interaction contract.
For math, read docs/MATH-DEVICE.md and components/ink_math/; for board work, read
BRINGUP.md and main/. Source pins are already recorded; do not rediscover them.
Stage 3 measurements in MATH-PROTOTYPE.md are historical host results.

## Latest confirmed UI changes
- Brightness and warmth: + and - only, no sliders. No sliders had been implemented.
- One short Power press wakes; awake long Power still sleeps. Wake press consumed.
  Implemented in main/sleep.c, not yet verified on physical switches/hardware.
- Reader Home: Go to chapter / Go to page / Close, with current page and chapter.
  Markdown chapters are parsed H2 headings only; EPUB chapter handling comes later.
- Time/date/battery visible only in power-menu header; no status bar elsewhere.
- Product menus are still unimplemented. The above is the updated UI contract,
  not a claim that the firmware already displays these menus.

## Stage 4 delivered
- Shared C API and small C++ math backend moved from prototypes/math into
  components/ink_math. Host and ESP32 builds use cmake/math-sources.cmake.
- Optional scripts/build-math.sh produces a separate ESP32-S3 math diagnostic;
  default scripts/build.sh still builds only the hardware diagnostic.
- Separate math sdkconfig: exceptions/RTTI, 32 KiB task stack, generic allocations
  prefer PSRAM. These are provisional diagnostic defaults, not measured budgets.
- 29 generated corpus cases, manual Prev/Next, portrait-to-panel bitmap conversion.
  Errors show a diagnostic X with source/error on serial, not final reader UI.
- Per-phase internal/PSRAM local heap minima/current free/largest block, stack
  high-water mark, allocation failure counters and timing. No measurements yet.
- 13 FreeType source units, four modules, no incremental glyph callbacks.
  Prepared font package: 27 TTFs / 401,840 bytes with hashes and licence notices;
  Euler/dsrom and Greek/Cyrillic language packs excluded. Optional Latin fonts stay
  because bold math needs them. See scripts/prepare-math-sd.py.
- Context-checked generated MicroTeX overlays fix mathop's limit placement and
  initialize its UTF conversion accumulator. Cached upstream remains unmodified.
  Its legacy UTF helper is not suitable as the future validating reader decoder.
- SD transport remains manual. No network/USB transfer feature added.

## Stage 4 verification
- Host: 26 rendered + 3 expected fallback formulas, 18 additional checks passed.
  Preview inspected, display lim/min now place limits below. Raw results retained
  in docs/results/stage4/; no need to rerun without a relevant source change.
- Both ESP32-S3 builds passed on pinned clean IDF v5.5.5
  b774170ff46c393eeb5e495ea37936038d3f4f4f and its submodule pins.
  Xtensa GCC 14.2.0 (esp-14.2.0_20260121), CMake 3.30.9, Ninja 1.13.2.
- Math binary: 1,503,584 bytes, 82% of configured app slot free; static D/IRAM
  81,059 bytes. Hardware-only: 332,048 bytes, static D/IRAM 73,735 bytes.
  Reports and binary hashes are committed; runtime RAM is not established.
- Input timing tests passed. Third-party reorder/switch warnings are nonfatal;
  InkPy's own sources compile with warnings-as-errors.
- No hardware attached/flashed. No panel/touch/SD/font/sleep/current/runtime
  heap/stack result may be inferred from these builds or host images.

## Existing foundation and fixed requirements
Stage 2 supplies the native C board diagnostic: X4 Pro pin map, three production
panel variants, monochrome full refresh, GT911/Home, SDMMC/FatFs, dual light,
read-only RTC validity/raw gauge, debounced input and retained-state light sleep.
FreeInk MIT provenance stays in PORTING.md. No Arduino/UI framework imported.
Stage 3 established a fixed 48 KB math bitmap, four cached font faces, bounded
math-source preflight and a narrow C boundary; this is not a hard engine heap cap.

Close console must stop its session; no background execution. Manual sleep must
suspend Python and resume it on wake. No script runtime limit; under a minute is
workload guidance only. Auto-sleep after five minutes except while script runs.
Python, editor, file browser, dictionary and complete Markdown reader do not yet
exist. EPUB remains deferred until Markdown is accepted. No slide gestures.

## Outstanding hardware gate
Before any flash command: establish this device's installed firmware/version,
actual partition layout/active slot, and a working recovery route. Committed
partitions mirror a source reference for linking only, not installation evidence.
Then follow BRINGUP.md and MATH-DEVICE.md and retain logs. No device session means
runtime acceptance stays pending; never call these ports device-validated.

## Stage 5 delivered
- components/ink_layout: C UTF-8 validation, bounded MD4C parse units, literal
  oversized-unit fallback, provisional text layout, disk page/H2 chapter indexes.
- prototypes/reader: reproducible pinned build, cache lookup/reflow source search,
  host preview and 229 acceptance checks. docs/READER-PROTOTYPE.md owns details.
- 8/32 MiB samples: context 19,976 bytes and parser peak 4,032 bytes unchanged;
  parser requested allocation cap 131,072 bytes. Not total RAM or device evidence.
- Two page previews inspected; results saved under docs/results/stage5/.
- Math is tagged source in this prototype; shared math backend unchanged. No
  EPUB-first pivot, firmware UI, SDK rebuild or flash performed this stage.

## Exact next bounded stage
Stage 6 on continuation: replace provisional codepoint metrics with actual font
metrics and connect bounded inline/display math composition in the host reader.
Define and test supported Markdown boundaries before calling the reader complete.
Keep cache invalidation/source anchors in view; do not start EPUB or whole UI yet.
Hardware/math runtime validation remains independently gated as above.

## Risks to preserve
- Full panel refresh blocks diagnostic input today; split scheduling before editor
  and console usability. Do not treat missed taps during refresh as resolved.
- One owner for hardware/filesystem today. Python needs arbitration and an
  acknowledged safe-point pause/stop protocol, never force-delete/suspend while
  holding locks. Catchable KeyboardInterrupt alone cannot guarantee termination.
- SD remains powered/mounted in light sleep. Measure current; not equivalent to
  deep sleep. External clocks/network timeouts continue while Python is paused.
- Battery percentage/profile, low-battery behavior and RTC setting are incomplete.
- MicroTeX/STL/FreeType/tinyxml2 allocate dynamically; no strict heap/stack cap or
  exhaustive OOM recovery. One initialization per boot/session. Current preflight
  accepts an intentionally limited ASCII math subset, not all ChatGPT notation.
- The old math harness reads a <=64 KiB fixture whole. Stage 5 adds a separate
  bounded reader prototype, with incomplete cross-block Markdown semantics and
  provisional font metrics. Its host results do not establish device viability.
- Open: extensionless Python execution, exact keyboard layout, project licence.

## Resume efficiently
Firmware pins and math fetching scripts are durable; scratch SDK/build caches
are disposable. Math dependencies remain under ignored prototypes/math/.deps;
scripts/fetch-math.sh enforces revisions and tracked-file cleanliness. Component
Manager is disabled; there are no managed components or runtime network downloads.
Do not rebuild or reread Stage 2/3 sources just to reconstruct this checkpoint.
