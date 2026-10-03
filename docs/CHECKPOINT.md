# InkPy checkpoint

## Stage 30: math guard, whole-line input, JSON and sleep rails (2026-10-03)

Device feedback: plain Markdown, TTF, Wi-Fi/HTTP 200 and SD writes work. Math
restarts; json.load/loads unavailable; battery sleep cold-boots while USB sleep
resumes Python but breaks browser SD access. Wi-Fi prompt is explicitly dropped.

serial-log3 matches Stage 29 ELF 393626bab. Backtrace reaches __cxa_guard_acquire
from preflight's function-local unordered_set before formula layout. Replaced
it with a fixed constexpr command table; final ELF has no guard for that table.
Underlying abort condition/memory pressure is not proven; device retest required.

Built-in input() now accepts one complete keyboard line on Enter, retaining Stop,
Close and cooperative manual sleep. Worker waits without exposing keystrokes to
Python; empty strings work. Console input is bounded by the existing 4096-byte
buffer. JSON is registered as a non-extensible built-in: SD json.py/json/ cannot
hide its standard API. Host tests demonstrate this override protection; actual
cause of the user's missing attributes is not confirmed without SD inspection.

Sleep holds GPIO1 keep-alive HIGH and GPIO5 SD-enable at its current LOW level;
SD bus and power wake pins opt out of GPIO sleep selection. Holds are released
on every resume/error path. Existing open files and VM remain in place; no remount
that could invalidate streams. Hardware clue/source: CrossPoint PR3215,
https://github.com/crosspoint-reader/crosspoint-reader/pull/3215 (deep-sleep case;
InkPy applies its retention principle to light sleep). No hardware result claimed.

Validation: regenerated Python headers; native-profile host files/JSON/load/loads,
input values/empty/EOF/Stop/reopen pass; console input-key test passes; mixed
Markdown/math reader host check passes. Native ESP32-S3 build and image validation
pass. A damaged restored WPA archive was regenerated from existing objects.
Application 2,586,944 bytes; SHA256 ac4d52a990ee16dd39723e1a12e839e1066c1f72175b88e6b5a2c72d66823a70.
Evidence: results/stage30; test script fixtures/python-device-test.py.

Next bounded task: device math ($x+1$ first), input(), JSON and repeated sleep/wake
with USB and battery, including SD/browser/editor access. If failing, collect serial
sleep/wake or math backtrace. Python worker resources still remain after Close;
full worker release and EPUB remain deferred. Pause after this firmware delivery.

## Stage 29: icon UI, SD selectors and reader preflight (2026-10-03)

Browser actions move to the bottom; app exit reloads the current folder. Shared
keyboard spans 480 pixels. Console titles, history labels and help footer removed;
Power, app menus and errors use fixed line icons. Reader retains page numbers,
chapter titles, words and dictionary content. Binary errors use a circled cross.
Fonts rescan /sd/fonts, including family subfolders (four levels, 31 external
faces). Supported: TTF and TrueType-outline OTF, not CrossPoint cpfont/CFF.
Existing recursive StarDict catalog uses /sd/dictionaries and its chooser.

Reader cache directory is stat-checked before mkdir; exact errno is logged.
Math initialization moves to first formula rendering, after cache creation. This
is a targeted attempt at the reported failure, not a confirmed device fix.
Python worker lifetime/allocation problem is unchanged and remains next work.

Browser/new-file/editor, mixed reader/math and dictionary host checks pass.
Icon previews inspected; native ESP32-S3 build, partition fit and image validation
pass. Application: 2,588,752 bytes; SHA-256
465acbd753700638caee293bd8322af8e44390aa5e6935abb7a6fcfecd4ac67c.
Evidence: results/stage29. Device acceptance remains pending.

## Stage 28: device observations, documentation only (2026-10-03)

User confirms Stage 27 boots and Console evaluates 1+1. Markdown cache creation
fails; later Console allocation fails after other OS use. Investigation notes
and confirmed Python resource-retention mismatch: docs/DEVICE-TEST-NOTES.md.
Close currently resets the VM but retains task/heap/mutex; user requires full
shutdown and resource release. Exact later allocation message conflicts with
the normal same-boot reopen branch; reset/state/error details remain unverified.
No code/config changes, build, or new firmware. Pause coding per user request.
Next authorized coding stage: focused diagnostics, Python lifetime fix, and
cache repair using actual filesystem error evidence.

## Stage 27: defer math allocation (2026-10-03)

