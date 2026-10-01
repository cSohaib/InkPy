# InkPy checkpoint

Updated: 2026-10-01
Stage 2 source/build complete; physical hardware validation pending. Pause here.

## Read first
Read AGENTS.md and this checkpoint. docs/UI.md owns interaction requirements;
docs/BRINGUP.md owns the diagnostic instructions/results; docs/PORTING.md owns
port provenance. The earlier COMPONENT-AUDIT.md remains a dated research record.
Use its pinned references instead of repeating source discovery.

## New confirmed Python requirements
Close console stops/kills its session; no background execution. Manual sleep
suspends the running script, wake resumes it. No hard runtime limit. Scripts
usually taking under a minute are the intended workload, not an execution cap.
The existing auto-sleep exemption for running Python is retained. These are
recorded contracts; Python is not present in the current diagnostic.

## Implemented
- Native C/ESP-IDF build restricted to ESP32-S3 and required components;
  scripts/build.sh enforces IDF b774170ff46c393eeb5e495ea37936038d3f4f4f (v5.5.5).
- main/pins.h: fixed X4 Pro map; main/board.c: shared I2C, GT911/Home, dual light,
  RTC read/validity, raw gauge report, SDMMC/FatFs mount with bounded retries,
  sector validation and non-overwriting temporary-file write/read test.
- main/display.c: two-pass live controller probe/MTP fallback, SSD1677/UC8179/
  UC8279 monochrome full-frame ports, internal DMA bounce buffer, timeout/error
  reporting and panel sleep. Variant orientation/padding is preserved.
- main/input.c: debounced short/double/long recognition; meaningful host tests
  in tests/input_test.c, invoked by scripts/test-host.sh.
- main/sleep.c: light sleep, retained RAM/pixels, touch/light shutdown and restore,
  long-Power qualification, consumed wake press, no redraw on wake.
- main/main.c: diagnostic patterns, tap/hold coordinates, paging buttons,
  light controls, reports and five-minute idle sleep. Product UI is deferred.
- partitions.csv mirrors the source reference layout for linking only, not
  permission/evidence to replace the actual device's partition table.
- FreeInk MIT notice retained; no full SDK/Arduino/UI framework imported.

## Verified
Host input tests passed. Native IDF build/link and image-size checks passed with
InkPy warnings-as-errors. Application binary 332,112 bytes; 96% of configured app
slot free. Static DIRAM use 73,735 bytes; runtime memory remains unmeasured.
See BRINGUP.md for tool versions, binary SHA and local setup fixes.
No hardware attached or flashed. No panel, touch, sleep-current or SD behaviour
has been demonstrated on the user's device. No MicroPython lifecycle test exists.

## Exact next step: hardware handoff
Before any flash command, establish installed firmware/version, actual partition
layout/active slot and a known working recovery route for this particular device.
Then follow the hardware checklist in BRINGUP.md and retain the diagnostic log.
If no hardware session is available, pause at this gate; do not describe the
board port as device-validated. Independent math/font host prototyping can be
its own later stage when requested, with hardware readiness still marked pending.

## Known limits and follow-up
- Full refresh blocks the diagnostic task: short touches during refresh can be
  missed. Separate input/render scheduling before the actual editor/console UI.
- All hardware calls have one owner today. Add storage/I2C arbitration and a
  quiesce handshake before a Python worker can access them. Never force-delete
  or suspend a task while it owns these locks.
- SD remains mounted/powered in light sleep. Current draw needs measurement;
  CPU sleep is not claimed equivalent to deep sleep. Screen pixels are retained.
- Battery service is raw read-only: profile validation/low-battery handling and
  RTC setting are not completed. Do not treat this diagnostic as daily-use firmware.
- No script runtime limit; any future peripheral-operation timeout protects an
  I/O transaction, not total script duration. External time still passes in sleep.
- MD4C bounded input and MicroTeX resource/backend work remain later gates.
- Open choices: extensionless script execution, final keyboard layout and
  InkPy licence. Proposed English/UTF-8 UI/content defaults remain as documented.

## Resume efficiently
Inspect main/ and BRINGUP.md for the current stage, not the entire SDK again.
Reference files/toolchain under the preceding session's scratch directory are
only disposable caches. Repository sources and pinned upstream revisions are
the durable checkpoint. Dependencies are all from the pinned IDF this stage;
Component Manager is disabled until an actual managed dependency is introduced.
