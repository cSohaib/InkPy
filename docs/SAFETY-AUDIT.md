# Stage 19 installation audit — 2026-10-02

**Decision: do not install yet.** Offline checks passed, but compatibility and
recovery for the user's actual X4 Pro have not been established. No device was
connected; no flash, erase, boot-slot change, or eFuse write was performed.
The user identified the current firmware as CrossPoint/CrossInk; exact product,
version and USB recovery status remain unknown.

## Evidence and boundaries

| Check | Result | Boundary |
|---|---|---|
| Delivered image identity | 344544 bytes; SHA-256 `3d8b7eac5b1b69cf63d1c387ff735f399003677e2ae97022bc55e08ae5e14471` | Hash establishes identity, not device safety |
| esptool 4.12.0 image parsing | Valid checksum and appended validation hash; ESP32-S3 chip ID 9, DIO, 16 MiB, 80 MHz; secure version 0 | Not a signed image; actual device security state unknown |
| Revision requirements | Image permits chip v0.0–v0.99; eFuse block v0.0–v1.99 | Must compare against actual chip; do not bypass revision checks |
| Native firmware rebuild | Pinned IDF v5.5.5 and Xtensa GCC 14.2.0 build passed | Rebuilt artifact is distinct: version/timestamp metadata changed; delivered image remains unchanged |
| Configured partitions | Both reference OTA app slots fit image; 16 MiB reference map does not overlap | Does not establish actual partition map, bootloader compatibility or active slot |
| Board source comparison | Display, touch, light, buttons, I2C and SD pins agree with pinned FreeInk X4 Pro profile | Production panel detection, electrical behavior and touch mapping still require hardware |
| Editor sanitizer tests | POSIX and ESP save branch: 6000 deterministic edit/navigation operations each; exact save bytes, UTF-8/BOM/CRLF preservation, invalid UTF-8/binary rejection passed | Host ASan/UBSan; LeakSanitizer unavailable under this execution environment; not actual FatFs |
| Save failure | Injected replacement-rename failure preserves original via rollback and permits retry | Power loss, full-card conditions and failed rollback not exhaustively simulated |
| Large-file editor | Existing 8 MiB fixture, tail preservation, gap expansion, paging and Tab passed | Host filesystem, not card latency or device stack measurement |
| Browser | Existing paging, folder, Home, format routing and Edit/Execute gate probe passed | Host input/model, not touch hardware |
| GCC static analyzer | `-fanalyzer` compilation of editor/browser/text/keyboard/input: no diagnostics | Not a proof of absence of defects; excludes IDF drivers |
| Destructive operations | Browser port has no OTA/eFuse/flash-writing API; SD mount refuses formatting | Editor writes user files by design; filesystem/power failures can still lose data |
| Memory and power | Previous linker sizing and 20 KiB task-stack configuration retained | Runtime stack/heap, battery, temperature and sleep current unmeasured |

Logs: `results/safety-audit/`; reproducible editor checks:
`bash scripts/audit-editor.sh`. Fixed the browser probe's outdated link list to
include the editor. No application behavior changed in this audit.
Rebuild SHA-256: `73d0c613f3d2b2e76e4f3f1d0437b86e074ad517a5aa9d33819b8e9d003aa054`.
This is an audit build, not a replacement deliverable.

## Recovery blocker

CrossPoint's current official README explicitly warns that unsupported firmware
on a USB-locked unit can strand or brick it when OTA is the remaining route.
CrossPoint and CrossInk are the only officially supported unlocker firmwares.
InkPy has neither an OTA updater nor a recovery/return-to-firmware entry point.
The USB console build setting cannot undo factory restrictions. USB mass storage
or the ability to install CrossPoint through Wi-Fi is not evidence of ROM USB
download access.

Independent confirmation of the risk: CPR-vCodex currently withdraws its X4 Pro
images and blocks flashing while locked-device recovery is investigated.
Do not reuse plain X4/X3 escape-hatch instructions as evidence for X4 Pro.

Preserving a second app slot alone is insufficient: a functioning way to select
and boot it must remain available even if InkPy crashes before its UI starts.
Normal reset is not a guaranteed rollback. A full-flash backup is valuable but
is not a recovery route if it cannot be restored. Encrypted readback may also
be insufficient for restoration, per Espressif.

## Required evidence before installation

1. Exact installed firmware/version and device source/edition. Keep a known-good
   X4 Pro firmware download and its checksum; back up SD contents externally.
2. Demonstrate a recovery path independent of InkPy's functioning UI. Prefer
   actual ESP32-S3 ROM download access and read-only chip/security/flash queries.
   Do not attempt a write merely to discover whether USB access works.
3. Record chip revision, flash capacity, secure boot, encryption and download
   restrictions. With a deliberately selected port, esptool 4.12.0 commands
   `chip_id`, `flash_id`, and `get_security_info` are queries. They may reset the
   device into download mode; they do not write flash. Never bypass protection
   with `--force`, burn eFuses, or alter flash status registers.
4. If reading is permitted, obtain a full backup using the detected capacity;
   verify repeated readback matches and preserve it off-device. Decode the
   actual partition table and OTA selection data, including table location and
   currently running slot; do not infer them from InkPy's reference CSV.
5. Choose an installation/recovery plan from that evidence. Retain the existing
   bootloader, partition table, NVS and known-good app. Do not use IDF's generated
   whole-project flash recipe: it writes bootloader, partition table and OTA data.
   InkPy's current image is application-only. No approved write offsets exist yet.
6. If USB is locked or recovery remains uncertain, stop installation. A supported,
   hardware-tested recovery chain must precede an InkPy release for that unit.

After that gate, initial hardware acceptance must cover boot without SD, mount
failure without formatting, all panel variants actually encountered, corner/Home
touches, side/Power buttons, light channels, save/discard and error paths on a
disposable card, repeated sleep/single-click wake, stack/heap margins and battery
behavior. Use a charged device and stable connection. Interrupted-write recovery
must be tested only where an independent restore route is already established.

## Remaining data/runtime risks

ESP save moves the original to a sibling backup before installing the new file.
This handles ordinary replacement failure but is not power-loss atomic on FAT.
Interrupted saves can leave `.inkpy-*` backups/work files; preserve and inspect
them on a computer before cleanup. Automatic recovery is not implemented.
No validated low-battery save inhibition/shutdown policy exists. Keep originals
backed up; daily-use approval is pending. Display BUSY waits are bounded, but
refresh blocks input. SD stays powered during light sleep; a held/stuck Power
button can delay release handling. Python is not present in this device image,
so Python stop/pause/network behavior has not been device-tested.

## Primary sources checked

- https://github.com/crosspoint-reader/crosspoint-reader — USB-locked device warning.
- https://franssjz.github.io/cpr-vcodex/flash.html — X4 Pro withdrawal/recovery notice.
- https://docs.espressif.com/projects/esptool/en/latest/esp32s3/esptool/advanced-commands.html — security-info query.
- https://docs.espressif.com/projects/esptool/en/latest/esp32s3/esptool/basic-commands.html — image/readback/write boundaries.
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/security/flash-encryption.html — encrypted-backup limitations.
- https://github.com/Free-Ink/freeink-sdk/blob/111fdcc7f0176c3ee38391a160ee296bf492dbd8/libs/hardware/BoardConfig/include/BoardConfig.h — pinned X4 Pro pin profile.