serial-log2 confirms Stage 26 passes the constructor-stack crash but aborts in
esp_startup_start_app at app_startup.c:86: main-task creation returns failure.
The four heavy math maps still allocated before the scheduler/main task and
internal DMA reservation. They now remain empty through global construction;
explicit generated init functions populate them once inside ink_math_init,
only when Markdown math is first opened. Incremental insertion is retained.

Native build/image validation and rebuilt mixed Markdown/math host reader pass.
This fixes the identified early-allocation pressure; hardware startup/remaining
heap capacity are not yet measured. Rollback/boot confirmation stay unchanged.
Current application: 2,583,904 bytes; SHA-256 a36fdfeeae8e9c1968b97889b828f96b9d679e8008a24fa2cab073c166ce1b9c.
Evidence: results/stage27/startup.txt. Next: device startup, console/reader,
sleep/wake; capture a new boot log if another initialization failure appears.

## Stage 26: startup rollback fix (2026-10-03)

User OTA dump confirms app0 sequence 9 VALID, app1 sequence 10 ABORTED,
with both CRCs valid. The USB log confirms the delivered Stage 25 image starts,
then crashes in MicroTeX's symbol-table global constructor before app_main.
The old image subsequently boots. PSRAM detection/test passes.

The symbol map initializer_list creates roughly 40 KiB of temporary objects on
the pre-scheduler stack; its overflow corrupts heap metadata. Shared CMake
overlays now build symbols, formula mappings, predefined formulas and macro
commands incrementally. Constant descriptor tables stay in flash; map entries
and first-key-wins semantics are preserved. Upstream source cache is untouched.
The symbol constructor entry frame is now 80 bytes. Inspecting all global and
static-initialization entry frames in the new ELF finds a maximum of 2,048 bytes
(direct frames, not cumulative call-stack measurements).

Native build and application checksum/hash validation pass. Rebuilt host reader
passes mixed Markdown/math, fallback, H2 navigation, paging and Close checks.
OTA confirmation timing, rollback, bootloader and partition table are unchanged.
New application: 2,588,912 bytes; SHA-256 110ed8d8d8d15833af4b6368db149edb1fd3f2ebcee36fafcd5f5e915f9ff904.
Evidence: results/stage26/boot-fix.txt. Next: install this application with the
existing web installer and verify native UI/console, then sleep/wake. If it still
rolls back, capture the new USB log; on-device success is not yet observed.

Previous checkpoint — Stage 25: remaining non-EPUB integration.
Committed stage boundary: pause before starting further implementation.

Final pre-test check (2026-10-03): rebuilt the committed Stage 25 application;
ESP32-S3 / 16MB DIO / 80MHz header, checksum/hash and OTA-slot fit pass.
SD ZIP CRC and all 30 manifest resource hashes pass; reader/math/navigation checks
also pass using the exact supplied SD math tree. No functional code changes.
Current download SHA-256: 15969001175f5f592be4c9ebad02c4c64cb7c39eb2a43b1c5f09db3b6b35eec2.
The rebuilt image remains 2,767,840 bytes. Earlier Stage 25 hash below is historical.
Evidence: results/stage25/final-check.txt. Historical pre-test assessment; superseded by Stage 26 above;
physical timing/heap/panel/Wi-Fi/sleep acceptance remains pending.

## Current stage: 25 delivered
- User requested the full non-EPUB remainder in this batch, overriding the usual
  small-stage scope. Pause after commit/delivery; EPUB remains explicitly excluded.
- `ink_math/ink_font.c` adds shared FreeType UTF-8 drawing, bundled DejaVu Sans
  Mono 2.37 (343,140 bytes, retained licence), and up to 31 SD faces in `/sd/fonts`.
  Power is the font selector. Fixed cells preserve hit geometry; unsupported
  glyphs fall back. Editor/viewer/console preserve UTF-8, including split output.
- Power orientation/calendar editor with +/- fields, validation, PCF8563 write/
  restart, TLS system clock update. CW2017 percentage/charging requires a running
  gauge and verified resident OEM profile; errors show unavailable. No profile
  writes. Power settings last the session; dictionary choice persists.
- Landscape Markdown reflows at native 800x480, keeps nearest numeric page;
  remaining screens keep compact fixed hit grids with unstretched glyphs.
- Routine display updates use pinned FreeInk differential B/W sequences on all
  three panels, OLD-plane sync and UC8279 +120 gate partial window. Full only at
  first paint/first redraw after controller sleep or manual Refresh screen.
  No periodic clear/custom LUT; independent input capture retained.
