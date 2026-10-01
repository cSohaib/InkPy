# Hardware port provenance

The Stage 2 diagnostic is a native C / ESP-IDF application. It does not link
Arduino, CrossPoint, a UI framework, MicroPython, or a document renderer.

Board constants, controller detection, monochrome command sequences, touch
decoding, SD power timing and frontlight mixing were adapted from **FreeInk SDK
111fdcc7f0176c3ee38391a160ee296bf492dbd8**, the submodule pinned by the audited
CrossPoint master revision. The MIT notice is retained in
`third_party/notices/FreeInk-LICENSE.txt` and applies to the adapted portions.

| InkPy file | Inspected FreeInk sources, relative to that revision |
|---|---|
| `main/pins.h` | `libs/hardware/BoardConfig/include/BoardConfig.h`, XTEINK_X4_PRO |
| `main/board.c` | `libs/hardware/InputManager/src/InputManager.cpp` (GT911 only); `libs/hardware/SDCardManager/src/SdmmcBlockDevice.cpp`; `libs/hardware/FrontlightManager/src/FrontlightManager.cpp`; `libs/hardware/Rtc/src/Rtc.cpp` (PCF8563 only) |
| `main/display.c` | `libs/hardware/XteinkDetect/src/XteinkDetect.cpp`; `libs/display/FreeInkDisplay/src/bus/EpdBus.cpp`; `libs/display/FreeInkDisplay/src/driver/{Ssd1677Driver,Uc8179Driver,Uc8279X4Driver}.cpp` |
| `main/sleep.c` | Power sequencing informed by `libs/hardware/PowerManager/src/PowerManager.cpp`; MCU light-sleep uses ESP-IDF directly |

Source root: https://github.com/Free-Ink/freeink-sdk/tree/111fdcc7f0176c3ee38391a160ee296bf492dbd8

## Deliberate reductions

- Exactly one board pin map. Preserve its SSD1677/UC8179/UC8279 production panel variants.
- Monochrome full refresh only; no grayscale LUTs, partial-window machinery,
  async rendering, multiple device profiles, or theme framework yet.
- UC8179: reverse the 480 visible rows, then pad to 600 gates. UC8279: 120 white
  padding gates first, then 480 forward rows. Preserve the UC8279 PSR write
  **after power-on**, because power-on reloads its internal defaults.
- Probe on bidirectional MOSI before handing the bus to hardware SPI. Keep two
  agreeing VER reads, FLG checks and the MTP fallback; do not trust OEM NVS to
  identify the installed panel. Inconsistent probe results fail explicitly.
- Fixed 10 MHz display SPI and one 4,000-byte internal DMA transfer buffer.
- One IDF I2C bus; GT911 single-point/Home decode, no swipe/multitouch actions.
  Movement and multiple contacts suppress taps. Keep long-hold timing separate
  from fresh GT911 frames, since a motionless hold may produce no fresh frame.
- One IDF FatFs mount, whole-mount retries, explicit sector-zero validation,
  no formatting, no USB transfer. Diagnostic I/O uses a temporary file opened
  with O_EXCL; an existing file with that name causes failure rather than overwrite.
- RTC read/validity check only in this stage. No time-setting UI.
- Gauge readout is diagnostic raw data; no battery profile writes or low-battery
  policy yet. A full battery service is required before daily use.
- Light sleep preserves RAM. Panel-controller sleep preserves visible pixels but
  invalidates controller RAM; reinitialize only at the next requested draw, not
  on wake. SD remains mounted/powered; no claims of optimal sleep current.
- All board calls are owned by the diagnostic task. A shared storage/I2C locking
  policy must be added before introducing the Python task, not retroactively
  assumed to exist.

## Build provenance

ESP-IDF is pinned to v5.5.5, commit
`b774170ff46c393eeb5e495ea37936038d3f4f4f`, including its git submodule pins.
`scripts/build.sh` rejects a different IDF commit. The dependency set for this
stage is entirely IDF-provided; no external managed components are fetched by
the project. IDF and its bundled components retain their upstream licences.
InkPy's own project licence remains unselected; the FreeInk notice is not a
blanket licence assignment for all new project code.


## Stage 4 addition

The optional math diagnostic adds the shared C++ MicroTeX/FreeType renderer.
See MATH-DEVICE.md for pins, the targeted operator-limit overlay, resource
reductions and build/measurement boundaries. Hardware code remains C. Wake now
accepts one Power click, consumes release bounce, and leaves pixels unchanged.
