# InkPy checkpoint

## Stage 45: README wording and Dependabot maintenance (2026-10-05)

Removed the requested introductory font/size phrase and Arabic feature bullet
from README; device features are unchanged. Merged reviewed Dependabot PRs
#2 (checkout 7.0.1), #3 (upload-artifact 7.0.1), #1 (download-artifact 8.0.1).
Diffs only update SHA-pinned official GitHub Actions. Inspected upstream action
definitions: Node24 on hosted runners, existing input names/default archive and
extraction behavior fit our workflow; download now fails on digest mismatch.
Checkout still disables credential persistence; job token permissions unchanged.
No firmware dependencies, code or released assets changed. No open PRs remain.

Release workflow now runs only via workflow_dispatch: maintenance merges must
not build or try republishing v0.1.0. Build guide updated. Local publication checks,
workflow YAML/input/pin/permissions inspection and whitespace checks passed.
PR #3 source check succeeded; other checks were canceled amid earlier runner
issues. Full automated build/artifact cycle still needs validation before the
next release. Preserve unrelated prototypes/python/run.sh mode change.
Next: user feedback; no device reflash needed for this stage.


## Stage 44: public documentation and release preparation (2026-10-05)

User confirms Stage43 works and authorizes publishing InkPy publicly. No firmware
behavior changed. README now presents a mathematical device: Markdown/LaTeX and
MicroPython first, limited EPUB second, StarDict, installation/recovery, release
asset links, examples and Python clock setting. MIT covers original InkPy code;
dependency notices and licenses remain separate. Public docs index distinguishes
historical stage reports from current usage. Remote AI examples are preserved.

Release workflow builds the pinned ESP-IDF application, packages math SD assets,
notices/checksums and source including fetched math/Python dependencies, then
creates a new versioned GitHub Release. Build has read-only token; only publish
has contents write. Existing releases are not overwritten. Actions are SHA-pinned.
Added a small source/public-link/Python syntax check, monthly Actions Dependabot,
and SECURITY.md. GitHub account switches still require authenticated admin UI.

Review found no credential-pattern matches among 1,203 local historical Git
objects; current remote examples have empty credential fields. This is a targeted
publication review, not a comprehensive security audit or full remote-history scan.
Checks passed: maintained public-page relative links, tracked Python AST syntax,
current tracked-file credential patterns, staged whitespace, workflow YAML parse,
release packaging, all artifact checksums, ZIP integrity/math paths/notices, and
source archive contents including LGPL and fetched dependencies. No firmware
rebuild needed locally: firmware behavior is unchanged. Publication state will
be reported after the push and GitHub workflow inspection. Existing
Stage43 binary: 2,728,224 bytes, SHA256
6617b4fb50e8e7309234b27713d65695297e7a9bb6f6e1862da760383c79416b.
Release rebuild may have a different binary hash; no new device test is inferred.
Unrelated prototypes/python/run.sh mode change is not staged.

Publication completed after user finished browser sign-in. Repository is public;
v0.1.0 is published at https://github.com/cSohaib/InkPy/releases/tag/v0.1.0.
GitHub confirms all five assets uploaded: exact tested Stage43 firmware.bin,
inkpy-sd-resources.zip, inkpy-source.tar.gz, licenses.zip, SHA256SUMS. GitHub's
firmware digest matches the Stage43 SHA above. All local archive checksums and
ZIP integrity passed. Device-build active code matches remote modulo trailing
blank lines; source archive contains the local build sources plus fetched deps,
while GitHub's standard source archives include the full tagged repository.

Enabled private vulnerability reporting, dependency graph, Dependabot alerts and
security updates, secret scanning and push protection. Active ruleset 24528086
protects the default branch against deletion and force pushes, with no bypass;
normal direct commits remain allowed. No expensive/custom CodeQL setup added.
Source-review workflow has passed on GitHub. Initial Release run 37365050279
failed at Git dubious ownership inside the ESP-IDF container before compilation.
Fixed by trusting only GITHUB_WORKSPACE (commit 06539669342c8864c4cc31d230b8171f3065f046).
Run 37367095137 was canceled while queued; first release published manually from
the already tested binary. Full CI rebuild/publish still needs validation before
next release; automation does not overwrite existing releases. Next bounded task:
user feedback or validate CI with a new version when firmware changes are ready.