- Native Python open/io, .py imports, os/uos, JSON and small time module. Script-
  folder cwd and `/sd/lib` import path; eight tracked streams. Paths stay on SD,
  firmware caches excluded. Stop/Close clean handles/readers/listings before reset.
  SD file slots raised to 32 for retained fonts/dictionaries plus VM streams.
- `inkpy.wifi/wifi_status/wifi_off/http`: Python-only Wi-Fi and streamed HTTP/HTTPS
  GET/POST/PUT/DELETE to SD, headers/body, checked roots/hostname with RTC date.
  RAM credentials, exclusive temporary/no-overwrite rename, abort/error cleanup.
  Polls between operations, 500ms socket timeouts; SDK DNS/handshake delays unmeasured.
- Sleep stops Wi-Fi after VM pause, reconnects before resume; remote requests may
  expire. No runtime limit/forced deletion or socket/machine/thread/input() API.
- Host files/imports/JSON/abort/reopen, Unicode/Power/orientation, reader/math/
  dictionaries/browser/text and 8 MiB editor checks pass. Native product build and
  checksum/hash pass; previews inspected. Evidence: `docs/results/stage25/`.
- Download updated to Stage 25 app-only image: 2,767,840 bytes; SHA-256
  `0c670413d9c6bad52b20f714ad8bb691643654b531e73b94bdcd433b7e85e182`.
  Math resource archive supplied for manual SD copy. No device flashed, no map/
  bootloader change; existing recovery retained. FIRMWARE.md/PYTHON-DEVICE.md usage.
- FreeInk 111fdcc7f0176c3ee38391a160ee296bf492dbd8 partial/gauge source rechecked:
  https://github.com/Free-Ink/freeink-sdk/tree/111fdcc7f0176c3ee38391a160ee296bf492dbd8
  Dependency revisions unchanged. Font source: DejaVu 2.37, system package font,
  exact committed bytes and licence; text face remains single-owner on UI task.
- Remaining: EPUB, physical integration tests and polish. Exact next task: user
  tries combined firmware, fix concrete issues; measure heap/stack, typing/ghosting,
  gauge, Wi-Fi/TLS and Python Stop/Close/sleep. No host check establishes runtime.

## Previous stage: 24 delivered
- `components/ink_dict/` implements normal StarDict metadata/index/data/synonym
  parsing, disk-backed binary search, text-field extraction and paged definitions.
  Supports plain/gzip/dictzip, 32/64-bit records within native file-seek limits,
  duplicate keys/aliases, UTF-8 and HTML/XDXF text. Media/locale fields are labelled
  omitted, not executed/rendered as another feature. No new dependency revision.
- Reader taps identify words across styled and wrapped runs; lookup popup has
  Change dictionary and Close. Only this popup opens the paged chooser. Switching
  retries the word; Home dismisses the topmost modal and book page is retained.
- Dictionaries live under `/sd/dictionaries` (nested folders supported); hidden
  owner-marked `/sd/.inkpy-dict` stores expanded data/offsets/definitions/selection.
  Choice survives reboot; active data is reused across books during the boot.
  Source files remain read-only. Cached handles are released before editing in
  the dictionary folder. SD file slots raised to 16 for simultaneous streams.
- Preparation and long definitions are bounded-memory streaming operations, with
  cooperative yielding. First preparation still blocks UI dispatch while capture
  queues events. Persistent prepared-cache reuse/progress/cancel are deferred.
- Dictionary engine/UI host tests and reader/math regression passed; native
  combined product builds, image checks and symbol verification passed. Popup/
  chooser previews inspected. Evidence: `docs/results/stage24/`; instructions and
  format limits: `docs/STARDICT.md`. Hardware runtime/taps/resources unverified.
- Download remains Stage 20. Text glyph coverage/fonts, partial refresh, remaining
  power controls, Python file/network bindings and EPUB remain pending.
- Next bounded task: proper text fonts and Power-menu font selection, improving
  reader and dictionary glyph coverage without adding theme/language settings.

## Previous stage: 23 delivered
- `components/ink_reader/` consumes existing streamed layout indexes on SD and
  renders text/formula runs into the native panel frame. `.md` taps open it;
  side buttons page, Home shows page/chapter and Go to chapter/Go to page/Close.
  Power menu overlays reader and prompts; wake retains reader state.
- H2 headings now start a new page: chapter selection and current page/chapter
  agree. One fixed behavior, no chapter-layout setting. Chapters list pages by
  side buttons; page entry uses a small numeric keypad. No swipes/status header.
