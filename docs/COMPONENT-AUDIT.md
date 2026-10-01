# Stage 1: component audit
2026-10-01. Source inspection only; no build, port, benchmark or device test has been performed.

## Decision
Proceed with a C-first ESP-IDF application. Reuse selected X4 Pro driver logic, not CrossPoint's application framework or complete Arduino dependency graph. Keep MicroTeX behind a narrow C interface if its prototype succeeds; translating its class hierarchy to C would not remove its layout complexity.
One board can still contain different panel controllers. Retain X4 Pro controller detection/support until the user's actual panel is identified; other devices are out of scope.

## Pinned references
These are audit pins, not claims of working integration. Future work should use these exact revisions rather than re-searching branch heads.
- [crosspoint-reader/crosspoint-reader](https://github.com/crosspoint-reader/crosspoint-reader/tree/93e98bb78702e29868a16a13b80c40e6b36ccdff): `93e98bb78702e29868a16a13b80c40e6b36ccdff` (master reference).
- [Free-Ink/freeink-sdk](https://github.com/Free-Ink/freeink-sdk/tree/111fdcc7f0176c3ee38391a160ee296bf492dbd8): `111fdcc7f0176c3ee38391a160ee296bf492dbd8` (exact CrossPoint submodule).
- [micropython/micropython](https://github.com/micropython/micropython/tree/19e685eca906a5a602135a485976253e705297d0): `19e685eca906a5a602135a485976253e705297d0` (audited candidate; not yet integrated).
- [espressif/esp-idf](https://github.com/espressif/esp-idf/tree/b774170ff46c393eeb5e495ea37936038d3f4f4f): `b774170ff46c393eeb5e495ea37936038d3f4f4f` (v5.5.5 selected build baseline).
- [mity/md4c](https://github.com/mity/md4c/tree/c7ba975c34d714966ea910c58a97524e5d674ff7): `c7ba975c34d714966ea910c58a97524e5d674ff7` (audited candidate; not yet integrated).
- [NanoMichael/MicroTeX](https://github.com/NanoMichael/MicroTeX/tree/0e3707f6dafebb121d98b53c64364d16fefe481d): `0e3707f6dafebb121d98b53c64364d16fefe481d` (audited candidate; not yet integrated).

CrossPoint's default branch was develop; this audit deliberately used master. FreeInk is the gitlink recorded by that master commit. MicroPython and MD4C pins are development snapshots, not asserted stable releases. ESP-IDF v5.5.5 was resolved through its annotated tag to the commit above.

## Hardware and ports
Primary inspected paths in FreeInk:
- docs/xteink-x4pro-support.md
- libs/hardware/BoardConfig/include/BoardConfig.h (XTEINK_X4_PRO initializer)
- libs/hardware/PowerManager/src/PowerManager.cpp
- libs/hardware/FrontlightManager/src/FrontlightManager.cpp
- libs/hardware/SDCardManager/src/SdmmcBlockDevice.cpp
- libs/display/FreeInkDisplay/src/bus/EpdBus.cpp
- libs/display/FreeInkDisplay/src/driver/Ssd1677Driver.cpp

| Area | Source finding | InkPy consequence |
|---|---|---|
| MCU | ESP32-S3, 16 MB flash, 8 MB octal PSRAM | Separate internal/DMA RAM from external memory budgets. |
| Panel | 800 × 480; SSD1677, UC8179 and UC8279 production variants | Preserve variant detection and relevant waveform differences. Do not assume one command stream. |
| Display SPI | CLK12, MOSI11, CS13, DC18, RST14, BUSY6 | Port transport to IDF; retain sequencing and timeouts. |
| Buttons | Previous GPIO0, next GPIO7, Power GPIO3; active low | Home is not another GPIO. GPIO0 is a boot strap. |
| Touch/Home | GT911; SDA39/SCL38, IRQ10, RST4, enable GPIO2 active low | Read touch coordinates and Home key status; omit swipe recognition. |
| Light | Cool GPIO8, warm GPIO9; active-high PWM, 25 kHz, 10-bit profile | Both requested light controls have a hardware foundation. |
| SD | Native 1-bit SDMMC: CLK41/CMD42/D0=40; GPIO5 active-low enable | Do not substitute an SPI SD driver. |
| Clock | BM8563/PCF8563-compatible RTC at 0x51 on shared I2C | Local time controls need no Wi-Fi. |
| Battery | CW2017 at 0x63; charge status GPIO21 | Keep essential battery/power handling even with minimal UI. |

Source discrepancies were found: the SDK hardware document has a stale paragraph reversing touch IRQ/reset and says touch mirroring is pending; its current initializer uses IRQ10/RST4, swapXY=true, flipY=true with a corner-test comment. Use executable profile values as the starting point and verify all four corners on hardware. The document's light channel numbers also differ from current implementation (which reserves channels 0/1); pins/polarity matter, do not copy stale channel numbering.

SD implementation power-cycles GPIO5 per mount attempt (80 ms off, 120 ms on), validates sector 0, retries the whole attempt, and uses DMA-capable bounce buffers. Port these behaviours; do not import SdFat simply because CrossPoint uses it. Prefer one IDF FatFs/VFS mount shared by native firmware and the Python adapter, with serialized access and explicit ownership. Never mount the same card independently through two filesystem stacks.

The driver source uses Arduino SPI, GPIO and timing calls. Native IDF porting is real work, not just changing compiler flags. Only the SSD1677 driver/transport was inspected in detail this stage; UC driver internals and GT911 implementation need targeted inspection during bring-up.

## MicroPython
Inspected ports/esp32/{README.md,CMakeLists.txt,esp32_common.cmake,main.c,mphalport.c,mpconfigport.h}, main/idf_component.yml and ESP32_GENERIC_S3's octal-SPIRAM variant.
The pinned README recommends IDF v5.5.5; select that baseline. The component manifest's >=5.3 constraint is not a blanket compatibility guarantee. Pin resolved component/submodule dependencies when the first build is created.

Reuse the ESP32 port and its networking support, not a bare VM with networking reconstructed from scratch. InkPy owns the application lifecycle; adapt the port's task/startup and I/O hooks:
- Route stdin/stdout through bounded queues and a console history store.
- Keep rendering/input in a native task so Python does not own UI responsiveness.
- Dispatch explicit Run and REPL commands instead of automatically executing SD boot.py/main.py.
- Reserve a bounded Python heap. The existing split-heap auto-growth configuration must not consume all memory needed by the UI.
- Disable unneeded Bluetooth, WebREPL, USB file-transfer support and optional device modules. Keep a development flashing/logging route.
- Do not allow Python peripheral initialization/deinitialization to reset display, light, SD or shared I2C resources owned by InkPy.
- Adapt Python filesystem access to the one shared storage service; this adapter is not already provided by the audited code.

Stop is a lifecycle problem, not simply deleting a FreeRTOS task. Use an interrupt request at VM safe points, finite waits in supported blocking operations and explicit cleanup. KeyboardInterrupt is catchable; native calls may block. A robust escalation strategy for a non-cooperative script remains to be prototyped. Do not promise instantaneous termination of every possible program or kill a task while it owns storage locks. Single interpreter/session is the initial scope; Python threads should stay disabled unless needed and evaluated.

## Markdown and math
MD4C (MIT) is the preferred parser candidate: C, callback API, table support and explicit inline/display LaTeX span callbacks via MD_FLAG_LATEXMATHSPANS. Use selected flags, not the entire evolving GitHub dialect. Disable HTML interpretation. Inspected src/md4c.h and LICENSE.md.

Important limitation: md_parse accepts one contiguous input string; callbacks do not make input streaming. It cannot simply receive unrelated page-sized chunks while preserving Markdown semantics. Before the full reader, prototype a stateful block scanner with an SD-backed block/reference index and bounded parsing units. Fences, nested lists, references defined later and oversized blocks must be handled explicitly. If MD4C cannot meet bounded-memory requirements without excessive adaptation, revisit the parser decision at that gate. No arbitrary whole-file RAM limit is approved.

MicroTeX (MIT core) is a conditional math candidate. Inspected CMakeLists.txt, src/{latex.cpp,config.h,graphic/graphic.h,core/parser.cpp,fonts/fonts.cpp}, LICENSE and res/RES_README.
- Upstream build selects desktop backends and links tinyxml2. Do not import GTK/Qt/Skia/Cairo.
- Implement its Font/TextLayout/Graphics2D interfaces over a small raster backend, including transforms and metrics needed by supported formulas.
- Source uses exceptions, STL structures, dynamic allocation and optional filesystem facilities. Its IDF component must enable required C++ support locally and catch exceptions at the C boundary. Do not globally disable exceptions without changing the parser.
- Evaluate a minimal FreeType configuration for both text and math glyph rasterization. FreeInk's vendored FreeType is an available pinned reference, but its adaptation is not yet selected or tested.
- Keep the required math font/metric resources, render one formula at a time and cache bounded bitmaps. Preserve baseline, ascent and descent for inline math.
- Compiled-in metrics do not prove XML paths can all be removed. Audit actual initialization reachability during the prototype before deciding whether tinyxml2 is removable.
- Bundled res/fonts TTF files total 458,568 bytes on disk (sum of tree metadata), excluding other resource directories. This is not their RAM footprint or the required minimal subset.

The new corpus in fixtures/markdown-math.md is a proposed compatibility target, not a claim of current support. Unsupported/malformed math should display its source with a compact error rather than crash or silently disappear. Wide equations need a deliberate no-swipe layout policy; proposed first choice is fit-to-width down to a readability floor, then a visible overflow fallback.

## Fonts and dictionary
CrossPoint's src/util/Dictionary.h and docs/dictionary.md describe an SD-sidecar indexed reader supporting uncompressed .idx, .dict or dictzip .dict.dz and optional .syn. Its documented 64-bit offset rejection and definition cap must not be mistaken for universal StarDict support. Reuse its indexing strategy and evaluate selected lookup code later; do not import its settings or EPUB definition UI. Begin with exact/synonym lookup and lightweight definition text; English stemming is optional, not a multilingual guarantee.
Font selection is now confirmed UI scope. Propose SD font discovery plus fixed fallback text/monospace/math fonts. Changing the text font must not remove math symbol fonts.

## Sleep and event semantics
Inspected IDF docs/en/api-reference/system/sleep_modes.rst at the pinned version: light sleep preserves execution state; deep sleep powers off most RAM and resumes through startup. Retaining the e-ink image alone does not preserve an editor or Python VM.
Initial proposal: state-preserving light sleep after inactivity, with touch/input disabled and retained display image. Reserve deep sleep for a later measured policy with explicit restoration; never silently lose unsaved text or console variables. Even idle firmware must wake to process input; the e-ink panel being static is not CPU sleep.
Power GPIO wake is level-triggered, not a hardware long-press detector. Wake internally on press, validate hold duration before enabling interaction, and return to sleep for short presses. Suppress the wake press's normal action and avoid immediate re-wake after entering sleep. Measure actual rail/PSRAM/light-sleep current; no battery-life estimate is justified yet.

## Memory and build gates
| Consumer | Known quantity or measurement needed |
|---|---|
| 1-bit framebuffer | 800 × 480 / 8 = 48,000 bytes; two buffers 96,000, excluding metadata |
| Full 8-bit coverage buffer | 384,000 bytes; prefer bounded tiles where possible |
| SD transfer | DMA-capable internal buffer; SDK example uses up to 8 × 512 = 4,096 bytes |
| Python | GC heap cap, task stacks, retained objects and network/TLS peaks must be measured |
| Math/fonts | Startup peak, per-formula peak, glyph caches and temporary allocations must be measured |
| Document/editor/console indexes | Bound RAM; spill large indexes/history to SD |
| Firmware | Linker map and partition fit required; 16 MB flash is not automatically one application slot |

## Licensing and exclusions
CrossPoint, FreeInk, MicroPython core, MD4C and MicroTeX core have MIT notices. Third-party components/fonts have separate terms. FreeInk's vendored FreeType LICENSE.TXT offers FTL or GPL; prefer FTL with required attribution if reused. MicroTeX lists OFL/Knuth/dsrom font notices and separate Greek/Cyrillic resource directories with other licences. Audit the exact selected font set, not only the root MIT file. No third-party code or fonts are copied in this stage; InkPy's own licence is still unselected.

## Next implementation boundary
Stage 2 is repeatable minimal board bring-up only: pinned IDF build, X4 Pro hardware adapter, diagnostic display/input/SD/light/RTC and sleep probes, plus a clear hardware checklist. No full browser/editor/reader/Python implementation. Preserve existing boot/recovery capability; establish actual partition/boot state before supplying flash instructions. Stop after a reproducible build and documented device-test handoff (or a reproducible blocker).
Subsequent separate gates: math/font prototype; bounded Markdown parsing; Python lifecycle/coexistence; then feature stages. Hardware evidence determines readiness, not this audit.
