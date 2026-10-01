# Stage 4 — isolated ESP32-S3 math diagnostic

The shared renderer now lives in `components/ink_math`. The host probe remains
in `prototypes/math`; both compile the same backend and vendor source list.
This is a diagnostic, not the product reader. Hardware acceptance is pending.

## Build and resources

Use the pinned ESP-IDF environment from BRINGUP.md. Nothing here flashes a device.

```sh
bash scripts/build-math.sh
# Prepare a NEW local staging directory, not an inserted card's mountpoint:
python3 scripts/prepare-math-sd.py prototypes/math/.deps/MicroTeX build-math/sd
```

Copy the generated `inkpy` directory to the microSD root manually. The renderer
uses `/sd/inkpy/math/fonts/...`. The manifest records sizes and SHA-256 values.
The staging script permits repeats only in a directory carrying its own marker;
use a new directory when changing the selected resource set to avoid stale files.
No Wi-Fi, USB transfer protocol, cloud or firmware download service is involved.

The normal hardware build still uses `bash scripts/build.sh`. The math build has
separate `build-math/` outputs and sdkconfig: C++ exceptions/RTTI enabled, 32 KiB
main-task stack and generic allocations preferring PSRAM. Internal allocations
remain available for stacks, DMA and peripherals. These are diagnostic defaults,
not a demonstrated safe final memory configuration. Never flash the generated
bootloader or partition table based only on their existence; BRINGUP.md's device
installation/recovery checks remain mandatory.

## What changed

- One shared C-facing renderer, retaining the small C++ adapter MicroTeX needs.
- Shared build source list compiles 13 FreeType C translation units rather than
  its full desktop archive. Four modules: TrueType, SFNT, PostScript glyph names,
  monochrome raster. No GUI, color font backend, auto-hinter or shaping library.
  Incremental glyph callbacks are disabled: InkPy loads complete TTF files.
  Bundled gzip remains an SFNT link dependency; no external zlib is linked.
- Math assets: 27 TTFs / 401,840 bytes, down from 30 / 458,568. Euler and dsrom fonts
  are omitted because their commands are outside the current allowlist. Latin
  files labelled `optional` upstream remain: bold math actually needs them.
  Greek/Cyrillic language packs remain excluded; ordinary TeX Greek symbols work.
- A context-checked CMake overlay changes MicroTeX `mathop` from `noLimits` to
  `normal`. Display `lim`/`min` place limits below, inline keeps them beside.
  The overlay copies macro_def.cpp beside the patched header because its include
  is relative. Cached upstream sources stay unmodified; MIT notice is retained.
- A second small overlay initializes an upstream UTF conversion accumulator
  flagged by the Xtensa compiler. This is not a general UTF-8 validation fix;
  the future text reader must use a validating decoder.
- Generated `main/math_cases.h` snapshots the 29 corpus formulas. Regenerate with
  `scripts/generate-math-cases.py` only after a passing host probe. The header
  records the original Markdown fixture hash. This build does not parse whole
  Markdown documents or include MD4C in the firmware image.

## Device run contract

Same board initialization and controls as BRINGUP.md, except Prev/Next cycle 29
formulas instead of five patterns. At boot, SD and PSRAM are required. Math fonts
are loaded on demand; missing files produce an explicit logged fallback.
The 480x800 portrait bitmap is rotated into the existing native landscape panel
buffer and its bit polarity is inverted. Physical orientation needs checking.
Touch redraws the current sample; this is not word selection or reader UI.

Each render reports case ID, expected/actual result, dimensions and baseline.
Cases 27–29 intentionally fall back; the panel shows an X and serial output gives
the error/source. This is not the eventual literal-source reader fallback UI.
An unexpected result is logged as `UNEXPECTED RESULT`.

Initialization and each render record:

- elapsed microseconds (render timing excludes panel refresh);
- current free bytes, local minimum free bytes, and largest free block, separately
  for internal 8-bit RAM and PSRAM;
