# Stage 2 hardware diagnostic

This default build verifies board primitives. For the optional math build and
latest build measurements, see MATH-DEVICE.md. It is not yet the InkPy file-browser UI.
There is no Markdown, text editor, font selector, dictionary or Python runtime.

## Build

Use Git, Python 3, a C compiler for host tests, CMake and Ninja. The development
run used CMake 3.30.9 and Ninja 1.13.2. Install a fresh pinned ESP-IDF checkout
outside InkPy, rather than switching an unrelated project's SDK:

```sh
git clone --branch v5.5.5 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout b774170ff46c393eeb5e495ea37936038d3f4f4f
git submodule update --init --recursive
./install.sh esp32s3
. ./export.sh
cd ../InkPy
bash scripts/test-host.sh
bash scripts/build.sh
```

On macOS/Linux, install CMake/Ninja through your package manager if absent.
The local Linux verification used the IDF Python environment and
`python -m pip install cmake==3.30.9 ninja==1.13.2`.
The IDF tool installer selects the target toolchain; the project itself is C.
Use a fresh build directory/sdkconfig when reproducing from defaults.
Only `main` and its transitive IDF dependencies are built. The build script disables
the optional IDF Component Manager because this stage has no managed components.

Outputs are under `build/`: `inkpy.bin`, `inkpy.elf`, `inkpy.map`, and the generated
bootloader/partition binaries. **Build instructions are not flashing instructions.**

## Installation gate

Before installing this diagnostic, establish the actual device's current firmware,
recovery/unlock route, partition table and active boot slot. The committed table
mirrors the audited OEM layout solely to size/link the build. It is not evidence
of what is on this particular device. Do not erase flash, write a merged image,
replace the bootloader/partition table or choose an application offset by guess.
No automatic flash action or raw flash command is supplied in this stage.

## What it does

1. Holds the peripheral rail, starts buttons/I2C, initializes touch and the two
   frontlight channels (light initially off).
2. Probes the panel and initializes the selected monochrome driver.
3. Mounts SD without formatting; writes, flushes, reads and removes only its own
   `/sd/.inkpy-bringup.tmp` file. An existing file at that path is left untouched
   and reported as a failed probe. A missing/broken SD card does not stop input testing.
4. Logs memory, RTC validity, raw gauge readings and the running application slot.
5. Draws an asymmetric pattern: clockwise corner squares of 10/20/30/40 pixels
   starting at top-left; one to five central bars indicate the diagnostic page.

### Diagnostic controls (not the final UI)

| Input | Result |
|---|---|
| Previous / Next short press | Cycle the five patterns |
| Tap screen | Draw a cross at the tapped native landscape coordinate |
| Touch hold | Log/draw one hold; release does not become a tap |
| Slide or multiple contacts | No action |
| Home tap | Cycle light warmth 50 → 100 → 0 → 50; brightness stays 20%, on/off unchanged |
| Home hold | Print diagnostic report |
| Power short press | Print diagnostic report (power menu comes in a later stage) |
| Power double press | Toggle frontlight |
| Power hold while awake | Sleep |
| Power short press while asleep | Wake; consume the click |
| Five minutes without input | Sleep (no Python exists in this diagnostic) |

Debounce is 30 ms, double-click window 300 ms, hold 800 ms. Single Power action
is delayed to disambiguate double presses. Full e-ink refresh blocks this initial
diagnostic task; a very short input during a refresh can be missed. This is an
explicit bring-up limitation to remove before editing/console usability tests.
There is no drag gesture, including diagnostic input.

## Hardware acceptance checklist

Record panel VER/FLG, selected driver, device batch if known, and results once a
verified installation route exists. A build or host test does not pass these checks.

- [ ] Boots on battery and connected power; no reset loops. Correct flash/PSRAM sizes.
- [ ] Panel initializes and observes a real BUSY pulse; corners/bars are visible,
      correctly oriented and erase after a page change. Record refresh duration.
- [ ] Tap all four corners; cross lands under each tap. Repeated taps and a
      motionless Home hold work; slides produce no action.
- [ ] Power single/double/hold each causes only its intended diagnostic action.
- [ ] SD read/write probe succeeds; card contents remain otherwise unchanged.
      Missing card and existing probe filename produce bounded failures.
- [ ] Double Power toggles light; warmth 0/100 visibly isolates cool/warm channels.
- [ ] RTC either reports a valid time or explicitly reports unset/untrusted data.
      Gauge readings are plausible; percentage/profile validation is deferred.
- [ ] Sleep leaves the exact pixels in place, turns light off, disables touch and
      ignores side buttons.
- [ ] One short Power click restores touch/light without refreshing the display; page
      index survives, subsequent drawing works, and the wake press is consumed.
- [ ] Repeat sleep/wake, including with SD mounted. Measure battery sleep current
      (USB/logging can change the result); record it rather than assuming deep-sleep draw.

## Python lifecycle contract for later integration

The user confirmed: Close console kills its session, manual sleep suspends the
script, waking resumes it, and there is no script runtime limit. Most scripts
are expected to finish within a minute; this is workload guidance only.
Automatic sleep remains inhibited while a script is running.

Before adding Python, introduce an acknowledged pause/stop protocol. Suspend at
a VM safe point after releasing hardware/storage locks; then enter state-preserving
sleep. Do not force-delete/suspend a task while it may own a lock. Close must
finish termination/cleanup before reporting the console closed. Catchable
KeyboardInterrupt alone is insufficient for an uncooperative script. External
network timeouts and wall-clock time still advance during sleep.

No Python termination or suspension behaviour is implemented or tested by this build.

## Recorded verification (2026-10-01)

- Native host input tests passed: bounce rejection, delayed single, double suppression,
  motionless hold, click-then-hold, separate singles and millisecond-counter wraparound.
- ESP32-S3 build/link and partition-size checks passed using the pinned, clean IDF
  checkout, Xtensa GCC 14.2.0 (esp-14.2.0_20260121), CMake 3.30.9 and Ninja 1.13.2.
  InkPy C sources compile with `-Wall -Wextra -Werror`; no warnings were reported.
- Application binary: **332,112 bytes** (0x51150); linked image reported by IDF:
  331,983 bytes. The configured 0x7e0000 application slot has 96% free.
- IDF size report: DIRAM static use 73,735 bytes, not a measurement of runtime
  heap use or proof that the final reader/Python combination fits.
- Local diagnostic binary SHA-256:
  `95f627702135d6f427081da1bbec5e4855aab20c293a341959b1c99df21cba55`.
  This identifies this build only; SDK metadata/build paths/version fields can
  affect byte-for-byte reproducibility.
- No device was attached, flashed or measured. Every hardware checklist item is pending.

Local setup issues resolved: CMake/Ninja were initially absent; the extracted
toolchain's cc1plus lacked its executable bit and was corrected. The optional
Component Manager failed process discovery in this container and was disabled
for this dependency-free project. No SDK source or hardware checks were patched out.

