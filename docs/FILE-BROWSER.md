# Stage 7: minimal device file browser

Stage 8 connects text-file selection to a paginated viewer; see TEXT-VIEWER.md.
The routing-only descriptions below record the Stage 7 boundary.

Optional firmware app: `bash scripts/build-browser.sh` after sourcing the pinned
ESP-IDF environment from BRINGUP.md. This selects INKPY_BROWSER and outputs to
build-browser/. The original hardware and math diagnostic builds remain available.
No flashing was performed or device installation route assumed.

The app mounts /sd without running the diagnostic write probe. It lists filenames
and folders, with a trailing slash for folders, fourteen rows per portrait page.
Side buttons move between pages without wrapping. Tap enters a folder or selects
a file. Home returns to the parent, does nothing at root, or dismisses a selection
or notice. Slides and touch holds have no browser action in this stage.

The C core retains one screen of directory entries and rescans for each page;
order is the filesystem's enumeration order. Dot-prefixed entries are hidden.
Paths are capped at 511 bytes. Visible names are clipped; the complete stored name
is used for opening. A small built-in ASCII bitmap font preserves case; non-ASCII
characters show '?'. Its renamed raster subset and licence are in components/ink_browser.
Unicode font support and sorting can be added later.

File selection routes .md (case-insensitive) to Markdown and other regular files
to text. A bounded first-4-KiB control/NUL heuristic shows a binary/read-error notice.
This is preliminary detection; invalid UTF-8 or binary data later in the file is
not yet checked by the browser. The selection screen records the destination,
but does not display file contents yet. No editor or interpreter has been added.
New file and Console buttons currently show notices; Power single likewise has
an interim notice. File Edit/Execute menus and the full power menu come later.
Double Power toggles light; long Power/five minutes idle sleeps; short Power wakes
without changing the screen, through the existing board layer.

## Basic check

`bash prototypes/browser/run.sh` compiles the same browser and drawing code with
warnings-as-errors and uses a temporary folder of sample files. Paging across
20 entries, folder entry/parent/root, Markdown/text selection and binary notice
passed. It never modifies user files. The generated portrait preview under
results/stage7/browser.png was visually inspected.

The device app and CMake wiring were added, but no ESP-IDF SDK exists in this
workspace: no ESP32 compile, flashing or physical-device validation this stage.
Full panel refresh still blocks input. The original hardware gate remains pending.
This is the browser portion of the exploratory firmware, with reader connections
and remaining controls deliberately left for subsequent small stages.
