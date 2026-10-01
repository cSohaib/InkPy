# InkPy

Minimal firmware exclusively for the Xteink X4 Pro. Currently in planning: no runnable firmware yet.

## Confirmed scope
- Markdown reader with embedded LaTeX mathematics: inline `$…$` and display `$$…$$`, including multiline display blocks. Target the mathematical notation used in ChatGPT Markdown responses, not full LaTeX documents or packages.
- Simple text editor designed for bounded RAM, including documents larger than available RAM.
- MicroPython: run .py files and use an interactive onscreen Python console.
- Fonts and StarDict tap-word lookup.
- Files supplied through microSD. Wi-Fi may be used by Python.
- One fixed theme, one UI language with hardcoded text, one keyboard layout, and almost no settings.
- EPUB later, only after Markdown works well, using the same supported rendering capabilities.

No cloud, synchronization, KOReader sync, USB file-transfer mode, file-transfer protocol, multiple themes, localization system, or additional device targets. These are deliberate scope boundaries.

## Proposed technical direction
A fresh, C-first application on ESP-IDF, reusing selected proven hardware drivers and libraries. Small C++ dependencies may be retained when translation would add work or risk. Language and dependency choices remain provisional until the component audit.

Keep layout separate from document parsing so a later restricted EPUB importer can target the same renderer. Use bounded caches and SD-backed data structures; avoid loading entire documents into memory. MicroPython is for user scripts, while core firmware operations remain native.

“ChatGPT-style math” describes the intended use, not a fixed compatibility specification. The first audit must establish representative examples and an explicit supported notation list. Delimiters alone do not make mathematical layout trivial.

## Working agreement
Work in bounded stages; save research, decisions, and progress in GitHub, then pause for continuation. See [AGENTS.md](AGENTS.md) for execution rules and [the current checkpoint](docs/CHECKPOINT.md) for the next task.
