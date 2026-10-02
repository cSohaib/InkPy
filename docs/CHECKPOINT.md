# InkPy checkpoint

Updated: 2026-10-02. Stages 17–18 creation flow and device editor source wired; ESP32 build and hardware validation pending.
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
A host Python embedding now exists; device Python, editor, dictionary and complete Markdown reader do not yet exist.
An optional file browser and paginated plain text viewer now exist. Markdown
selection remains a placeholder until its richer reader is connected. EPUB remains deferred until Markdown is accepted. No slide gestures.

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

## Stage 6 delivered
- C layout accepts the existing math C API through an optional callback; inline
  formulas reserve width/baseline, display formulas occupy centered lines.
- Version 2 cache stores bitmap runs; host cache reader and preview compose them.
  Unsupported/too-large math falls back to source. Text metrics remain provisional.
- Small fixture: six rendered formulas, one fallback, three H2 chapters, two pages.
  Portrait/landscape generated and read; portrait previews visually inspected.
  Existing larger corpus: 26 rendered formulas and three expected fallbacks.
- docs/READER-MATH.md owns run instructions and limits; results/stage6 holds previews.
  Host build passed with own warnings-as-errors; no SDK build or device validation.
- User changed pace: prioritize a feature-complete exploratory version, small
  stages, basic compile/run/visual checks; defer extensive testing and polish.
  AGENTS.md records this. Real text font metrics and typography are deferred.

## Stage 7 delivered
- components/ink_browser: bounded directory page, folder/Home navigation,
  side-button paging, Markdown/text selection and sampled binary notice.
- main/browser_app.c and INKPY_BROWSER optional build connect the board input,
  native rotated framebuffer, SD mount, light and retained-state sleep.
- scripts/build-browser.sh builds separately; default diagnostics remain available.
  New-file/Console/power controls are notices; no reader contents or file-action menu.
- Basic same-core host compile/run passed; browser preview inspected and saved
  in results/stage7/. docs/FILE-BROWSER.md owns commands and current limitations.
- No SDK in workspace: device integration was not ESP32-compiled or flashed.
  Rendering issue noted by user stays deferred; Stage 6 rendering unchanged.

## Stage 8 delivered
- ink_text.c/.h adds a fixed current-page text grid, forward offsets and backward
  rescanning. Browser non-Markdown selection now opens it; side buttons page,
  Home restores the browser, tapping text does nothing.
- Read-only, no open file retained across sleep. CR/LF/CRLF, tabs and UTF-8/BOM
  supported; non-ASCII uses the interim '?'. Invalid text errors when reached.
- Browser and viewer host checks passed with warnings-as-errors; real drawing
  preview inspected under results/stage8/. docs/TEXT-VIEWER.md owns details.
- Word wrapping/font polish and backward seek optimization deferred. Stage 6
  math rendering unchanged. SDK absent; no ESP32 build or hardware claim.

## Stage 9 delivered
- ink_keyboard.c/.h: fixed English keys with ASCII characters and action events;
  reusable by later editor/console. Browser New file now accepts the filename and
  extension as typed, creates exclusively, and never overwrites existing files.
- Cancel/Home exits naming; success refreshes the listing. Dot-prefixed filenames
  are now shown. SD-invalid names and excessive path lengths produce inline errors.
- Basic creation/keyboard/no-overwrite checks and existing browser/viewer checks
  passed; real keyboard preview inspected under results/stage9/.
  docs/NEW-FILE.md owns commands/limits. No SDK/device build; math unchanged.
- User is eager for MicroPython: prioritize its first working prototype next,
  before expanding the text editor. This is the current sequencing decision.

## Stage 10 delivered
- Reused the already-audited MicroPython pin 19e685eca906a5a602135a485976253e705297d0
  via ports/embed. components/ink_python is a small C execution/REPL boundary.
- Runnable prototypes/python: .py script execution and multiline terminal REPL,
  expression display and persistent globals; no runtime timeout. Fixed 256 KiB
  host VM heap, streaming source lexer, 4,095-byte console block limit.
- Floats, bigints, math/gc/sys. Python file I/O/external imports, network and machine
  bindings deferred. No onscreen console/worker lifecycle integration yet.
- Python-first main keyboard now has quotes/brackets/backslash and Tab action;
  digits/operators on symbol layer. Future Tab default: four spaces, not literal
  tabs. New file ignores indentation action. Keep one fixed English keyboard.