- Disposable cache is `/sd/.inkpy-reader`, guarded by an owner marker; indexes
  regenerate per open and are removed on Close. Source remains read-only. Layout
  receives caller-owned SD bitmap spool, avoiding unsupported `/tmp` assumptions.
  A scheduling hook yields while indexing; indexing still blocks UI dispatch,
  though input capture continues. Cancel/progress UI and reusable caches deferred.
- Math uses the unchanged pinned MicroTeX/FreeType backend and `/sd/inkpy/math`
  fonts from `scripts/prepare-math-sd.py`. Missing assets/unsupported notation
  preserve formula source. Text currently uses scaled fixed ASCII bitmap glyphs,
  with non-ASCII fallback and basic bold/italic; selectable text fonts remain pending.
- Browser build verifies existing dependency pins and applies fixed exceptions/
  RTTI, 32 KiB main stack, PSRAM allocation profile. SD open-file limit is 12 for
  source/index/spool/font streams. No new upstream revision or SD transfer feature.
- Product compiles/links; esptool checksum/hash and symbol checks passed. Portable
  device-renderer workflow and existing 255 layout checks passed; page/menu previews
  inspected. Evidence: `docs/results/stage23/`. Native VM/reader execution, memory
  margin, physical touch, sleep and panel behavior remain unverified.
- Download remains Stage 20. Full refresh remains current; StarDict/selectable
  fonts, orientation/time-setting/battery, Python file/network bindings and EPUB
  remain pending.
- Next bounded task: StarDict word lookup and dictionary selection in the reader.
  Preserve user preference for useful feature coverage before broad optimization.

## Previous stage: 22 delivered
- `main/browser_app.c` connects the native Python worker to Console/Execute,
  keyboard submission, history paging, Home Stop/Close/Cancel and long Home Close.
  Globals persist until Stop/Close; cleanup is acknowledged before browser return.
- Manual sleep waits for VM pause acknowledgment; wake resumes it. Busy Python
  inhibits auto-sleep, without a script runtime limit. Failed pause handshakes
  leave the device awake. Editor work is flushed before sleep.
- `main/input_capture.c` captures independently every 10 ms into a 256-event FIFO,
  including during blocking display refresh. UI drains batches and coalesces draws.
  `main/input_events.c` owns discrete gesture classification. Overflow logs are
  explicit; this finite queue is not a guarantee against unlimited input bursts.
- Touch/RTC I2C access is serialized. Sleep pauses capture by acknowledgment and
  consumes stale/wake gestures on resume. The UI still owns display/frame access.
- Native NLR now matches pinned upstream ESP32 `MICROPY_NLR_SETJMP`; the combined
  link exposed an out-of-range jump in embed's architecture-selected assembly NLR.
  No dependency revision or cached upstream source was changed.
- Product and separate Python diagnostic compile/link; own code uses warnings as
  errors. Host Python script/REPL and console workflow checks passed. Input tests
  cover repeated taps buffered before consumption, ordering, swipes and buttons.
  Results: `docs/results/stage22/`. No physical native execution, memory margin,
  touch speed or sleep/resume claim follows from these checks.
- Downloadable firmware.bin remains Stage 20; user will test after integration.
  Full refresh remains current behavior; partial refresh is still pending.
- Next bounded task: connect existing Markdown/math reader to the device browser.
  StarDict/fonts, missing power controls, Python file/network bindings and partial
  refresh remain pending. Do not implement the entire remainder in one run.

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
- Power/editor/console/reader menus are connected in source; physical checks
  of the new integrations are pending.

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

## Stage 19 outcome
- Added power-menu Refresh screen requirement to UI.md: close menu, full refresh
  of underlying view, preserve page/cursor/session. Power menu remains pending.
- Restored unchanged board/display/pins/partitions/bringup files from current
  remote. SDK v5.5.5 b774170ff46c393eeb5e495ea37936038d3f4f4f and recursive
  submodule pins restored clean; official esp32s3 tools installed. Host SDK path
  /workspace/scratch/5fae3dc186b5/esp-idf, build tools in ../build-tools/bin.
  Reuse this cache; do not repeat downloads unless missing after environment reset.
- Xtensa GCC esp-14.2.0_20260121, CMake 3.30.9, Ninja 1.13.2. Run source export.sh
  with build-tools/bin on PATH, IDF_PY_BUILD_JOBS=4, bash scripts/build-browser.sh.
  Separate build-browser/sdkconfig; browser ON, math diagnostic OFF, stack 20480.
- Fixed device-only Home handler/button name collision. Inspected pinned FatFs
  rename: no overwrite support. Device Save now uses sibling backup/replacement
  and rollback; host keeps POSIX rename. Backup sequence isn't power-loss atomic.
