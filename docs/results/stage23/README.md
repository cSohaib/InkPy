# Stage 23 checks

- Combined native product build: passed, own sources use warnings as errors.
- Esptool application checksum/hash: valid; linked symbols include reader/layout/math/Python/input.
- Host `reader-device`: formula bitmaps/fallback, H2/chapter menu, numeric page entry, paging, cleanup, no-math path and binary rejection passed.
- Existing layout checks: 255 passed after chapter/page behavior change.
- Page and Home-menu images generated from the same native-frame renderer and visually inspected.
- Source fixture was read-only; no device was connected or flashed.

Build size/hash are in build.txt. Native runtime memory, input timing, SD/panel
behavior, sleep/resume and formula execution on hardware remain unverified.
