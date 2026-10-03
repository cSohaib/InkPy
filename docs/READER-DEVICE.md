# Stage 23 device reader

Build with `bash scripts/build-browser.sh` in the pinned IDF environment. The
product now links Markdown, formula rendering and MicroPython together. Existing
upstream pins and notices are unchanged. The downloadable image remains Stage 20.

To prepare math resources, run:
```
python3 scripts/prepare-math-sd.py prototypes/math/.deps/MicroTeX NEW_STAGING_DIR
```
Copy that directory's `inkpy` folder to microSD manually. Math reads
`/sd/inkpy/math/fonts/...`; missing fonts or unsupported formulas leave source
visible. No background transfer, cloud, USB protocol or preprocessing is required.

Tap `.md` to index/render. Source is read-only. Indexes and temporary math bitmaps
use `/sd/.inkpy-reader`, hidden from the file browser and guarded by an ownership
marker. Close removes disposable data; later opens reindex. A failed/incomplete
index is never opened as a successful reader session. SD space grows with indexed
content; RAM remains bounded by the existing layout/parser budgets and one-page
streaming renderer. Cache offsets are limited to LONG_MAX on the target.

H2 starts a new page. Home displays current page/chapter and Go to chapter / Go to
page / Close. Side buttons page the document or chapter list. Numeric page entry
rejects out-of-range input. Home from a navigation prompt returns to the page.
Power remains available over reader/prompts; manual Refresh redraws the reader.

Text currently uses the fixed ASCII bitmap font, scaled to layout metrics, with
basic bold/italic and a placeholder for non-ASCII characters. Math uses font
rasterization and layout from the existing backend. Images remain layout markers,
not decoded image assets. Stage 24 adds [StarDict tapping and selection](STARDICT.md);
selectable text fonts are still pending.
Indexing is synchronous: capture runs and queues events while indexing yields,
but UI dispatch waits for completion. Reusable caches, indexing progress/cancel,
partial refresh and typography refinement are deferred.

`prototypes/reader/device.c` exercises the same renderer and navigation on the host;
its page/menu previews are in results/stage23. Native compilation, image checks
and desktop results do not establish device runtime, available RAM/stack, physical
input speed or sleep/panel correctness. No device was connected or flashed.