- minimum remaining task stack in bytes;
- global allocation-failure counter and last failed allocation size.

IDF's local heap monitoring observes allocations between checkpoints, unlike the
Stage 3 host's post-formula samples. Aggregate per-heap minima need not occur at
the same instant; treat them as conservative resource indicators, not an exact
simultaneous peak for one formula. Other IDF activity may contribute. Allocation
failure tracking starts in app_main before renderer initialization; it cannot
capture startup/static-constructor failures. The 32 KiB stack is provisional.

No hard heap budget or exhaustive allocation-failure recovery is claimed. The
parser uses dynamic STL allocations; an exception caught at the C boundary does
not prove all interrupted upstream initialization paths can be reused safely.
Only one initialization per boot/session is permitted.

## UI changes confirmed in Stage 4

No sliders: brightness/warmth will use + and - only. No power-menu UI existed
before this change. Reader Home will show Go to chapter / Go to page / Close,
plus current page and chapter; Markdown chapters are H2 headings only. Time,
date and battery level appear only in the power-menu header, never in a persistent
status bar. These are recorded in UI.md; the product menus remain unimplemented.

Sleep code now accepts one Power press to wake. The previous GPIO wake already
resumed the CPU; the old additional 800 ms qualification has been removed.
Entering and waking presses are consumed through a stable release, so they do
not trigger an awake action. Awake long-press still sleeps. Wake still restores
touch/light without changing pixels. Real switch/wake behavior is not yet tested.

## Runtime acceptance remains pending

After establishing the device's installation/recovery route:

1. Verify all 29 cases and inspect glyphs, bounds and physical orientation.
2. Retain initialization/per-formula heap, stack, failure and timing logs.
3. Cycle repeatedly; examine fragmentation, failure counters and baseline memory.
4. Exercise missing fonts/card and SD read errors without claiming recovery from
   arbitrary allocator failures.
5. Verify short-click wake, bounce/held presses, unchanged pixels and no unintended
   menu/light/sleep action. Measure actual sleep current.

Do not start the full reader on an assumption that host success proves device fit.

## Recorded verification — 2026-10-01

- Host corpus: 26 renders + 3 expected fallbacks; all 18 extra checks passed.
  Updated preview inspected: display lim/min limits now sit below the operators.
  See [preview](results/stage4/preview.png), [case results](results/stage4/formulas.tsv)
  and [host metrics](results/stage4/host-metrics.txt).
- Both ESP32-S3 configurations built/linked with pinned IDF v5.5.5, Xtensa GCC
  14.2.0 (esp-14.2.0_20260121), CMake 3.30.9 and Ninja 1.13.2. Vendor C++ is
  explicitly compiled as C++17 despite IDF's newer default. Existing upstream
  reorder/switch warnings remain visible with narrowly scoped nonfatal treatment;
  InkPy C/C++ sources retain warnings-as-errors. No other warning category was
  waived to obtain the build; actual uninitialized-value/path issues were handled
  as described above.
- Math app binary: **1,503,584 bytes**, leaving **82%** of the configured 0x7e0000
  app slot free. Static D/IRAM: **81,059 bytes**. This excludes runtime allocations
  and is not a measurement of total live RAM use.
- Updated hardware-only app: **332,048 bytes**, static D/IRAM **73,735 bytes**.
  Input timing tests passed. It does not link the math renderer.
- Full [math size report](results/stage4/math-size.txt),
  [hardware size report](results/stage4/hardware-size.txt),
  [binary sizes/hashes](results/stage4/binaries.json) and
  [font manifest](results/stage4/font-manifest.json) are retained. Binary hashes
  identify these builds; local Git/build metadata may affect reproducibility.
- No device attached or flashed. Runtime heap/stack, panel orientation, SD font
  reads, timing, single-click wake and sleep current remain unmeasured.
