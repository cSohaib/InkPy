# Device test notes — 2026-10-03

Stage 27 device results supplied by the user. Original Stage 28 investigation
below is retained; Stage 29 changes are recorded separately.

## Confirmed observations
- Stage 27 boots on the X4 Pro; the previous startup/rollback blocker is passed.
- Python console evaluates `1+1` to `2`. Close and reopen worked at least once.
- Opening Markdown reports `Cannot create reader cache`.
- Bare `sqrt(16)` fails. The built math module is linked/registered; standard
  usage is `from math import sqrt`, followed by `sqrt(16)` (expected `4.0`).
  The user's exact exception and imported-math behavior were not captured.
- After additional OS use, opening Console reports `Cannot allocate Python
  session` (user-reported wording; no runtime log for this sequence).

## Python lifetime: confirmed implementation mismatch
`main/python_worker.c` owns a 256 KiB PSRAM VM heap, a 48 KiB internal task
stack, mutex, and persistent worker task. Close cooperatively interrupts work,
closes tracked files/network/listings, calls `mp_deinit`, and acknowledges closed.
The next open clears the console and initializes a new VM. Python globals and
history are not deliberately saved across Close. However, the task, heap, and
mutex remain reserved until reboot (`main/python_worker.h` documents this).

User requirement is now explicit: Close must stop execution, destroy the VM,
release its resources, and leave no Python session behind. Future implementation
must replace the retained-worker policy with orderly shutdown and resource
release after acknowledgment; avoid deleting a live worker during native I/O.
No script runtime limit is requested. Clarify UI wording: console/process,
not a saved session.

The allocation error in `main/browser_app.c::open_requests` occurs only when
`python_started` is false. A successful first open sets that flag true and it
is never cleared within the current boot; reopen does not allocate. Therefore
normal retained-worker reopening alone cannot explain the reported later exact
message. Check for an intervening reset/reboot, state corruption, or different
actual error text before claiming a leak. Capture boot identity/reset reason,
free/largest internal and PSRAM blocks, and worker allocation stage on failure.
The first-open path can fail at mutex, 256 KiB heap, or contiguous 48 KiB task
stack allocation. Repeated Close/Open and reader-to-console sequences need
physical testing after the lifetime change.

## Reader cache: confirmed failure location, cause unresolved
`components/ink_reader/ink_reader.c::ink_reader_open` calls
`mkdir(/sd/.inkpy-reader, 0700)`. Only failure with errno other than EEXIST
produces this exact message. It precedes owner-marker and cache-file opens.
Current error text omits errno. Possible categories include SD capacity/write
access, filesystem/I/O/path failure, or allocation pressure; none is confirmed.
The native profile enables FAT long filenames and directory operations.

`main/browser_app.c` attempts math initialization before creating the cache
when `/sd/inkpy/math` exists. The Stage 27 deferred tables then allocate and
remain resident, including if reader opening fails. This is a plausible source
of shared resource pressure, not proof of the mkdir failure or Python error.
The reserved Python stack/heap also remain after Console closes. Check the
filesystem errno/FatFs result and SD writes independently, then inspect memory
before/after math initialization and Close. Do not ask the user to delete or
format files as a speculative repair.

## Next bounded implementation (after user resumes coding)
1. Add focused failure diagnostics and boot identity to distinguish resets from
   resource failures; reproduce Console → Close → Markdown → Console.
2. Implement complete cooperative Python shutdown/resource release; verify fresh
   state and reclaimed memory through repeated open/close.
3. Fix the cache failure from its actual errno/underlying result. Check allocating
   math before an unsuccessful cache operation and appropriate ownership/lifetime.

No firmware rebuilt/delivered for this documentation-only checkpoint. Current
application remains Stage 27, 2,583,904 bytes, SHA-256
`a36fdfeeae8e9c1968b97889b828f96b9d679e8008a24fa2cab073c166ce1b9c`.

## Stage 29 retest
Cache preflight now checks the directory before mkdir and reports errno in serial
logs. Math is deferred until the first rendered formula, after cache creation.
This removes one plausible source of early memory pressure, without establishing
the actual reported cause. Retest Markdown with and without math assets; capture
the ui/reader warning if it fails. No Python lifecycle fix was made.

Check new-file save and console-created files immediately appear after app exit;
full-width keys at both edges; icon Power controls in all contexts; nested SD font
selection and dictionary chooser; repeated Close/Open and sleep/wake.
