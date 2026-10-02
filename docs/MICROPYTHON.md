# MicroPython host prototype (through Stage 15)

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
The device has no FreeRTOS Python integration yet, and shows a pending screen.
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
