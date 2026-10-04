# EPUB and images

Tap a DRM-free .epub in the browser. EPUB 2 NCX and EPUB 3 navigation contents
supply chapter titles and fragment targets; headings do not create chapters.
Without usable contents, spine documents become chapters. Screen pages are
calculated by InkPy; Go to page and reader-only rotation work as for Markdown.

ZIP stored and deflated entries are streamed. Container, manifest and spine
select the reading order. XHTML becomes a temporary Markdown stream plus chapter
anchors on SD, then uses the existing layout, math, tables, word lookup and page
cache. Publisher CSS, embedded fonts and scripts are ignored. Scratch files are
removed after indexing, on failure and on close. Opening a book currently imports
and paginates it again; there is no persistent preprocessed-book cache.

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