## Stage 43: one bundled font, denser text, Power dictionary (2026-10-05)

User confirms Arabic works perfectly; prior failure used the wrong download.
Removed SD font catalog/scan/select APIs, fallback face, Power picker state/icon,
font-dependent EPUB keys and heap path catalog. One bundled Unicode face remains;
math's separate formula resources are still necessary and unchanged. Text height
stays 26; cell 17 -> 14, editor/viewer/console grid 27 -> 33 columns, dictionary
24 -> 33 columns. Reader headings keep emphasis but use the same height. EPUB
e43 namespace rebuilds geometry once. No proportional-layout framework added.

Power's former Aa tile is AZ: opens keyboard query, Enter submits, bottom AZ
opens shared dictionary picker. Definition/picker/paging reuse reader drawing
and dictionary APIs through a separate zero-page popup; underlying app state is
kept. Home returns to input then app; long Home still discards/kills/goes root.
Keyboard input queues remain unchanged. Font rendering means rasterizing the
one bundled face, not allowing font selection; host bitmap fallback is test-only.

CrossPoint reference inspected: https://github.com/crosspoint-reader/crosspoint-reader/blob/develop/docs/dictionary.md
describes persistent .qidx/.sidx sidecars. InkPy already has fixed-width SD ordinal
indexes and binary search, so no source copied/dependency added. Reuse existing
decompressed data/ordinal caches using source path/size/mtime signature; rebuild
on change/missing/truncated ordinal. Signature published after successful setup.
Same-size changes with unchanged mtime and undetected cache corruption remain
ordinary cache limits. Source dictionaries are read-only; RAM stays bounded.

Checks: native build/image validation; Arabic reader both orientations/tap,
justification/64 formulas, StarDict formats/aliases/errors plus reuse/truncation/
invalidation regressions; Power/input/font UI and visual preview. Evidence:
results/stage43. App 2,728,224 bytes; SHA-256
6617b4fb50e8e7309234b27713d65695297e7a9bb6f6e1862da760383c79416b.
No physical test here. Unrelated Python script changes on remote preserved;
local prototypes/python/run.sh mode untouched. Next: user tests spacing and
typed lookup, including Power overlay while Python/editor/reader active.

## Stage 42: UTF-8 SD filenames; Arabic report unresolved (2026-10-05)

User reports reader Arabic still separated/LTR, missing Arabic-named EPUBs copied
into books/, but Python-created Arabic-named Markdown visible. Reader shaping
was present in the delivered Stage 41 image; host reader still shapes the fixture,
including logical tap lookup and both orientations. No reproduced reader cause
yet: do not mark Arabic on-device working. Need one failing source document and
comparison with fixtures/reader-arabic.md in the freshly installed image.

Confirmed filesystem mismatch: native FatFs API used ANSI/OEM CP437, while app
and Python paths are UTF-8. Enable CONFIG_FATFS_API_ENCODING_UTF_8 in base and
browser defaults; cached browser config is overridden too. Filename presentation
unchanged. This addresses encoding compatibility; actual missing-file report
needs device confirmation. No file conversion/renaming, formatting, or source
modifications. Names previously created through the incorrect OEM API may have
been stored as mojibake and can require correction from a computer.

Browser regression covers Arabic EPUB visibility/routing in a subfolder. Reader
fixture and 500 bidi cases pass, including host unsigned-char build matching
Xtensa's char signedness. Native config confirms UTF-8; build/partition fit and
esptool checksum/hash pass. App 2,729,536 bytes; SHA-256
874c9054ec0837120d10571345f19ed278a9653d4ca98d5b16b800543fd8c379.
Evidence: results/stage42. No physical testing. Next: test copied Arabic EPUB
names and send failing Arabic reader document; no speculative rendering patch.

