# Host math/font experiment (Stages 3–4)

Run from the repository root:

```sh
# Linux/glibc, C++17 compiler, git, CMake >=3.16, Ninja, Python3 + Pillow
bash prototypes/math/run.sh
```

The script fetches exact upstream revisions into ignored `.deps/`, checks their
identity and tracked-file cleanliness, then builds an isolated host executable.
It never builds or flashes firmware. Outputs live under `build/`:
`results.tsv`, `metrics.txt`, `results/{id}.tex`, monochrome `{id}.pbm`, and
`preview.png`. No desktop window system, Cairo, Qt, Skia or Fontconfig is needed.
Set `PYTHON=/path/to/python3` if Pillow is installed in another environment.
Pillow only assembles the contact sheet; it does not render the formulas.

`../../components/ink_math/ink_math.h` is the shared C-facing boundary. The C++ adapter implements only the
interfaces MicroTeX requires. Two context-checked build overlays fix MicroTeX operator limits and initialize
an upstream UTF conversion accumulator; vendor
source caches stay unmodified. No translation into C. Global initialization happens once, one owner renders one formula at a
time, and shutdown ends the session. Caller must provide valid pointers and a
NUL-terminated source. Not thread-safe. The optional ESP32 diagnostic uses this same component.

MD4C parses the small corpus and supplies actual inline/display math callbacks.
This is **not** a streaming Markdown reader: the harness explicitly caps its
input at 64 KiB and loads that fixture whole. Paragraph layout, links, images,
tables, pagination, reader fonts and dictionary hit boxes are not implemented.

## Deliberate reductions

- No desktop resource discovery, environment-variable lookup or `latex.cpp`.
  Resource root is explicit; initialization calls only required core services.
- TrueType, SFNT, PostScript glyph names and monochrome raster FreeType modules.
  No auto-hinter, color/SVG font renderer, external compression or shaping deps.
  Glyph hinting is disabled: native hints erased thin mathematical strokes.
  The shared source list compiles 13 required FreeType translation units.
- Four FreeType face slots, least-recently-used replacement; no glyph cache.
- One fixed 480x800, 48,000-byte caller-owned bitmap, 1=black. This is portrait
  math output, **not** the diagnostic's landscape/1=white frame format.
- 24px in the corpus run; C API accepts 12–40px. Inline uses text style and display
  uses display style, with baseline returned separately from bitmap dimensions.
- No rotation, round boxes, Unicode fallback text shaping or invented substitutes
  for missing glyphs. Unsupported drawing yields source fallback.
- Preflight allowlist is intentionally only the corpus command vocabulary;
  environments: `pmatrix`, `aligned`, `cases`. No macro definitions, resource
  loading commands or general LaTeX package language.
- Prototype budgets: 2,048 source bytes, brace depth 16, 128 commands, 64 `&`
  separators, 32 row breaks, 4 environment starts. These are provisional math
  complexity limits, unrelated to document size or Python execution duration.
- Oversized expressions return an explicit fallback; no clipping, swipe UI or
  automatic shrinking. Final reader should wrap literal source when it falls
  back. That UI policy is proposed, not user-confirmed or implemented here.

These checks limit obvious input growth; they do **not** constitute a strict
MicroTeX heap/stack allocation cap or a parser security audit. Exceptions remain
enabled, and STL/FreeType/tinyxml2 allocate dynamically. Firmware acceptance needs
measured ESP32-S3 heap/stack, allocation-failure behavior and long-run stability.

## Provenance

All pins are enforced by `../../scripts/fetch-math.sh`, called by `run.sh`; upstream files remain in the disposable cache.

| Source | Revision | Purpose |
|---|---|---|
| [MicroTeX](https://github.com/NanoMichael/MicroTeX) | `0e3707f6dafebb121d98b53c64364d16fefe481d` | Math parser/layout/metrics/fonts |
| [FreeType](https://github.com/freetype/freetype) | `42608f77f20749dd6ddc9e0536788eaad70ea4b5` (2.13.3) | TrueType monochrome glyphs |
| [tinyxml2](https://github.com/leethomason/tinyxml2) | `321ea883b7190d4e85cae5512a12e5eaa8f8731f` (10.0.0) | MicroTeX parser link dependency |
| [MD4C](https://github.com/mity/md4c) | `c7ba975c34d714966ea910c58a97524e5d674ff7` | Markdown math extraction |

Notices are retained in `../../third_party/notices/`; upstream dependency trees
also retain their original licences. MicroTeX and MD4C use MIT; tinyxml2 uses zlib.
FreeType is used under the FreeType License (FTL): portions of this software are
copyright © 2024 The FreeType Project (www.freetype.org). All rights reserved.
MicroTeX fonts have separate notices, preserved without renaming/modifying fonts.
Only `res/fonts` is copied into the test resource root. Optional `res/greek` and
`res/cyrillic` language packs are neither bundled nor needed for TeX Greek symbols.
The Stage 4 staging script retains 27 math fonts and generates their hash manifest.
See `../../docs/MATH-DEVICE.md` for the device diagnostic and measured build results.

