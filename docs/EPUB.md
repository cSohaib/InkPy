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
Markdown keeps its full pagination and Go to page.

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

## Temporary Stage 38 diagnostics

SD-root inkpy-epub-debug.txt records book paths, TOC targets/fallbacks, ZIP methods
and entry sizes, chapter conversion, cache hits, layout progress, image signatures
and decoder failures. Device waiting updates also record heap/largest block/main
stack headroom. The log appends up to roughly 512 KiB; delete it before a fresh
reproduction if it reaches that limit. No source books are changed.

An hourglass with the current processing phase is shown during uncached work.
No invented percentage: the whole book's page count is intentionally unknown.
Long Home cancels loading and returns home. Temporary logging/phase text should
be removed after the four reported books have been diagnosed.

For a device report: open each problematic book, try its contents and image pages,
close/reopen it, then copy inkpy-epub-debug.txt from SD. Include the book name,
what failed and, when shareable, the EPUB or a small excerpt reproducing it.
