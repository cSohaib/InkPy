# InkPy checkpoint

Updated: 2026-10-01
Stage 1 complete: UI specification and component audit. Pause here.

## Read first
Read AGENTS.md, then this file. Read docs/UI.md when changing interactions and only the relevant sections of docs/COMPONENT-AUDIT.md for implementation. Do not repeat upstream discovery; the audit contains exact source pins and inspected paths.

## Completed
- docs/UI.md records the full user-requested controls, menus, file routing and sleep behaviour; proposals and open questions are explicitly labelled.
- docs/COMPONENT-AUDIT.md pins CrossPoint master, its FreeInk SDK gitlink, MicroPython, ESP-IDF v5.5.5, MD4C and MicroTeX. It records inspected interfaces, licensing, hardware inconsistencies, porting decisions and memory gates.
- fixtures/markdown-math.md defines proposed rendering/edge-case coverage. Its missing image is intentional; add a real small image in the rendering prototype.
- README.md links the specifications and reflects the new power menu.
- No firmware implemented, dependencies imported, builds run or hardware tests claimed.

## Main findings
- Keep a C-first native ESP-IDF application; isolate useful C++ components. SDK code needs an Arduino-to-IDF transport port.
- X4 Pro has production panel variants; one-device scope does not permit assuming a single panel controller.
- MD4C recognizes math spans but requires a contiguous input buffer. Bounded parsing remains a prototype gate, not a solved feature.
- MicroTeX needs an embedded graphics/font backend, local C++ exception support and resource trimming validated by measurement.
- Integrate the existing MicroPython ESP32 port with a controlled task lifecycle, bounded heap, shared storage and console I/O. Automatic boot/main scripts and peripheral cleanup need adaptation.
- Preserve unsaved editor state and idle REPL state. Prefer light sleep initially; deep sleep loses ordinary RAM. Unchanged e-ink pixels are separate from CPU power state.
- Brightness/warmth hardware exists. Treat sliders as tap-to-set (+/- also available), consistent with the no-slide rule.

## Unresolved user semantics (do not block board bring-up)
1. Does manual sleep while a Python script runs leave it running with the interface asleep, suspend it, or stop it?
2. Does ordinary Close console stop the script or allow background execution? Long Home definitely stops and closes.
3. Should extensionless Python files be executable? Proposed initial identification is .py.
Proposed defaults (not user-confirmed): English UI, tap-to-set sliders, save-then-close, frontlight off during sleep/restored on wake, UTF-8 text, single interpreter session. Exact keyboard layout remains open.

## Next bounded stage: minimal board bring-up
1. Create an ESP-IDF v5.5.5 build pinned to the audit commit and record build prerequisites.
2. Implement a small X4 Pro-only hardware adapter/diagnostic entry point: display, buttons/GT911 Home, SDMMC, warm/cool light and RTC; establish memory reporting.
3. Retain relevant panel detection/driver variants. Inspect only their needed source plus GT911 details at the saved SDK revision; earlier audit did not inspect every driver body.
4. Add sleep/wake and input-event probes, preserving visible pixels. Confirm actual partition/recovery conditions before offering flashing instructions.
5. Build if the execution environment permits; record exact blockers otherwise. Produce a concise device checklist (panel ID, corner taps, Home hold, button sequences, SD read/write, light channels, sleep retention/current).
6. Commit and pause for hardware validation. No full file browser, reader, editor or MicroPython app in this stage.

## Later gates
Math/font rendering; bounded Markdown parsing/indexing; Python lifecycle/coexistence; feature stages for reader/editor/console/dictionary; hardware integration; restricted EPUB only after Markdown acceptance.
Dependencies are research pins, not a tested lockfile. Build-stage component resolution still needs exact locks. InkPy's own licence is unselected; preserve notices when code/assets are actually ported.

## Validation
Planning files reviewed against user requirements and inspected primary source code. Source SHA pins verified via GitHub. No runtime or hardware results exist. Local scratch directory was not a git checkout; planning commits are made through GitHub Git data APIs with a non-force ref update.