## Stage 41: Arabic in EPUB/Markdown (2026-10-05)

Reader-only Arabic joining, lam-alef ligatures, basic vowel marks, mirrored
punctuation and mixed-script ordering. Each displayed line independently uses
its first letter in logical order: Arabic chooses RTL; Latin or no letters
chooses LTR. Digits/punctuation do not select direction. Final RTL lines align
right; automatic prose wraps retain default justification. Tables keep explicit
alignment. Inline formulas/images are neutral objects, not reversed internally.
Chapter/dictionary labels also use reader shaping; other apps/keyboard unchanged.

Small C adapter uses pinned FriBidi 1.0.16 (Unicode 16.0.0), LGPL-2.1-or-later;
source, licence and provenance are in ink_layout/vendor/fribidi. Fixed 128-codepoint
line budget; layout context size unchanged. Latin-only ordinary lines keep the
old fast path. Shaped cache runs preserve original logical words for StarDict
lookup. Bundled font already contains Arabic; selected SD fonts missing glyphs
fall back to the bundled face. No new SD assets. Typography remains fixed-cell,
not a full OpenType/calligraphic engine. EPUB cache namespace e41 rebuilds once.

Checks: Arabic direction/maps/glyphs/logical tap in both orientations; existing
Markdown/math/tables, stored/deflated/heavy EPUB, incremental restore and 64
expanded formulas pass. ASan/UBSan passes 500 bounded mixed-script inputs
(LeakSanitizer unavailable under runtime /proc restrictions). Rendered Arabic
preview inspected. Evidence: results/stage41. ESP32-S3 build and image validation
pass; app 2,729,104 bytes, SHA-256
b16b302cf0bac3a42a5cc589207e6b26f4c98982f3b5002f02751764e28acc35.
No physical flashing/testing. Unrelated prototypes/python/run.sh mode preserved.
Next: user tests mixed Arabic/Latin documents and original-word dictionary lookup
on X4 Pro. Stop here; no additional feature work in this stage.

## Stage 40: default justification and expanded math (2026-10-05)

User confirms Stage 39 works and requests justified EPUB/Markdown paragraphs
plus the exact supplied math commands and matrix/bmatrix/vmatrix environments.
Shared C layout distributes spare line width over eligible interword spaces on
automatic wraps only. Final lines/hard breaks, headings, code, display formulas
and table-cell alignment retain natural spacing. Existing run format, word lookup,
fixed line buffer and continuation ABI remain unchanged; runs split at stretched
spaces rather than adding a per-character renderer or configurable alignment.
Inline math/images participate in width calculation. EPUB namespace e40 prevents
old layout caches masking the change; first opening rebuilds the current document.

Expanded the small preflight command/environment allowlist in ink_math.cpp.
Pinned MicroTeX already implements all requested notation; no new dependency,
engine patch or SD assets. Existing complexity/size limits and source fallback
remain. fixtures/markdown-math-expanded.md exercises 64 formulas including every
requested command and all three environments; no fallback in portrait/landscape.

Checks: test-justify.py verifies filled wrapped lines, natural final lines, exact
text, headings/code/hard breaks, mixed styles/Unicode/math and bounds. Existing
Markdown/math, tables, stored/deflated EPUB, indented images/tables, chapter/cache
navigation and incremental restore pass. Matrix preview inspected. Evidence:
results/stage40. Native ESP32-S3 build, partition fit and esptool checksum/hash pass.
Application 2,632,928 bytes; SHA-256
d922933ad07132441f6281850a61b81c0f0c3257efa68d56a1ed5fe207f09eb3.
No physical testing/flashing here; unrelated prototypes/python/run.sh preserved.
Next bounded task: user tests justification and expanded math on X4 Pro, then
reports concrete issues. Existing parser/math resource limits remain intentional.

