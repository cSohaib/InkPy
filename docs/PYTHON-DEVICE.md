# Python on InkPy

Open Console or long-press a .py file and Execute. Globals persist in the session.
Home shows Stop/Close/Cancel; long Home closes. Stop resets the VM but keeps history.
Close stops execution and closes native file/network resources. No execution timeout.

## SD files and imports

open() supports r/w/a, optional b/t and +. Streams support read/readinto/readline,
iteration, write, seek/tell, flush, close and with. Eight simultaneous file/import
streams, 4 KiB I/O chunks and GC finalization. Read-all/listdir/JSON still consume
the 256 KiB VM heap; stream large files.

Relative paths use the script folder, or /sd in a fresh console. Imports search that
folder and /sd/lib (.py modules/packages). Absolute paths stay below /sd; .inkpy-*
firmware caches are excluded. os/uos: getcwd/chdir/listdir/mkdir/rmdir/remove/rename/
stat. FatFs rename does not overwrite. Binary files can be opened in Python.
io/json/sys/gc/math/core builtins are present; time has sleep(seconds)/ticks_ms().
This custom port does not supply socket, machine, threading or input() bindings.

## Wi-Fi and HTTP

```python
import inkpy, time
inkpy.wifi('YOUR_SSID', 'YOUR_PASSWORD')
for attempt in range(100):
    connected, ip = inkpy.wifi_status()
    if connected:
        break
    time.sleep(0.1)
if connected:
    print(inkpy.http('https://example.com/', 'response.html'))
inkpy.wifi_off()
```

inkpy.http(url, destination, *, method='GET', body=None, headers=None) returns HTTP
status and streams the body to an exclusive SD temporary, then renames it.
Destination must not exist. GET/POST/PUT/DELETE, str/bytes body, dict-of-strings headers.
Redirect statuses are returned, not followed. Error bodies are saved too. Transport/
file errors raise OSError and delete partial data; power loss may leave an
.inkpy-download temporary for manual removal.

HTTPS checks hostname/certificates against IDF's bundled roots; set current time/date
in Power first. No insecure bypass, Wi-Fi settings UI, cloud/sync/NTP/transfer server.
Credentials stay in RAM; scripts may store their own on SD.

Polls for Stop/Close/sleep occur between native calls; socket timeouts are 500ms.
SDK DNS/handshake timing can delay a safe point. Sleep stops Wi-Fi after VM pause,
wake reconnects before resume. Remote deadlines continue during pause, so requests
may fail; catch OSError and retry when appropriate.

Host check: bash scripts/test-python-native.sh reuses device-generated headers.
Files/imports/JSON/Unicode and abort/reopen cleanup are tested; network is deliberately
stubbed there. Wi-Fi/TLS/sleep require an X4 Pro.
