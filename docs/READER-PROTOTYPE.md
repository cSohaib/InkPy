# Stage 5: bounded Markdown pagination

Host prototype only. This stage adds the C layout core in components/ink_layout,
not a firmware reader or product menu. Existing math rendering is unchanged.

## Run

From the repository root, run `bash prototypes/reader/run.sh`. Requires a C11
compiler, Git, Python 3, and network access on the first dependency fetch.
It pins MD4C to c7ba975c34d714966ea910c58a97524e5d674ff7 from
https://github.com/mity/md4c (MIT; upstream LICENSE.md remains in the fetched tree).
No upstream application or framework is imported. Build dependencies stay ignored.

Then run:

```
prototypes/reader/build/reader-probe fixtures/reader-layout.md /tmp/inkpy-pages
python3 prototypes/reader/preview.py /tmp/inkpy-pages 1 /tmp/page.png
```

The output directory must not already exist. Preview requires Pillow and system
DejaVu Sans Mono fonts; these host-only resources are not firmware font choices.
The optional probe arguments are width, height and read-buffer size, used for
verification, not proposed user settings.

## What is implemented

- Streaming UTF-8 validation, BOM and CR/LF/CRLF handling. Invalid encodings,
  NUL and disallowed controls reject the file; this is a text heuristic, not a
  universal binary-file detector. Input is never edited.
- MD4C parses bounded units; text, emphasis, code, headings and list markers
  produce UTF-8 draw runs. Word wrapping and pagination use a provisional
  fixed advance per codepoint. Tabs currently become a single space.
- ATX and Setext H2 headings create chapter records, including duplicate titles.
  Fenced-code headings do not become chapters in the tested ordinary fences.
- Page/source anchors and chapter/page records are disk-backed. cache.py seeks
  directly to a requested page/chapter and binary-searches source anchors to
  restore an approximate position after reflow. It loads only one page at a time.
- Cache output uses temporary files and a final completion manifest. Failures
  leave an unpublished directory, which must be removed before retrying. A source
  stat check detects ordinary changes during generation; this is not a snapshot
  or protection against concurrent changes preserving file metadata.

## Bounds and format

Layout context is 19,976 bytes on this host, with an 8,192-byte Markdown buffer,
8,192-byte physical-line buffer, 1,024-byte I/O buffer and 128 line cells.
MD4C requested live allocations, including wrapper headers, are capped at
131,072 bytes. This excludes libc allocator overhead, realloc transient storage,
stack, FILE buffers and the separate math engine. The allocator has one owner;
concurrent parses are unsupported. No whole-document RAM allocation is made.

Oversized units become literal text until the next scanner boundary. This keeps
content readable without truncating long lines, but loses Markdown interpretation
and chapter detection for those units. Excess parser complexity fails cleanly.
Chapter titles are UTF-8 excerpts up to 191 bytes, with a truncation flag; their
page/source locations are retained. Cache size and processing time grow with
source size; the file is scanned fully before the complete cache is published.

All integers are little-endian. Version 1 cache files:

| File | Record |
| --- | --- |
| pages.bin | 32 bytes: u64 draw begin/end/source, u32 chapter/reserved |
| chapters.bin | 212 bytes: u64 source, u32 page/id, u16 title length/truncated, 192 title bytes |
| draw.bin | 20-byte header: u16 x/y/cell/style, u32 UTF-8 byte count, u64 source; then text |
| manifest.json | Geometry, completion marker, counts and diagnostic measurements |

Page and chapter IDs start at 1; chapter 0 means no preceding H2. A page's chapter
is the chapter active at its first run; an H2 later on that page has its own chapter
index entry. Source anchors are byte offsets, approximate for synthetic markers
and normalized text; they are not per-word dictionary hitboxes. A future cache
needs source/font/layout identity and invalidation before reuse across sessions.

## Verification

229 acceptance checks passed with warnings-as-errors. See results/stage5/test-report.json.
They cover 1/2/7/1024-byte reads, deterministic cache output, UTF-8/entities,
invalid late input without published cache, newline variants, H2/Setext/fenced
code, title truncation, oversized units, random page access, source lookup,
portrait/landscape chapter anchors, empty files and refusing existing output.

| Source bytes | Pages | Context bytes | Peak parser bytes | Reported host RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| 8,387,708 | 10,804 | 19,976 | 4,032 | 11,392 |
| 33,553,420 | 43,217 | 19,976 | 4,032 | 11,392 |

RSS is the child process ru_maxrss and can include an inherited pre-exec high-water
mark; use it only as a coarse regression indicator, not a firmware RAM estimate.
The explicit context/allocation counters are the useful bounds here. No ESP32
build, stack measurement, SD throughput or physical-device acceptance was done.
The two committed page previews were visually inspected for wrapping and clipping.

## Deliberate prototype gaps

This scanner is not a complete streaming CommonMark parser. Blank boundaries
split parse units: cross-block references, loose/nested list continuity, nested
container fences and other context spanning units need further work or an explicit
supported subset. Full CommonMark conformance is not claimed. HTML is literal,
tables are linearized, images show alt-text placeholders, and math is tagged source
text. Only a small entity set plus numeric entities is decoded. No links, image
loading, math bitmap composition, shaping, grapheme-aware wrapping, proportional
font metrics, heading keep-with-next, persistent-cache validation or firmware UI.
UTF-8 preservation does not imply glyph coverage or correct complex-script display.
These results establish a bounded pagination foundation, not Markdown acceptance.
