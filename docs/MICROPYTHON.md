# MicroPython embedding (host and first device port)

## Stage 21: FreeRTOS/native ESP32-S3 port

With the pinned IDF environment active: `bash scripts/build-python.sh`.
This produces `build-python/inkpy.bin`, an isolated serial diagnostic, **not the
product browser image**. It neither replaces the downloadable firmware.bin nor
requires the user to install another intermediate version.

`scripts/prepare-python-device.sh` reuses the same clean upstream revision and
generates a separate embed package with `main/python_port/mpconfigport.h`.
`main/python_worker.c` ports the host worker protocol to FreeRTOS: one VM owner,
one command slot, locked bounded console/history snapshots, cooperative pause,
uncatchable VM abort for Stop, and Close acknowledgment after VM deinitialization.
Close discards globals; reopening creates a fresh VM. Heap and idle worker remain
reserved for this boot: 256 KiB PSRAM GC heap, 48 KiB internal task stack, 32 KiB
interpreter C-stack limit. One session/worker per boot; do not call Start twice.
No Python thread module, execution timeout, forced task suspension/deletion,
network binding or file-write binding was introduced.
VM loop polls periodically yield one tick so endless bytecode does not starve
the core's idle watchdog task. Blocking native calls still need future cooperation.

Xtensa GC uses the pinned upstream ESP32 register-window spilling strategy,
with sibling-call optimization disabled; it traces this worker's stack only.
Native Xtensa NLR and VM abort link successfully. ESP-IDF owns the assertion
handler; embed's fallback handler is omitted only for embed_util.c. Its remaining
fatal nlr_jump_fail loop is still a limitation, not robust device fatal recovery.
The generated package and this adapted code retain MicroPython's MIT notice.

The diagnostic schedules 6*7, an endless loop catching BaseException, pause,
Stop/reset, gc.collect(), Close and reopen. Its five-second test waits are
diagnostic watchdogs, not runtime policy. **It has compiled/linked but has not
run on hardware.** Reports and linked-symbol checks are in results/stage21.
No runtime correctness, stack margin, GC stress or pause acknowledgment on the
physical device is claimed from a successful link.

Next: wire the existing onscreen console/keyboard and browser Execute into this
adapter. Serialize SD ownership before enabling concurrent browser/editor and
script file reads. Sleep must wait for acknowledged VM quiescence without holding
the session lock; auto-sleep remains inhibited during an active job. Render a
snapshot outside the mutex and throttle output redraws. Separately capture and
queue touch/button events while panel refresh runs; current browser still drops
taps during blocking refresh. Capture events first, coalesce only rendering.

## Existing host prototype

```
bash prototypes/python/run.sh fixtures/python-demo.py
bash prototypes/python/run.sh
bash prototypes/python/run.sh -i fixtures/python-demo.py
python3 prototypes/python/smoke.py
# Once the pinned dependencies/generated package exist:
make -s -C prototypes/python build/inkpy-console build/inkpy-browser-console
(cd prototypes/python && build/inkpy-console && build/inkpy-browser-console)
python3 prototypes/python/preview.py
```

This is native MicroPython, not CPython. The host prototype runs .py files and an
interactive terminal REPL. Globals persist in a session; expressions print their
value. Multiline blocks finish with a blank line, and EOF closes the idle console.
There is no script runtime timeout.

Reuses the audited upstream pin 19e685eca906a5a602135a485976253e705297d0 at
https://github.com/micropython/micropython, its MIT core and ports/embed machinery.
No new release search or ESP32 application framework. Fetch/build checks the exact
revision and clean tracked sources. Generated code stays disposable and keeps
upstream notices; LICENSE remains in the fetched tree. Requires Git, GNU Make,
a GNU-compatible C compiler, Python 3 and libm; no vendor submodules are needed.

components/ink_python provides a small C boundary for initialization, execution,
REPL continuation and normal shutdown. One owner/worker stack; caller owns the
heap. The prototype uses a fixed 256 KiB GC heap and provisional 32 KiB C-stack
limit. Script source feeds the lexer through a read-only FILE stream instead of
a whole-file source buffer; parser/bytecode allocations must fit the VM heap.
Terminal input is capped at 4,095 bytes per block; output streams to stdout.
The onscreen model reserves a continuation newline in its 4096-byte input and
keeps the newest 128 wrapped output lines, seven visible at once.
These are host defaults, not device RAM acceptance measurements.

Core features include doubles, big integers, math, gc and sys. Python open(),
external .py imports, networking, machine bindings and user input() are not yet
provided by this minimal embedding. The host browser Console/Execute routes now
use prototypes/python/app and session: one VM worker, file/REPL command slot,
cooperative pause and uncaught VM abort. Stop resets globals and retains history;
Close acknowledges VM cleanup before returning to the browser. Native blocking
calls need their own cooperation; no universal arbitrary-native-call guarantee.
The product device browser still shows a pending screen; the separate Stage 21
FreeRTOS adapter/diagnostic has not yet been connected to its UI.
SD-backed full history is deferred. Upstream embed fatal internal errors still
use its terminal loop; robust fatal/OOM recovery is deferred.

The fixed English keyboard is now Python-first: quotes, parentheses, square/curly
brackets and backslash share its main layer with letters, colon and underscore.
Digits/operators remain on the symbol layer. Tab emits an indentation action for
the console/editor to insert four spaces; it is ignored in filename
entry. Enter remains an action, allowing a console to submit multiline input.

Basic host checks passed script execution, math/bigints, persistent globals,
multiline input, expression results and continuing after an exception. Recorded
session is in results/stage10/session.txt. Filename creation checks still passed,
including the new Python key positions. The Python keyboard preview was inspected.
No broad compatibility suite, device SDK build, flashing or runtime measurements.
