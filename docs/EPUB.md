# EPUB and images

Tap a DRM-free .epub in the browser. EPUB 2 NCX and EPUB 3 navigation contents
supply chapter titles and fragment targets; headings do not create chapters.
Without usable contents, spine documents become chapters. Their title/first heading
replaces the numeric fallback when a document is opened; unopened fallbacks remain
numeric to avoid reading every chapter at startup.

ZIP stored and deflated entries are streamed. Opening reads container/manifest,
spine and contents, then imports only the current XHTML document. Screens are
laid out on demand; one bounded parser block can produce several screens ahead.
EPUB has no total screen count, page-number display or Go to page. Go to chapter,
Previous/Next, dictionary lookup and reader-only rotation remain available.
Markdown keeps its full pagination and Go to page. Both formats justify wrapped
prose by default; final lines, headings/code and table alignment do not stretch.

Converted documents, screen runs and pointer-free layout continuations survive
Close and reboot on SD. Reopening resumes the last document/screen without
reconversion or replaying already generated layout. Cache identity includes source
path/size/mtime, font identity/file size/mtime, orientation and layout version.
Caches live below the owned .inkpy-reader directory; removing that directory on SD
resets them. There is no automatic eviction yet. These are derived files only.
A damaged/missing continuation rebuilds that document's layout.

The current XHTML document is still fully decompressed and converted before its
first screen. Very large single-document EPUBs can therefore still wait. Returning
to the end of an unvisited preceding document also requires laying it out. These
are explicit next optimization candidates if the device log identifies them as
bottlenecks; the whole book is not imported or paginated at opening.

Supported content includes paragraphs, headings, emphasis, code, basic lists,
tables, existing Markdown math notation and raster images. Table row one becomes
the header. Links retain their text. PNG and baseline JPEG images fit the available
screen or cell, preserve aspect ratio and are never enlarged. Small images stay
inline; larger images occupy a block. SVG wrappers referencing raster images work;
SVG drawing and MathML do not. Missing/unsupported images retain a text placeholder.
Markdown images now use the same decoder, with paths relative to the document.

PNG supports noninterlaced gray/indexed 1/2/4/8-bit and 8-bit RGB/RGBA/gray-alpha.
Palette/alpha transparency blends onto white. JPEG uses ChaN TJpgDec. Images are
dithered into the same packed monochrome bitmap spool as formulas. One reusable
48 KiB bitmap is shared with math; PNG uses two fixed 32,769-byte row buffers and
the existing inflater, JPEG an 8 KiB work buffer. Source image size does not cause
full-image RAM allocation. Progress callbacks keep cancellation/input polling live.

Deliberate limits: no DRM/encrypted ZIP, ZIP64, fixed-layout positioning, audio,
publisher styling, standalone SVG rendering, progressive JPEG, interlaced/16-bit
PNG or merged table cells. Existing Markdown parser/table bounds still apply.
This is a restricted EPUB reader, not a complete browser or EPUB conformance engine.

## Checks

The generated corpus covers EPUB 2/3 with stored/deflated ZIP, fragment chapters,
ignored H2/style, PNG/JPEG/SVG-wrapper/inline images, missing-image fallback, math,
tables, portrait and landscape. Existing Markdown, table, dictionary and browser
checks pass. Firmware builds for ESP32-S3. Real books and device RAM/stack/refresh
behavior still need physical testing.

Reproduce fixtures with prototypes/reader/make-epub-fixtures.py and use the
reader-epub host target. Results: results/stage37/checks.txt.

## Loading and whitespace

Uncached work shows only a static hourglass. Long Home cancels and returns home.
Temporary EPUB logging, phase strings and image-signature probes were removed in
Stage 39. Existing inkpy-epub-debug.txt files are no longer appended to.

XHTML whitespace collapses outside preformatted code; indentation between image
wrappers or table rows cannot become Markdown code or interrupt a pipe table.
Preformatted code preserves spacing; chapter anchors do not inject table-breaking
blank lines. Body element IDs are valid chapter targets. JPEG recognition is by
signature, so .jpeg and .jpg behave identically.

Stage 39 uses a new cache namespace so old malformed conversions are not reused.
The first opening after upgrading rebuilds the requested document; subsequent
opens reuse it normally. Older derived cache folders can be deleted if desired.
ZIP uses fixed 8 KiB heap scratch rather than task-stack buffers. The hourglass
uses primitives only, avoiding a large font-rasterization stack during extraction.