- Browser/editor ESP32-S3 build/link pass, own code warnings-as-errors. App image
  344544 bytes, configured slot 0x7e0000, 96% free. esptool checksum/hash valid.
  Both Save branches pass host 8 MiB/gap/page/Tab check. No physical result.
- App-only firmware.bin supplied as downloadable artifact. Hash/results and scope
  in docs/FIRMWARE.md and results/stage19. No merged image, flash, runtime heap,
  stack high-water or device recovery assumption. Markdown/Python remain host-only.
- Stage boundary: commit and pause. Preserve unrelated Python run.sh mode change.

## Exact next bounded stage
Stage 21 delivered separate native ESP32-S3 embedding/worker and serial diagnostic.
Reuse pinned MicroPython 19e685eca906a5a602135a485976253e705297d0; separate generated
package/config, unchanged host package. FreeRTOS single-owner VM, bounded command
and output model, cooperative pause/Stop/Close, cleanup acknowledgment and reopen.
256 KiB PSRAM heap, 48 KiB internal stack; heap/task reserved after Close, no script
continues and globals discarded. Upstream ESP32 GC register spilling adapted with
MIT notice; native Xtensa NLR/uncatchable abort linked. Fixed IDF assertion-handler
collision. Native build/image integrity and linked symbols checked; no hardware
execution, GC/stack measurements or runtime pause/stop claim. Product firmware.bin
unchanged, no new intermediate image supplied at user's request.
Recorded new requirement: capture/queue all taps/buttons during display refresh,
preserve event order/repeated letters; coalesce drawings only. Blocking display
still drops input. This stage focuses on Python, input/display task split deferred.
Next Stage 22: connect Console/Execute and keyboard/Home controls to FreeRTOS
adapter; arbitrate SD and require pause acknowledgment before sleep. Keep output
refresh throttled and render snapshots outside locks. User will test after features
are integrated; do not ask them to flash each development checkpoint.

Stage 20 follows new hardware feedback instead of the previously planned VM worker.
Hidden files/folders filtered; InkPy title removed; editor filename only in its
Save/Discard/Cancel menu; bottom keyboard with matching tap coordinates; footer
removed. Global Power modal preserves browser/editor state and gates underlying
input. Brightness/warmth +/-5, light toggle, night inversion and Refresh screen
work; Home/Power closes it. RTC date/time only in this menu; battery unavailable
until validated. Orientation/time-setting/font rows remain explicit pending notices.
All remarks recorded in UI.md, including global menu for future reader/console.
Routine full refresh is STILL present; manual refresh button closes modal and
refreshes underlying screen. Fast/partial panel refresh is a distinct pending
driver task; user prefers manual cleanup, optional 20-update policy not selected.
Browser hidden-fixture check, Power/editor model and render checks, existing host
browser/Python/editor workflow passed. Power/editor previews inspected. Pinned
ESP32-S3 build and esptool checksum/hash validation passed; new image in FIRMWARE.md.
No new physical-device results inferred. Next implementation: resume bounded
FreeRTOS MicroPython integration (now Stage 21), then panel-specific fast refresh.

Stage 19.1: user confirms unlocked X4 Pro and reports browser firmware worked,
was upside-down, and CrossPoint returned after sleep/wake. docs/BOOT-FIX.md owns
the evidence/limits. Added main/boot.c: reset/slot/OTA-state reporting and trial-boot
confirmation after first successful display refresh; preserve startup-failure
rollback, prohibit anti-rollback/eFuse updates. Installer writes OTA state NEW;
missing confirmation was missed by the earlier audit. Reset cause is still unknown.
Device framebuffer now rotates 180 degrees; short/long touch mapping follows it.
Host orientation probe checks all pixels and matching touch coordinates. Sleep
return now logs error/wake cause. Next hardware task: check upright/aligned UI,
reboot persistence, then sleep/wake; capture reset logs if it restarts. Keep existing
bootloader/partitions and USB recovery. No claim that sleep reset is resolved.

Audit details remain in docs/SAFETY-AUDIT.md: pinned image/SDK/pin checks,
sanitizers, failure rollback, large-file/browser probes, static analyzer. Its
installation hold reflected knowledge before the user's unlocked-device/test report.

Stage 22: integrate the device console UI using the Stage 21 FreeRTOS VM worker and
existing console model/keyboard. Reuse the compiled embedding and cooperative
Stop/Close/pause protocol.
No runtime timeout, arbitrary task suspension/deletion or premature sleep while
VM active. Keep networking, reader and editor polish out of this stage. No flash
without a verified installation/recovery route for the actual device.

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
