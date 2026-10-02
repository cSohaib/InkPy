# Stage 6: text and math together

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
