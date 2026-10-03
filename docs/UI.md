# InkPy interaction specification
Updated 2026-10-03. User-confirmed requirements unless explicitly marked proposed or open.

## General
No swipe, slide, drag-to-scroll or touchpad navigation gestures anywhere. Side buttons page through content. One fixed UI with hardcoded text and one keyboard layout. The power menu below is the complete requested settings surface; night mode and orientation are now explicitly in scope.
Brightness and warmth use only + and - buttons; no sliders. Press durations, debounce and double-click timing are fixed implementation constants, not user settings. A long press must suppress its short-press action; a double power press must suppress the single-press menu.
No screen displays an InkPy title/header. Power menu is global: accessible from
the browser, editor, reader, console and their prompts, without losing their state.
Routine display updates should use a fast/partial refresh; full refresh is explicitly
available through Refresh screen in the Power menu. Manual cleanup is the preferred
default. Automatic full refresh every 20 updates is an optional alternative, not
a confirmed requirement. Hardware initialization/recovery may require a full refresh.
Touch/button capture must continue during display refresh. Queue discrete key/tap
events and consume them in order; do not wait for rendering before listening for
the next tap. Coalesce drawing, not input. Fast repeated keyboard taps must remain
separate characters. Stage 22 implements independent input capture and a 256-event
FIFO while rendering remains blocking; physical fast-typing verification is pending.

## Home: file browser
- Filename list, no thumbnails.
- Hide dot-prefixed files and folders; do not delete or modify them.
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
- Normal editing displays neither filename nor "Home: save or discard" footer.
  Keyboard sits at the bottom. Filename may remain in Save/Discard/Cancel prompt.
Proposed: Save writes successfully then closes; on failure remain in the editor with changes intact. Discard closes without replacing the original. Cancel resumes editing. Prompt text and layout must make the destructive long-press rule predictable.
A single onscreen keyboard is shared with filename entry and the Python console. Its exact layout is not chosen yet.

## Python console
- Side buttons page through the current session's history; no slides.
- Home shows Stop process / Close console / Cancel.
- Long-press Home stops its process, closes the console and goes home.
- The top-level browser button opens an interactive MicroPython console.
Proposed: Execute opens this same console with the selected script; output stays visible after completion. Stop interrupts execution and keeps the console open.
Closing the console stops whatever is executing and releases the session; no background script continues. Long-press Home does the same and goes home.
Scripts have no fixed runtime limit. The expected workload is short scripts (usually under one minute), which guides design and testing but must never become an execution timeout.
Console history needs SD-backed paging or an explicit retention policy, not unlimited RAM accumulation. Output rate must not drive a display refresh for every print.

## Markdown reader
- Side buttons turn pages; no slides.
- Tap a word to display its StarDict translation/definition.
- The translation popup includes Change dictionary, opening a dictionary chooser.
- This is the only place to change dictionary; do not add it to the power menu or another settings page.
- Home menu: Go to chapter / Go to page / Close.
- This menu also shows the current page number and current chapter.
- Markdown chapters are level-two headings (`##`, equivalent to `<h2>`), not all heading levels. H1 and H3–H6 do not create chapter entries. Use parsed heading levels, including Setext H2; headings inside code are not chapters.
Proposed: page numbers are one-based and belong to the current font/orientation layout. Before the first H2, or in documents without H2, show "No chapter". The current chapter is the last H2 at or before the page's reading-position source offset; retain duplicate titles as separate entries. Keep reading position by source offset when reflowing. If no dictionary is selected or found, still show the lookup popup and Change dictionary control. Modal Home/back behaviour should dismiss the topmost popup first.

## Sleep and power
- Auto-sleep after five minutes of inactivity, except while a Python script is running.
- Sleep preserves whatever is currently visible. No sleep screen, clearing, clock overlay, or page replacement.
- Long-press Power while awake sleeps; a single short Power press while asleep wakes (subject to hardware verification). From the user's perspective, waking re-enables touch and restores interaction.
- Double-press Power toggles the light on/off.
- Short-press Power opens exactly:
  - Light brightness: + and - buttons only
  - Light warmth: + and - buttons only
  - Light on/off
  - Night mode
  - Orientation: portrait/landscape
  - Time settings
  - Font selector
  - Refresh screen: close the menu and force a full refresh of the underlying screen to clear e-ink ghosting, keeping its page/cursor/session unchanged
- Only while the power menu is open, its header shows time, date and battery level. Do not show these in the reader, browser, editor, console or any other screen. No persistent status bar.

Proposed: disable touch and side-button interaction while asleep; consume the waking Power press so it does not also open the power menu, toggle light or put the device back to sleep. Turn the frontlight off for sleep and restore its prior state on wake; this changes illumination, not screen content. Night mode means inverted rendering within the fixed theme. Time is set locally, without a network time service.
Sleep suspends Python execution and waking resumes it; it must not kill or restart the script. The existing automatic-sleep exception while a script runs remains in effect. Manual Power sleep can suspend a running script.
Implementation note: preserve the VM, stack and local variables, but wall-clock time still passes. Network connections and external I/O may time out across a long sleep; suspension does not freeze the outside world.
Implementation must distinguish idle power saving, state-preserving sleep and deep sleep. An unchanged e-ink image does not imply that the processor is already sleeping.

## Stage 23 source status
Device `.md` opening now connects the shared reader/layout/math path. H2 headings
start new pages; Home shows current page/chapter and the requested navigation.
Chapter list side-paging and numeric page entry are implemented. Text uses the
fixed bitmap font for now; StarDict tap lookup and selectable fonts are pending.
First-open indexing captures input but dispatches it after indexing completes.
Physical acceptance remains pending; full refresh is still the current driver.

## Stage 24 source status
StarDict tap lookup, paged definitions and dictionary chooser are connected.
The chooser is only opened through Change dictionary in the lookup popup;
selection is retained on SD, retries the word and preserves the reader page.
Home dismisses chooser to definition, then definition to book. No dictionaries,
missing words and invalid data still leave Change dictionary available.
Preparation is synchronous/yielding with capture active; typography and physical
accuracy remain subject to later font integration/device testing.