## Stage 39: EPUB whitespace, stack relief and diagnostic removal (2026-10-04)

User confirms large books now open and some images work; other books crash or
show literal image/table Markdown. Inspection of the supplied Stage 38 log finds
about 8 MiB heap free but a historical task stack minimum of 40 bytes (the old
stack-words label was incorrect: ESP-IDF returns bytes). Without a panic trace,
stack overflow remains a strong suspect, not a confirmed cause for every book.
Loading phase text called FreeType's monochrome renderer with a 16 KiB local
pool from within EPUB/ZIP extraction; ZIP itself had 8 KiB local buffers.

Loading now paints only one primitive hourglass, with no text/font call or
periodic redraw; cancellation/yields remain. ZIP scratch is a checked fixed 8 KiB
heap allocation freed on success/error; native extraction stack frame drops from
8,320 to 640 bytes. Removed temporary SD logger, debug API,
phase strings, image-header probes and calls from product/prototype code.
Cancellation during OPF import goes through existing cleanup instead of leaking.

XHTML whitespace collapses outside preformatted code, with row/cell boundaries
kept intact. This fixes reproduced indented SVG/JPEG wrappers being parsed as
Markdown code and whitespace splitting generated tables. JPEG extension never
controlled decoding. Chapter markers no longer insert blank lines into content;
body IDs now work. Existing table-layout limitations/publisher styling remain.
Cache namespace e39 forces fresh conversions after upgrade without changing books.

Checks: EPUB2/3 stored/deflate, nav/NCX fallback, aliased namespaces/image URI,
missing TOC, heavy chapter, repeated reopen/rotation; new indented JPEG/table
fixture asserts bitmap/rule runs, body anchor and exact preformatted spaces.
Existing Markdown/math and table regressions pass. Evidence: results/stage39.
Native ESP32-S3 build/partition fit and esptool checksum/hash pass. Application
2,632,208 bytes; SHA-256 bb379bc4f00c0d7d5fb2084c93b4cc277c09431d456549f85a1b55a157f80b90.
No physical testing/flashing here. Unrelated prototypes/python/run.sh mode preserved.
Next: install/test formerly crashing books and indented covers/tables on X4 Pro.

## Stage 38: lazy EPUB, persistent cache and temporary diagnostics (2026-10-04)

User device results for Stage 37: loading froze the UI, TOC filenames/errors,
image placeholders and no cache reuse; a heavy book was interrupted. The four
EPUBs have not been supplied here, so their specific failures are not yet confirmed.

EPUB opening now reads metadata/TOC and imports only the requested spine document.
Layout advances at physical-line/parser boundaries until a screen is available;
long bounded blocks can generate a few screens ahead. No total-page indexing,
page numbers or Go to page for EPUB; Markdown behavior retained. Chapter jumps
load their document/fragment directly; Previous/Next cross spine documents.

Pointer-free scanner continuation (about 27 KiB, excluding the render scratch
bitmap) and generated runs survive close/reboot. Book/font/stat/orientation/version
key selects an owned e38b cache; last document/screen resumes. Streams/callbacks
are supplied afresh on restore. Incomplete appended output is truncated back to
saved offsets. Checkpoint publication handles FatFS rename's EEXIST behavior;
state-next can recover the complete save if the old state was removed first.

TOC handling tries EPUB3 nav, loose TOC links, NCX/spine toc and guide fallback.
Malformed navigation no longer prevents book opening. Namespace aliases, large
publisher attributes, same-document fragments and archive-root image references
handled. Missing TOC uses numeric entries, replaced with title/first heading as
that document opens. Current-document TOC filtering avoids per-id whole-TOC scans. Forward manifest/
spine lookup avoids quadratic scans for ordinary reading-order metadata; screen
title lookup reads local anchors instead of reopening every TOC entry.
Images are extracted only when reached in layout; angle destinations support
spaces/parentheses. Existing PNG/baseline JPEG restrictions remain.

