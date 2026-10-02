# Stage 9: keyboard and New file

Tap New file in the browser, type a name including any desired extension, and
tap Create or Enter. The app creates an empty file in the current folder.
No extension is forced. Existing files are never opened for truncation: creation
uses O_CREAT|O_EXCL. An error stays in the naming screen so the name can be fixed.
Success shows File created; Home/tap returns to the refreshed folder listing.
Cancel or Home while naming discards the name without creating anything.

The fixed English keyboard provides letters, numbers, printable ASCII symbols,
Space, Delete and Enter. Shift toggles uppercase; the symbol button switches the
key layer. No slide gestures. ink_keyboard.c emits characters/actions without
owning an input buffer, so the same keyboard can serve the editor/Python console.

Filename input is capped at 255 ASCII bytes and the full browser path at 511.
SD-invalid characters, directory separators, dot/dot-dot and trailing spaces/dots
are rejected. Dot-prefixed filenames are now visible in the browser. The new file
is empty; editing contents is still a separate feature. Errors on closing a newly
created file may leave that empty file present; no user file is deleted on failure.

Run `bash prototypes/browser/run-new-file.sh` for the basic same-core host check.
It passed keyboard taps, Shift/Delete, exact .py/.md/.txt names, extensionless
creation, preserving existing contents, invalid-name correction and cancel.
Existing browser/text-viewer checks also passed with warnings-as-errors. The real
keyboard drawing in results/stage9/new-file.png was visually inspected.

Build the app through scripts/build-browser.sh as before. ESP-IDF is still absent
in this workspace: no ESP32 build or physical-device validation this stage.
The rendering issue noted earlier remains deferred. MicroPython comes next.