- Basic script/REPL and keyboard/name checks passed; session and inspected keyboard
  preview under results/stage10/. docs/MICROPYTHON.md owns commands and limits.
- Fresh generated-header directories avoid upstream stale module registrations
  after config changes. GNU C11 required by GC register collector. Dependencies
  stay unchanged and ignored. No ESP32/SDK build or physical validation.

## Stage 11 outcome
- Python core now exposes worker-owned control/output callbacks. VM branch hooks
  acknowledge pause through the host adapter; stop uses upstream nlr_jump_abort,
  bypassing Python exception handlers. Return 2 means stopped; caller must close
  and reset the VM session. Python finally blocks are not guaranteed on stop.
- prototypes/python/worker.c proves a separate UI/control thread, pause/resume,
  stop while paused, and acknowledgment only after VM cleanup. Output retains at
  most 8192 newest bytes. This is a host adapter, not device task integration.
- Build and existing smoke check pass. Worker checks pass with an endless loop
  catching BaseException. Five-second test watchdog is NOT a runtime timeout.
- Control is cooperative at bytecode branch points, not arbitrary native calls.
  Future blocking storage/network bindings need their own safe cancellation.
  No forced task suspension/deletion and no close/sleep UI enabled prematurely.
- Preserved pre-existing run.sh permission-only change, excluded from commit.

## Stage 12 outcome
- components/ink_python/ink_console.{c,h}: bounded 4096-byte multiline input,
  four-space Tab, delete, Enter submission, busy input gate. History retains the
  newest 128 wrapped ASCII lines (24 columns); previous/next pages, seven rows.
  Older history is deliberately discarded for now; no unbounded RAM growth.
- Shared raster keyboard now draws filename and Python console screens. Console
  shows history, current input tail and running/continuation status; no swipes.
- prototypes/python/console.c: pthread VM owner, single command slot and locked
  output callbacks. Keyboard taps submit assignments, expressions and multiline
  blocks to the worker, preserving globals. Rendering happens outside the VM.
- Host build and checks pass: tap submission, result 5, multiline output 0/1/2,
  bounded history wrap, both page directions, normal close. Existing New file
  probe passes after sharing keyboard drawing. Preview in results/stage12.
- This is a scripted host preview, NOT an interactive desktop application or
  device console. Home menu, cursor movement and long-running stop/sleep controls
  are not wired to this screen yet. Stage 11 remains the stop/pause proof.
- SDK/device validation still pending. Kept pre-existing run.sh mode change out.

## Stage 13 outcome
- prototypes/python/session.{c,h} is the reusable pthread session adapter:
  one VM owner, one command slot, guarded console model, cooperative pause,
  Stop request and Close acknowledgment after cleanup. Caller supplies VM heap.
  Console probe uses this adapter; Stage 11 worker remains a historical proof.
- Short Home opens Stop process / Close console / Cancel. Second Home cancels;
  long Home requests Close. Menu taps cannot type through to keyboard. No swipes.
- Stop keeps console/history open, resets the VM and clears input; globals are
  lost deliberately because an aborted VM must not be reused partially unwound.
  Close works during pause and waits for VM cleanup. No task deletion or timeout.
- Build/probe pass: running endless command pauses at bytecode branch, Stop while
  paused, reset followed by 6*7 -> 42, Close during another paused command, reopen,
  idle long Home and second Home cancel. Shared New file probe still passes.
- Rendering uses a console snapshot outside the session lock. Menu/after-stop
  previews saved under docs/results/stage13. Host only, no ESP32/device result.
- Native blocking calls still need future cancellation support; abort skips
  Python finally blocks. Adapter is POSIX-specific, to be translated to FreeRTOS
  later without changing the console model. Pre-existing run.sh mode preserved.

## Stage 14 outcome
- Session adapter accepts .py file jobs with a fixed 512-byte path; rejects busy,
  closing, non-.py and oversized paths. Script source still streams through the
  existing FILE reader; parsing/bytecode use the supplied VM heap.
- Script output shares console history and globals remain available afterward.
  File errors print exceptions and return to the REPL. Stop still resets Python.
- Host checks pass: demo output, answer -> 42, non-executable rejection, missing
  file recovery, busy rejection, pause/Stop of a file catching BaseException.
- User authorized 2–3 stages this run; continue directly to Stage 15 rather than
  pausing at this intermediate local commit. Device integration remains pending.

