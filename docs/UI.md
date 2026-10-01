# InkPy interaction specification
Updated 2026-10-01. User-confirmed requirements unless explicitly marked proposed or open.

## General
No swipe, slide, drag-to-scroll or touchpad navigation gestures anywhere. Side buttons page through content. One fixed UI with hardcoded text and one keyboard layout. The power menu below is the complete requested settings surface; night mode and orientation are now explicitly in scope.
Proposed: sliders are tap-to-set tracks with + and - buttons, not draggable controls. Press durations, debounce and double-click timing are fixed implementation constants, not user settings. A long press must suppress its short-press action; a double power press must suppress the single-press menu.

## Home: file browser
- Filename list, no thumbnails.
- Two buttons at the top: New file and Python console.
- New file accepts a freely entered filename and extension: .py, .md, .txt, another extension, or no extension. It creates a text file; do not append or force .txt.
- Tap a folder to enter it. Side buttons change list pages.
- Home goes to the parent folder; at the root it does nothing.
- Tap a .md file to render Markdown. Tap another text file to open plain text. A binary file produces an error.
- Long-press a file: exactly Edit and Execute.
- Edit opens any valid text file in the editor regardless of extension; reject binary files.
- Execute runs a Python script; otherwise show exactly "not executable".
- No implicit execution when tapping .py.

Proposed defaults: creating a file opens its editor; reject invalid filesystem names and existing names without overwriting. File tap uses a read-only plain-text view, with side paging and Home returning to the browser. Extension comparison is case-insensitive. Execute initially accepts .py and lets MicroPython report syntax/runtime errors; whether extensionless Python scripts should also be executable remains open. Do not infer that every text file is Python.
Text validation must inspect content, not only the extension. Proposed encoding is UTF-8 (optional UTF-8 BOM) with common newlines; invalid/unsupported encoding is not definitive proof of binary data. Validate incrementally with bounded memory and preserve bytes/newline style during editing. A prefix-only binary check is provisional, not proof that the whole file is valid.

## Text editor
- Tapping text moves the cursor.
- Side buttons move between pages; no slide gestures.
- Home shows Discard / Save / Cancel. Pressing Home again cancels the prompt.
- There is no other Save button.
- Long-press Home closes and discards.
Proposed: Save writes successfully then closes; on failure remain in the editor with changes intact. Discard closes without replacing the original. Cancel resumes editing. Prompt text and layout must make the destructive long-press rule predictable.
A single onscreen keyboard is shared with filename entry and the Python console. Its exact layout is not chosen yet.

## Python console
- Side buttons page through the current session's history; no slides.
- Home shows Stop process / Close console / Cancel.
- Long-press Home stops its process, closes the console and goes home.
- The top-level browser button opens an interactive MicroPython console.
Proposed: Execute opens this same console with the selected script; output stays visible after completion. Stop interrupts execution and keeps the console open.
Open: does ordinary Close console stop the running script, or leave it running in the background? Do not silently decide this during implementation. Long-press Home always stops and closes.
Console history needs SD-backed paging or an explicit retention policy, not unlimited RAM accumulation. Output rate must not drive a display refresh for every print.

## Markdown reader
- Side buttons turn pages; no slides.
- Tap a word to display its StarDict translation/definition.
- The translation popup includes Change dictionary, opening a dictionary chooser.
- This is the only place to change dictionary; do not add it to the power menu or another settings page.
- Home menu: Go to page / Select chapter / Close book.
Proposed: chapters come from Markdown headings; page numbers belong to the current font/orientation layout. Keep reading position by source offset when reflowing. If no dictionary is selected or found, still show the lookup popup and Change dictionary control. Modal Home/back behaviour should dismiss the topmost popup first.

## Sleep and power
- Auto-sleep after five minutes of inactivity, except while a Python script is running.
- Sleep preserves whatever is currently visible. No sleep screen, clearing, clock overlay, or page replacement.
- Long-press Power while awake sleeps; long-press Power while asleep wakes. From the user's perspective, waking re-enables touch and restores interaction.
- Double-press Power toggles the light on/off.
- Short-press Power opens exactly:
  - Light brightness: slider, +, -
  - Light warmth: slider, +, -
  - Light on/off
  - Night mode
  - Orientation: portrait/landscape
  - Time settings
  - Font selector

Proposed: disable touch and side-button interaction while asleep; ignore short/double Power actions until an intentional long wake. Turn the frontlight off for sleep and restore its prior state on wake; this changes illumination, not screen content. Night mode means inverted rendering within the fixed theme. Time is set locally, without a network time service.
Open: what should manual sleep do while Python is running (continue running with input/light off, suspend it, or stop it)? Auto-sleep exemption is confirmed; manual policy is not.
Implementation must distinguish idle power saving, state-preserving sleep and deep sleep. An unchanged e-ink image does not imply that the processor is already sleeping.
