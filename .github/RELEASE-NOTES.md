InkPy turns Xteink X4 Pro into a small mathematical device.

Markdown with inline/display LaTeX, MicroPython console and scripts, a text editor,
Arabic reader text, StarDict lookup, and limited EPUB support. One bundled text
font and fixed size. EPUB uses chapter navigation and ignores publisher styling.

Download firmware.bin and inkpy-sd-resources.zip together. Extract the ZIP at
microSD root, then use the CrossPoint web installer's custom application-image
route for X4 Pro. Preserve the bootloader and partition table. See the README
for installation and recovery instructions.

Stage43 firmware behavior was confirmed by the maintainer on their device.
This release rebuilds that source with publication documentation and tooling;
its binary hash may differ from the earlier Stage43 download.

SHA256SUMS verifies the artifacts. licenses.zip contains third-party notices;
inkpy-source.tar.gz includes InkPy and the fetched math/Python sources.
ESP-IDF is pinned by the build script. InkPy code is MIT; dependencies retain
their original licenses.

Python scripts can access files and networking. The examples require your own
Wi-Fi credentials/API key. Do not share scripts containing those credentials.
