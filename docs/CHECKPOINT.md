# InkPy checkpoint

Updated: 2026-10-01
Stage 0: scope and workflow recorded. Pause here.

## Completed
- Repository inspected: initially empty, default branch main.
- AGENTS.md records efficient reads, batching, bounded stages, progress commits, and handoff requirements.
- README.md records confirmed scope and provisional architecture.
- No firmware, dependency import, build, or hardware experiment performed.
- Documentation checked for consistency with the user's scope; no software tests apply yet.

## Decisions and rationale
Confirmed: X4 Pro only; fixed minimal UI; Markdown math rather than full LaTeX; editor, MicroPython scripts/console, fonts, StarDict; SD file transfer; networking only as needed by Python; restricted EPUB later.
User requests continuity through commits, including planning/research checkpoints, and explicit pauses between stages.
Proposed: C + ESP-IDF with narrowly scoped C++ dependencies. Avoid translating useful libraries purely for language uniformity.
Proposed: English UI and a single programming-friendly keyboard; exact layout remains undecided.
Proposed: shared native layout engine, bounded document caches, SD-backed editing, and explicit memory ownership.
Preserve font/library licences when porting; project licence remains to be selected.

## Existing source leads (reuse; inspect exact code only when needed)
These were identified in the preceding discussion through web retrieval. No revisions are pinned and no port compatibility is established.
- CrossPoint reference: https://github.com/crosspoint-reader/crosspoint-reader
  Use X4 Pro hardware handling as a reference, not the entire application architecture. Inspect a pinned revision before selecting code.
- Board configuration: https://github.com/tuya/TuyaOpen/blob/master/boards/ESP32/XTEINK_X4_PRO/board_config.h
  Reports ESP32-S3, 16 MB flash, 8 MB octal PSRAM. Verify driver details and actual board assumptions before implementation.
- MicroPython ESP32 reference: https://docs.micropython.org/en/latest/esp32/quickref.html
- MicroPython S3 target: https://micropython.org/download/ESP32_GENERIC_S3/
  Existing target support is established; integrating the VM, SD ownership, network access, console and interruption remains work.
- Candidate math renderer: https://github.com/NanoMichael/MicroTex
  Embeddable C++ math renderer; size, dependencies, font needs and device performance unverified. Candidate only.

## Next bounded stage: component audit and feasibility specification
Research/design only; stop before firmware implementation.
1. Inspect and pin relevant CrossPoint hardware code, MicroPython ESP32 port, and one credible math renderer candidate. Record versions, relevant paths, licences, dependencies and reuse/removal decisions.
2. Specify a small Markdown/math compatibility corpus: paragraphs, emphasis, lists, code, tables, local images; Greek symbols, fractions, scripts, roots, sums, integrals, matrices and aligned equations. These are proposed coverage, not an already supported feature list.
3. Resolve integration approach and build-version constraints; identify memory consumers and measurements required. Do not claim measured budgets before a prototype exists.
4. Record a bounded bring-up/prototype plan and update this checkpoint. Commit and pause.

## Later stages (provisional; split when necessary)
- Minimal board bring-up and repeatable build: display, touch/buttons, SD and basic power management.
- Math/font and refresh/keyboard probes; MicroPython coexistence probe. Hardware evidence required before declaring feasibility proven.
- Markdown reader using shared layout, pagination and bounded caching.
- SD-backed text editor with bounded indexing, reliable save/recovery and long-line handling.
- MicroPython script runner and console with bounded output and interruption behaviour.
- StarDict integration with reader hitboxes and bounded lookup memory.
- Device-level integration and usability checks.
- Restricted EPUB import only after Markdown is accepted.

Each stage ends with a committed checkpoint and pause. The list is not authorization to implement all stages in one run.

## Open risks
Math coverage and font footprint; e-ink typing latency; peak RAM with Python/networking; interrupting blocking scripts; SD ownership and save recovery; board-specific sleep/wake and installation/recovery.
An English-only UI must not accidentally imply ASCII-only document content. Text encoding/glyph coverage is a separate decision.
No browser-equivalent math compatibility, arbitrary EPUB fidelity, or desktop Python package compatibility has been promised.