## Stage 15 outcome
- Portable browser has long-press Edit/Execute menu. Execute requires .py,
  otherwise exact "not executable" notice. Edit validates its initial text page
  and issues an editor request; editor itself is still pending. No directory menu.
- Top Console button now issues console request. Optional device browser source
  handles long touch events; device runtime/editor requests remain honest pending
  screens because no FreeRTOS Python adapter exists yet. No firmware build claim.
- prototypes/python/app.{c,h} composes browser, console and session for the host:
  Execute launches a file job, Console opens an empty session, keyboard/side/Home
  events route to console, Close is polled then returns to original folder/page.
  Rendering copies worker state under lock, then draws outside it.
- Browser-console probe passes actual tap path: Console button, non-Python error,
  file Execute, stdout, tapped 6*7 -> 42, Close menu, long Home on running file.
  Portable browser checks pass paging/folders/text routing, Edit/binary gate,
  Execute gate; New file probe still passes. Previews under results/stage15.
- Two stages completed this batch as authorized; pause after pushing. No SDK,
  flash or physical validation. Keep pre-existing run.sh permission-only change.

## Stage 16 outcome
- components/ink_browser/ink_editor.{c,h}: disk-backed gap working copy, fixed
  4 KiB transfer buffers, 9x24 visible cells with byte-offset cursor mapping.
  No document-sized RAM buffer/index. Open streams/copies and validates full UTF-8
  text; ASCII font displays non-ASCII codepoints as '?' while preserving bytes.
- Same Python keyboard, four-space Tab, newline and backspace. Taps move cursor;
  side buttons page. Home Save/Discard/Cancel; second Home cancels; long Home
  discards. No swipes or separate Save button. Host app now opens Edit requests.
- Gap moves only bytes between old/new cursor. Growth shifts suffix in chunks
  every gap capacity expansion. Save streams to a sibling temp, flushes/fsyncs
  and renames on host; source is unchanged until successful Save. Discard removes
  working temp. Disk cost proportional to file size; opening/saving O(file size).
- Checks pass: tapped print(1)->print(2), Save, Cancel, Discard, long Home, temp
  cleanup; 8 MiB edit preserves length/tail, gap growth, Previous pages, Tab.
  Browser/Python route and New file checks still pass. Previews in results/stage16.
- No ESP32 build/device validation. SD durability, disk-full recovery and stale
  temp cleanup after power failure are not validated. Previous page rescans and
  ASCII fixed-grid layout remain provisional. Original file stays separate while
  editing. Preserve pre-existing run.sh permission-only change outside commit.

## Stage 17 outcome
- Successful New file now requests the editor directly, keeping exact filename
  and extension; failed/existing names remain in filename entry. Empty source is
  created immediately; discarding new edits leaves that empty file in place.
- Host tap test creates new.py, types print(6*7), Home Save, long-press Execute,
  and sees 42. Extensionless create/discard and previous filename checks pass.
- User authorized two stages this run; continue to Stage 18 source integration.

## Stage 18 outcome
- Editor tap/Home controller lives in ink_editor.c and is shared by host app and
  main/browser_app.c. Optional device browser now opens New/Edit requests, draws
  editor, routes side buttons and long Home, and includes editor in main CMake.
- Device sleep path flushes working FILE before sleep and leaves editor/gap/cursor
  state alive; a flush error keeps device awake. Sleep remains light sleep with
  SD mounted, per existing implementation. Not physically tested.
- Restored current remote sdkconfig.defaults locally, changing only main stack
  8192 -> 20480 bytes. Device editor asserts >=16384 to avoid stale configurations
  using too little stack for nested fixed-size I/O buffers. No measured margin.
- Shared controller checks pass editor Save/Cancel/Discard/long Home and 8 MiB
  edit/gap growth. Filename checks pass. Full host create .py -> type print(6*7)
  -> Save -> Execute produces 42; extensionless discard keeps an empty source.
- Corrected host tap helper to include Space/control row and editor probe linkage
  to shared keyboard. No SDK build or device runtime validation. Python remains
  host-only; device Console/Execute pending screens are unchanged.
- Two stages completed as authorized. Push batch and pause; preserve unrelated
  run.sh mode-only change. Preview/results in docs/results/stage18.

## Exact next bounded stage
Stage 19: restore the pinned ESP-IDF/toolchain environment and missing board files
from current GitHub state, compile INKPY_BROWSER with editor, and resolve concrete
port/build failures only. Keep Python task port, new features and flashing out of
this stage. Record any environment blocker concretely; commit and pause.

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