Hourglass/phase appears during actual work. Long Home cancels loading. Scheduler
yields are throttled instead of adding a tick to every inflater chunk. Temporary
SD-root inkpy-epub-debug.txt logs TOC/ZIP/chapter/cache/image failures and device
heap/stack headroom, bounded around 512 KiB. Remove diagnostics in a later stage
after analyzing the user's four books. See EPUB.md for reproduction instructions.

Checks: EPUB2/3 stored/deflate, lazy next-document access, cache reopen without
progress callbacks/reconversion, fragments and rotation; malformed-nav/NCX,
namespace aliases/oversized styles/URI image names, absent TOC title fallback.
Continuation save/restore at every step matches uninterrupted draw/page/chapter
streams byte-for-byte. Existing mixed reader and table/math regressions pass.
Evidence: results/stage38/checks.txt. Native ESP32-S3 build and esptool validation pass. Application 2,633,808 bytes;
SHA-256 4a557c53829dcf9eb3525d87b225a54b5edd04226bf0bde3ed01ba29dfe30954. Image details in FIRMWARE.md.
No physical testing or flashing here. Unrelated prototypes/python/run.sh mode
remains untouched.

Remaining limit: current XHTML document still decompresses/converts in full before
its first screen; a huge single chapter can wait. Going Previous into an unvisited
chapter must reach its last screen. No cache eviction yet. Next bounded task:
review SD diagnostics from the four real books, fix their concrete remaining
failures/bottleneck, then remove temporary diagnostics.

## Stage 37: EPUB and shared images (2026-10-04)

Implemented restricted EPUB 2/3 import using container/OPF spine, nav/NCX TOC
and fragment anchors. TOC chapters replace H2 inference only for EPUB. Stored
and deflated ZIP entries stream through the existing inflater. XHTML is reduced
to supported Markdown plus chapter anchors on SD; all layout, tables, math,
translation and navigation are reused. Publisher styles/scripts/fonts ignored.

PNG/baseline JPEG images now work in EPUB and Markdown, with fit-to-screen
monochrome bitmaps and small inline images. Fixed buffers, no full-book DOM or
full-source-image allocation. SVG raster wrappers supported. See EPUB.md for
limits and ink_image/vendor/SOURCE.md for pinned TJpgDec provenance.

Checks: four EPUB archive/version combinations, Markdown images, TOC/fragment
navigation, H2 exclusion, inline/display math, tables, PNG variants/JPEG, missing
image fallback and portrait/landscape. Existing reader/table/dictionary/browser
regressions pass. Results: results/stage37/checks.txt. Native ESP32-S3 build and
esptool validation pass. Rebuilt a damaged zero-byte derived math object/archive;
no source/dependency reset. Unrelated prototypes/python/run.sh mode left untouched.

Application 2,613,200 bytes, slot 8,257,536 bytes.
SHA-256: c5879dd5c56ce90b1396bca62c2986723c0bbc43c62fcaee75b938e9d61bee27

Next: test on X4 Pro with an ordinary EPUB (including images), contents navigation,
rotation and repeated close/open; check available heap/stack and responsiveness.
No device flashing/testing performed here. Keep this stage bounded and pause.

## Stage 36: expanded math and Markdown tables (2026-10-04)

All user-listed commands supported: bar, beta, epsilon, frac, left, min, mu, pm,
quad, right, sigma, sqrt, sum, text, times. Added missing bar/epsilon/mu/sigma/times
to native math preflight; existing MicroTeX implementation/assets handle them.
Fixture fixtures/markdown-tables-math.md includes average, Cpk and table formulas.
Nine expressions render without fallback in portrait and landscape at body size.

Tables now have equal-width columns, bold headers, left/center/right alignment,
word wrapping (character wrapping for oversized words), borders, inline text
styles and inline math. Wide cell formulas retry at a smaller size down to 12px,
then preserve source if still oversized. Empty cells/escaped pipes supported.
Rows can continue across pages; headers are not repeated. No settings/dependencies.

