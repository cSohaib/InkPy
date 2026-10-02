# Host text editor, Stage 16

Browser long press -> Edit opens the host editor. Tap text to place the cursor;
type with the shared Python keyboard. Side buttons page, Home opens Save / Discard
/ Cancel, second Home cancels, long Home discards. No separate Save button.

The source is copied to a sibling temporary file with a 4096-byte physical gap.
Typing fills the gap; moving the cursor moves intervening bytes in chunks. Gap
expansion shifts the suffix once, rather than rewriting the whole document on
every keystroke. Working memory is fixed: visible text/cursor offsets plus 4 KiB
transfer buffers. Opening and saving stream the file; available disk space and
filesystem limits still apply. Previous pages rescan; faster indexing is deferred.

Full UTF-8 validation happens while making the working copy. Existing bytes are
preserved, with '?' display for non-ASCII characters in the provisional font.
Inserted text uses ASCII and LF; existing CRLF is preserved unless edited.

Save streams the virtual content (skipping the gap) to another sibling temporary
file, flushes/fsyncs and renames it over the source on the host. On success the
working file is removed; on failure the editor stays open with an error. Discard
removes only its working copy. Power-loss recovery, stale temp cleanup, SD/FAT
rename durability and device integration are deferred.

Basic checks:
```
bash prototypes/browser/run-editor.sh
make -s -C prototypes/python build/inkpy-browser-console
(cd prototypes/python && build/inkpy-browser-console)
python3 prototypes/python/preview.py
```

Host check covered an 8 MiB source, gap expansion and the browser edit/save/discard
path. These checks do not establish X4 Pro latency or memory acceptance.
