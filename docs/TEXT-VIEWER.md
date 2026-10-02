# Stage 8: plain text viewer

The optional browser app now opens non-Markdown text files, including .py,
extensionless files and .txt. Side buttons turn pages; Home returns to the same
browser folder/page. Tapping the text has no action. Markdown retains its separate
selection route until the richer reader is connected.

components/ink_browser/ink_text.c reads one screen into a fixed 22-by-24 character
grid. It wraps at the screen edge, preserves blank lines, handles CR/LF/CRLF and
UTF-8 BOM, and uses four-column tabs. The interim ASCII font shows '?' for valid
non-ASCII codepoints. Invalid UTF-8 and binary controls show an error when reached.
Word wrapping, Unicode glyphs and proportional fonts remain later improvements.

No whole-document buffer or growing in-memory page index. Next seeks to the saved
byte offset; Previous rescans from the beginning and can be slow for large files.
Files are opened read-only and closed after each page, including before sleep.
There is no application file-size limit; seek limits depend on the target libc and
filesystem. Files are assumed unchanged while viewing for this first version.

Build via scripts/build-browser.sh with the pinned SDK as before. Basic host checks:

```
bash prototypes/browser/run.sh
bash prototypes/browser/run-text.sh
```

The browser checks still passed. The viewer check passed opening the sample,
forward/backward page equality, first/last-page bounds, ignoring taps and Home back.
Own code compiled with warnings-as-errors. The actual drawing preview in
results/stage8/text.png was inspected. No SDK is available here, so ESP32 build and
physical-device validation remain pending. No new editor or Python implementation,
math rendering changes, exhaustive tests or performance optimization this stage.