One reusable SD row scratch file stores Cell records; one bounded line is read
for measuring then drawing each column band. Layout context increases by 384
host bytes to 75,560; neither table/cell length sets RAM use. Rows overwrite the
scratch from offset zero. Reader closes/removes it after indexing and on failure.
New INK_RULE cache runs encode solid rectangles; device draw/word lookup and
host cache/preview understand them. Cache is regenerated on every book opening.

Limits retained: 8 KiB parser block before literal fallback; over-wide or >16
column tables use readable linear cells. No arbitrary HTML tables or merged
cells. Normal cell math keeps existing complexity/size limits/source fallback.
Tests: all commands, aligned/wrapped/style/Unicode/empty/escaped-pipe tables,
1000-character row with exact text preservation across pages, 40 subsequent rows,
inline formulas, landscape and no-math path; prior reader/math/rotation/navigation
and StarDict suites pass. Host preview decoder and device previews inspected.
ESP32-S3 build/slot fit and esptool checksum/hash pass. Evidence results/stage36.
App 2,595,216 bytes, SHA256 fc7cf6339a13383c1bfcf2f8eb8687febf7d1a8836e1176d3901dbaa362aa030.
No physical device test. Existing math SD assets unchanged. Unrelated Python
run.sh mode excluded. Next: user tests real document tables/new commands; EPUB
still deferred. Scoped commit/push and pause.

## Stage 35: keyboard, sorted files and global Home (2026-10-04)

Reader Home order: Chapter / Page / Rotate / Exit / Back. File browser sorts
folders first, then files, case-insensitive alphabetical with exact-spelling tie
break. Fixed 15-entry page selection rescans earlier ranges for later pages:
RAM stays bounded regardless of directory size; later pages incur extra SD scans.
Clamps pages if directory contents shrink. Hidden files remain excluded.

Bottom +/>_ icons shrink to 48 pixels, centered in 88-pixel buttons reaching the
screen bottom. Battery body shrinks to 80×32. Shared keyboard follows requested
digit-first QWERTZ letter layer and full symbols layer with colon/underscore;
Tab remains in bottom letter row. Both layer toggles display abc as specified.

Long Home overrides all overlays, closes reader, discards editor, and returns to
root. Python close uses existing abort/cleanup acknowledgment before releasing
VM/task memory and returning to root; no forced deletion of a live worker.
Short Home behavior preserved. Settings/dictionary selection remain unchanged.

Checks: browser sorting across two pages/two folders/mixed case, routing/Delete,
all keyboard cells and filename entry; actual Home dispatch with owner stubs in
12 context/overlay combinations; font/Power previews and mixed Markdown/math,
Rotate/Exit/Back pass. Previews inspected. Product build and image validation pass.
One empty restored math object/archive regenerated; no dependency-source edits.
App 2,593,104 bytes, SHA256 9d7aad11ab1f503b1c3f652d28f8d0f2f6fcb9ca219e4bb30c0df2fd2262fd4b.
Evidence: results/stage35. No physical device test performed. Unrelated Python
run.sh mode excluded. Next: user tests UI/global Home during running Python;
math command expansion and EPUB remain deferred. Scoped commit/push and pause.

## Stage 34: compact UI, icons and Python-only clock (2026-10-04)

Implemented the requested UI stage. Reader Home adds Back; book/console Close
uses door/arrow Exit, editor Discard uses crossed Save. File menu adds Delete
(unlink regular file, reload list, fall back one page if it becomes empty).
Browser distinguishes file/folder entries and has 64-pixel +/>_ bottom controls.
Icons: open book chapters, # page, AZ book dictionary, 100×100 error, thermometer,
bulb with rays only when on, Aa and half-filled contrast. Scaled line icons stay
code-defined, with no image assets or icon-font dependencies.

Power header shows larger time/date left and percentage inside battery right.
Two 96-pixel rows: minus / bulb or thermometer + value / plus; brightness center
also toggles light. Three 152-pixel tiles: font, refresh, contrast. No time, close,
rotation or separate light rows. Font picker has eight entries and Back.
Removed calendar fields, time UI branches/actions and clock-icon code.

