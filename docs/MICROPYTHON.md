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
Stage 21 originally linked auto-selected Xtensa NLR; Stage 22 replaces that
selection with upstream ESP32 setjmp NLR. VM abort links successfully. ESP-IDF owns the assertion
handler; embed's fallback handler is omitted only for embed_util.c. Its remaining
fatal nlr_jump_fail loop is still a limitation, not robust device fatal recovery.
The generated package and this adapted code retain MicroPython's MIT notice.

The diagnostic schedules 6*7, an endless loop catching BaseException, pause,
Stop/reset, gc.collect(), Close and reopen. Its five-second test waits are
diagnostic watchdogs, not runtime policy. **It has compiled/linked but has not
run on hardware.** Reports and linked-symbol checks are in results/stage21.
No runtime correctness, stack margin, GC stress or pause acknowledgment on the
physical device is claimed from a successful link.

## Stage 22: product console and input capture

The device browser now opens the onscreen REPL and executes selected `.py` files
through the same worker. Enter submits a command; globals persist until Stop or
Close. Side buttons page current-session history. Home exposes Stop/Close/Cancel;
long Home requests Close. The browser returns only after VM cleanup acknowledgment.
Power remains global while the console or its prompt is open.

Manual sleep waits for a cooperative VM pause acknowledgment, then pauses input
capture before touching panel/light/touch sleep controls. Wake resumes the worker.
Auto-sleep is inhibited while Python is busy. A five-second failed pause handshake
leaves the device awake; this is not a script runtime limit. Browser/editor SD
access and script execution are separated by console ownership and Close acknowledgment.
Snapshots are drawn outside the worker mutex, with output changes coalesced.

`main/input_capture.c` polls touch/buttons every 10 ms in a separate FreeRTOS task,
queues up to 256 discrete events, and continues while the UI performs a blocking
refresh. The UI consumes FIFO events in batches before drawing. Repeated taps are
separate events; swipes remain ignored. Queue overflow is logged, not silently
hidden; controller timing and physical fast typing still need device verification.
RTC reads and touch polling share an I2C mutex. The display driver still performs
full refreshes: partial refresh is a separate pending task.

The native configuration now explicitly uses `MICROPY_NLR_SETJMP`, matching the
pinned upstream ESP32 port. This also removes the out-of-range assembly tail jump
seen when the small diagnostic was linked into the larger browser application.
The original embedded architecture autodetection is no longer used for native NLR.
Both product and diagnostic builds link; host script/REPL/UI checks and input event
checks passed. See results/stage22. Neither native VM execution nor actual touch,
sleep/resume, stack margin or runtime heap has been verified on this device.
The downloadable Stage 20 firmware remains unchanged while integration continues.

Next: connect Markdown/math rendering to the device reader; keep partial refresh,
fonts/StarDict, remaining power controls and Python file/network bindings tracked
as pending rather than treating this console integration as full feature completion.

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
The Stage 22 product browser now connects the FreeRTOS adapter to its UI;
native execution remains untested on physical hardware.
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
