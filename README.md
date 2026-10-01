# InkPy

Minimal firmware exclusively for the Xteink X4 Pro. Stage 2 provides a compiled
hardware diagnostic; Stage 3 adds an isolated host math/font prototype. Device
validation is pending. The file browser, full Markdown reader, editor and
MicroPython runtime are not implemented yet.

## Confirmed scope
- Markdown reader with embedded LaTeX mathematics: inline `$…$` and display `$$…$$`, including multiline display blocks. Target the mathematical notation used in ChatGPT Markdown responses, not full LaTeX documents or packages.
- Simple text editor designed for bounded RAM, including documents larger than available RAM.
- MicroPython: run .py files and use an interactive onscreen Python console.
- Closing the console stops its script; manual sleep suspends it until wake.
  No script runtime limit. Sub-minute scripts are the expected workload, not a timeout.
- Fonts and StarDict tap-word lookup.
- Files supplied through microSD. Wi-Fi may be used by Python.
- One fixed theme with night inversion, one UI language with hardcoded text, one keyboard layout, and the small power menu defined below.
- EPUB later, only after Markdown works well, using the same supported rendering capabilities.

No cloud, synchronization, KOReader sync, USB file-transfer mode, file-transfer protocol, multiple themes, localization system, or additional device targets. These are deliberate scope boundaries.

## Proposed technical direction
A fresh, C-first application on ESP-IDF, reusing selected proven hardware drivers and libraries. Small C++ dependencies may be retained when translation would add work or risk. The component audit selects an IDF baseline and candidate components; their integration still needs prototypes.

Keep layout separate from document parsing so a later restricted EPUB importer can target the same renderer. Use bounded caches and SD-backed data structures; avoid loading entire documents into memory. MicroPython is for user scripts, while core firmware operations remain native.

“ChatGPT-style math” describes the intended use, not a fixed compatibility specification. The proposed acceptance corpus defines representative notation; actual supported coverage must be established by the rendering prototype. Delimiters alone do not make mathematical layout trivial.

## Interaction and implementation references
- [Math/font prototype](docs/MATH-PROTOTYPE.md): measured host results, preview and remaining gates.
- [Hardware diagnostic](docs/BRINGUP.md): build instructions, diagnostic controls and device checklist.
- [Port provenance](docs/PORTING.md): reused code, deliberate reductions and licences.
- [UI specification](docs/UI.md): file browser home, file actions, editor/console/reader controls, no slide gestures, power menu and sleep.
- [Component audit](docs/COMPONENT-AUDIT.md): pinned sources, hardware facts, reuse decisions, dependencies and feasibility gates.
- [Markdown/math corpus](fixtures/markdown-math.md): proposed rendering coverage and failure cases, not a claim of implemented support.

## Working agreement
Work in bounded stages; save research, decisions, and progress in GitHub, then pause for continuation. See [AGENTS.md](AGENTS.md) for execution rules and [the current checkpoint](docs/CHECKPOINT.md) for the next task.
