# Stage 6: text and math together

Current Stage 40 behavior: EPUB and Markdown automatically justify wrapped prose;
paragraph-final and explicit-break lines stay naturally aligned. Headings, code,
display formulas and table alignment do not stretch. Fixed monospaced text metrics
remain; no alignment setting was added.

The requested Greek, comparison, arithmetic, function, accent, set/logic, arrow,
dot/product and common-variant commands now pass native preflight. matrix, bmatrix
and vmatrix join pmatrix/aligned/cases. Every requested command is exercised in
fixtures/markdown-math-expanded.md (64 formulas, zero fallback in both orientations).
Existing math size/complexity budgets and unsupported-source fallback remain.
No new SD fonts or dependencies. Check: prototypes/reader/test-justify.py.

The host reader now composes native math bitmaps alongside Markdown text.
Inline math is an indivisible line item with baseline and size; display math gets
its own centered line. This connects the existing MicroTeX renderer to the C
pagination core. No off-device preprocessing is required for the prototype.

```
bash prototypes/reader/run-math.sh fixtures/reader-mixed.md /tmp/inkpy-mixed
python3 prototypes/reader/preview.py /tmp/inkpy-mixed 1 /tmp/page.png
```

Requires CMake, C/C++ compilers, Git and the existing preview dependencies.
CMAKE can override the CMake executable. The script fetches the same pinned
sources as Stage 4; no new rendering dependency. Output must be a new directory.
Optional width/height/read-size arguments remain available for host experiments.
The core remains C; the existing math backend is a C++ dependency behind a C API.

Cache version 2 adds bitmap runs. Their existing run header uses style 32768 and
cell=image width; payload is u16 height followed by ceil(width/8)-byte rows,
most-significant bit first, black=1. Text/page/chapter records retain their format.
cache.py reads versions 1 and 2; preview.py composes either. A temporary disk spool
holds formula images waiting for their line to be laid out; it closes after parsing.

The layout context is currently 75,176 bytes on this host, including one fixed
48 KB math bitmap, a 4 KB formula collector and a bounded line of text/image items.
The separate math engine has its existing dynamic allocations. This is a host
result, not a firmware memory measurement. Old Stage 5 counters describe that
stage's smaller text-only context. Text spacing still uses provisional monospaced
metrics; proportional metrics, typography and detailed baseline polish are deferred.

The small fixture produced two portrait pages, six rendered formulas and one
intentional unsupported-command fallback. Landscape generated successfully with
the same formula/chapter counts. The larger Markdown/math corpus produced 26
rendered formulas and three expected fallbacks. All generated pages were read
through the cache reader; the two small portrait previews were visually inspected.
Own code built with warnings-as-errors. No new exhaustive suite, ESP32 build or
physical-device validation was performed.

Unsupported or oversized math is displayed as source. Formula collection is
bounded to 4,095 bytes; an overflow shows its prefix and an ellipsis. Math remains
atomic when wrapping and cannot exceed the current page's usable dimensions.
Markdown's existing cross-block parsing and literal-fallback limitations remain.
This is a working mixed-page prototype; reader menus, tap-word lookup, font UI,
images from files and the full device reader are still to come.

## Stage 36: math commands and tables

Supported commands include all of the requested bar, beta, epsilon, frac, left,
min, mu, pm, quad, right, sigma, sqrt, sum, text and times. Existing SD math fonts
are sufficient. Unsupported/oversized expressions still display source.

Markdown pipe tables render with equal-width columns, bold headers, colon-based
alignment, wrapped cells, borders and inline math/styles. Landscape reflows them.
A row may continue across pages; headers are not repeated. Up to 16 columns that
fit at least one character per cell use the grid; wider tables use linear cells.
The existing 8 KiB parser-block limit still gives literal fallback for oversized
blocks. No HTML tables, merged cells or table-specific configuration.

A reusable row scratch file on SD avoids retaining whole rows in RAM. Device and
host cache readers accept INK_RULE style 64: width in cell, payload u16 height,
solid black rectangle. Rules are skipped by dictionary word lookup. Reader
scratch is closed/removed after indexing. Layout context is 75,560 host bytes,
384 bytes more than the previous core. Tables do not add new dependencies.

Fixture: fixtures/markdown-tables-math.md. Host test: build reader-tables through
prototypes/reader/CMakeLists.txt, then run with fixture/output folder/math resources.
# Arabic reader text (Stage 41)

EPUB and Markdown share bounded Arabic shaping and mixed-direction layout. Each
displayed line chooses its base direction from the first logical letter; Arabic
is RTL, Latin/no letters LTR. Numeric prefixes do not choose direction. Formulas
remain LTR objects inside the surrounding line. Original Arabic words remain
available for StarDict lookup. Chapter/dictionary labels are shaped too.

The bundled font provides Arabic and missing-glyph fallback for selected SD
fonts, without new assets. Fixed-cell joining and basic marks are supported;
this is not full OpenType typography. Other applications and keyboard are unchanged.
Vendored FriBidi provenance/licence: components/ink_layout/vendor/fribidi/SOURCE.md.