Added inkpy.set_time(year,month,day,hour,minute[,second]): seconds defaults 0,
years 2000–2099, calendar/leap-day and ranges checked before RTC write. None on
success, ValueError on invalid date, OSError on I/O failure. Reuses board RTC
setter/system-clock update for TLS. No timezone conversion or time-service logic.
Usage: PYTHON-DEVICE.md; AI example docs updated. Host RTC function is a stub;
valid/invalid binding calls tested, actual hardware write not claimed.

One body geometry in ink_browser/ink_ui.h: 26-pixel text, 17-pixel cells, 6-pixel
grid margins (Markdown margin 8). Editor 16 rows up to bottom keyboard, console
14 output rows + input, viewer 24 rows. Browser 15 rows; chapters 10/page, narrower
margins. Markdown headings keep hierarchy; body matches other contexts. Existing
bounded storage, shared keyboard and Python lifetime/sleep logic preserved.

Checks: generated Python headers, native MicroPython/files/input/JSON plus clock
validation, browser routing/Delete, Power hit targets/selector/toggles, fonts and
mixed Markdown/math/rotation/Back, StarDict core/popups, editor ASan/UBSan checks
pass. Previews visually inspected. Product build/slot fit, esptool checksum/hash
pass; evidence results/stage34. App 2,592,480 bytes, SHA256 5984998c29faa612030b338b08f0fda22dca8a7f03cbc998bfbe0cde9bb5d054.
No physical UI/RTC/firmware testing performed. Unrelated Python run.sh mode stays
untouched. Next: install/test new UI and inkpy.set_time, then user refinements;
math command expansion/EPUB remain deferred. Scoped commit/push and pause.

## Stage 33: reader-only rotation and resource cleanup (2026-10-04)

User confirms the Stage 32 AI script works on-device. User requests a minimalism/
performance review before feature expansion. Reviewed first-party application,
UI, reader/layout, fonts/math adapter, editor/text, dictionaries, Python/I/O,
input/display/sleep and product build choices; findings: RESOURCE-REVIEW.md.

Removed global landscape state, shared pixel-scaling header, Power orientation
row, font squeezing and special fixed-grid text style. Reader Home has Rotate;
only book pages use landscape. Menus/translation/Power and all other apps stay
portrait. Rotation reindexes, binary-searches the source anchor, retains dictionary;
new books start portrait. The physical 180-degree mounting correction remains.
Display rows batch into the existing 4 KiB DMA buffer (12/15 transactions per
plane rather than 480/600); byte order/padding checked for all panel variants.
Reader menus no longer render/read page runs only to erase them. Font size changes
only as needed. Removed static 19,456-byte font names/path arrays; names derive
from optional heap paths (16 KiB PSRAM-preferred catalog only if SD fonts exist).

Python UI snapshots copy console only on changed revision. Close completes
VM/streams/network cleanup and acknowledges, then UI deletes the parked task,
heap and mutex. Frees 256 KiB PSRAM heap and 48 KiB internal task-stack allocation
plus overhead (RTOS idle cleanup may delay stack reclamation). Removed retained-
worker reopening state; a new console starts a new worker. Stop resets the active
VM; sleep preserves it. Task deletion never interrupts live VM/I/O cleanup.

Checks: font and bitmap reader host tests, both reader orientations and preserved
chapter/source location, menu with draw stream unavailable, Power/fonts/UTF-8,
StarDict core and popup suite, editor ASan/UBSan deterministic checks, native
MicroPython files/input/JSON/abort cleanup and mocked AI script pass. Updated
Python lifecycle diagnostic compiles but was not run on hardware. Product
ESP32-S3 build, slot fit, esptool checksum/hash pass. Evidence: results/stage33.
Application 2,587,056 bytes; SHA256 603e774e23670237f9a609fd5912ba4bfebb7d5c5a230c8a8af93829a90dd9f3.
No device timing/current/heap measurement or hardware regression claim.

