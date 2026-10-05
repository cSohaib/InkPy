# Firmware

Use the [latest GitHub Release](https://github.com/cSohaib/InkPy/releases/latest)
for firmware, mathematical glyph assets, checksums, licenses and source.
[Installation and recovery](../README.md#installation) · [Build](BUILD.md)

The image is an **application-only ESP32-S3 image for Xteink X4 Pro**. It does
not include a bootloader or partition table. Preserve the existing layout and
the working CrossPoint web-installer recovery route. No security fuses are
intentionally changed. Other build outputs are not installation images.

Extract the resource ZIP at microSD root so math fonts are at
`inkpy/math/fonts/`. Text uses the bundled font; no SD text-font selection exists.
StarDict dictionaries belong in `dictionaries/` and are supplied separately.
Python API: [PYTHON-DEVICE.md](PYTHON-DEVICE.md).

## Last pre-release device-tested image

Stage43 was confirmed working by the user after the earlier wrong-download
confusion. It adds denser bundled text and typed dictionary lookup in Power.

- ESP-IDF v5.5.5: `b774170ff46c393eeb5e495ea37936038d3f4f4f`.
- ESP32-S3, 16 MiB DIO; browser ON, diagnostics OFF; 32 KiB main stack.
- Application: 2,728,224 bytes; slot: 8,257,536 bytes.
- SHA256: `6617b4fb50e8e7309234b27713d65695297e7a9bb6f6e1862da760383c79416b`.
- Build: `bash scripts/build-browser.sh`; output `build-browser/firmware.bin`.
- Build validation: `results/stage43/image.txt`.

Public release v0.1.0 republishes this exact tested binary. Future rebuilds can
change the hash through build metadata; use each release's SHA256SUMS to verify
its downloads.

## Limits

Hardware feedback confirms the main paths on the maintainer's device, not every
document or dictionary. EPUB compatibility is limited. Dynamic math allocations
can exhaust memory on complex input. Ghosting may require manual Refresh screen.
Light sleep retains SD power and is not deep sleep; current draw is unmeasured.
Networking timeouts continue during sleep and remote connections may expire.
Python runs with file/network access and has no script runtime timeout.

Older stage reports and their pending-feature warnings remain in the development
history; consult the project README for current supported behavior.