Next bounded task: physical Python Close/reopen, math after Close, reader rotation/
word taps and portrait menus; profile measured bottlenecks before further changes.
Whole-document reader indexing, previous-page rescans, glyph caching and region
refresh remain opportunities, not completed optimizations. Math-command expansion
and EPUB deferred. Scoped commit/push and pause; preserve unrelated run.sh mode.

## Stage 32: SD Python OpenAI chat experiment (2026-10-04)

User confirms the markdown-math fixture now works on-device. Wider ChatGPT math
command coverage is requested for a future stage (including bar, mu, sigma,
min and scalable delimiters); no renderer change in this experiment.

Added examples/agent.py and examples/README.md. Runs on Stage 31 firmware with
existing input(), JSON, Wi-Fi and HTTP; no firmware changes/build/delivery.
Editable Wi-Fi/API credentials, gpt-4.1-mini, Responses POST, concise plain-text
instructions and 256 output tokens. Each request includes all successful turns
plus its new prompt; failed prompts roll back. History lives only in VM RAM.
User explicitly allows temporary SD files to keep this simple: reserved
/sd/.openai-response.json is deleted after a request, on normal exit and next
startup. Hard Close/reset may leave this last response until next run; no saved
conversation is restored. API store:false; no remote conversation object.

Verified official Responses/manual-history docs and model endpoint support:
https://developers.openai.com/api/docs/guides/conversation-state
https://developers.openai.com/api/docs/guides/text
https://developers.openai.com/api/docs/models/gpt-4.1-mini
Parse raw output message/content/output_text rather than SDK-only output_text.

Validation: python scripts/test-agent.py passes offline mocked two successful
turns plus HTTP failure, whole-history replay, error rollback and file cleanup.
No live OpenAI call or physical agent test; user must configure credentials and
correct device clock for HTTPS. Long chats may exhaust the Python heap.
Next bounded task: expand math command coverage after this experiment; native
Python worker resource release and EPUB remain deferred. Commit and pause.

## Stage 31: preserve internal RAM during math (2026-10-03)

User confirms Stage 30 input(), JSON and sleep/wake work on-device. $x+1$ still
crashes. serial-log4 matches Stage 30 ELF 5b199a2cb; the original preflight guard
is passed. New trace: lock_init_generic -> recursive FILE lock -> fopen ->
FT_Stream_Open -> FT_New_Face -> face_for -> CharBox/Raster drawing. SDK abort
branch is a failed internal RTOS mutex allocation; exact free/largest blocks were
not captured on-device.

Browser profile malloc/new/realloc now prefers PSRAM at every nonzero size
(CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=0, formerly 256). This is a firmware-wide
ordinary-allocation preference, including math's small map/string/font nodes;
explicit RTOS/DMA/internal allocations remain internal and the 64 KiB reserve
is unchanged. Using the SDK policy avoids replacing upstream allocators or
transient process-wide allocator changes while other tasks run.

Math logs internal free/largest and PSRAM free before/after initialization and
font open. A best-effort 2 KiB largest internal block check before font opening
returns through the existing formula-source fallback rather than calling fopen
when already critically low. This is not an atomic guarantee against every OOM.
Render errors are logged to serial. SDK source checked: heap/heap_caps.c,
freertos/heap_idf.c and newlib/src/locks.c at pinned ESP-IDF v5.5.5.

Mixed Markdown/math host reader passes. Native build/partition fit and image checksum/hash pass.
Application 2,587,392 bytes; SHA256 04ad4881ded7c563fca72fa5c132b27e096e6186d8017493afe5dbacd5606325.
Evidence: results/stage31; physical allocation measurements and LaTeX
acceptance still need device testing. Next: $x+1$, then mixed formulas and Python
opening after math; send new serial math heap snapshots/backtrace if it fails.
Python worker resource release and EPUB remain deferred. Pause after delivery.

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
