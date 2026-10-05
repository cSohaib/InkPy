# InkPy

Turn an **Xteink X4 Pro** into a small mathematical device: read Markdown with
LaTeX, write notes and formulas, and run Python on an e-ink screen.

InkPy is deliberately minimal. One bundled font, one text size, a Python-friendly
keyboard, files on microSD, and a small icon-based interface. No cloud, sync,
USB file transfer, accounts, or app store. Wi-Fi is available to Python scripts.

## What it does

- **Markdown + LaTeX:** inline `$…$` and display `$$…$$` mathematics, fractions,
  roots, Greek letters, accents, operators, and matrices; headings and tables.
  Mathematical notation is supported, not full LaTeX documents or packages.
- **MicroPython:** interactive console, `.py` scripts, `input()`, SD files,
  JSON, Wi-Fi, and HTTP/HTTPS through the `inkpy` module.
- **Text editor:** create and edit plain-text files, including Markdown and Python.
- **StarDict:** tap a reader word or type a query from the Power menu's AZ button.
  Dictionary selection lives in the lookup dialog.
- **Arabic:** connected letters and mixed Arabic/Latin layout in Markdown and EPUB.
- **EPUB:** basic chapters, text, and images through the shared Markdown renderer.
  EPUB is secondary: publisher styling is ignored, compatibility is limited,
  and navigation uses chapters rather than whole-book page numbers.

Text is justified in the reader. Landscape is available only inside the reader.
There is no selectable text-font system; formula resources are separate.

## Downloads

- [Latest release and release notes](https://github.com/cSohaib/InkPy/releases/latest)
- [firmware.bin](https://github.com/cSohaib/InkPy/releases/latest/download/firmware.bin)
- [Markdown/math SD assets](https://github.com/cSohaib/InkPy/releases/latest/download/inkpy-sd-resources.zip)
- [Checksums](https://github.com/cSohaib/InkPy/releases/latest/download/SHA256SUMS)
- [Markdown + LaTeX example](fixtures/markdown-math.md)

Release assets are supplied by the release workflow. These download links become
available when the first release finishes publishing.

## Installation

**Only X4 Pro is supported.** Device testing used an unlocked X4 Pro and the
CrossPoint web installer. Back up your microSD contents first.

1. Download `firmware.bin` and `inkpy-sd-resources.zip` from the same release.
2. Extract the ZIP at the microSD root. The resulting path must include
   `inkpy/math/fonts/`, without an extra enclosing ZIP directory. The bundled
   text font needs no installation; these assets supply mathematical glyphs.
3. Copy your `.md`, `.py`, `.txt`, and `.epub` files to the card and insert it.
4. Open the [CrossPoint web installer](https://crosspointreader.com/#flash-tools)
   in a browser with Web Serial support. Connect the device and choose X4 Pro.
   Use its custom/local firmware option to select `firmware.bin`.
5. Flash the **application image** using the installer's existing X4 Pro layout,
   then disconnect and restart the device.

InkPy's binary is an application image, not a merged factory image. Keep the
existing bootloader and partition table. Do not flash it at address zero or use
an erase-all/factory-flash procedure. If the installer cannot accept an application
image, stop and check its instructions. To recover, use the same supported
installer route to reinstall CrossPoint; keep a known-working image available.
InkPy does not intentionally change security fuses or disable recovery.

## Using it

The home screen lists files and folders, with **+** for a new file and **>_** for
the console at the bottom. Tap to open; long-press a file to edit, execute, or
delete. Markdown and EPUB open in the reader; other text opens as plain text.
Side buttons turn pages. Touch swipes are not used.

Home opens the current app's menu. In the editor, this is where you save or discard.
**Long Home discards changes, stops Python, and returns home.** Short Power opens
brightness, warmth, dictionary, refresh, and contrast controls. Double Power toggles
the light; long Power sleeps. Short Power wakes. Sleep preserves app state and
pauses Python; closing the console ends its running process. Scripts have no fixed
runtime timeout, but short scripts suit this device best.

### Dictionaries

Copy unpacked StarDict dictionaries into `dictionaries/` on microSD, for example:

```text
dictionaries/en-fr/en-fr.ifo
dictionaries/en-fr/en-fr.idx
dictionaries/en-fr/en-fr.dict
```

The basename must match. `.idx.gz`, `.dict.dz`, and optional `.syn` aliases are
also supported. First use may take time and requires space for generated indexes
and decompressed data. Subsequent lookups reuse the SD cache. Dictionary files
are your own downloads and retain their own licenses. See [StarDict details](docs/STARDICT.md).

### Python makes it more than a reader

Python opens the door to calculators, small utilities, network requests, file
generation, and experiments. The [examples folder](examples/README.md) includes
an OpenAI agent and a Markdown document generator: the device can ask an AI a
question or generate a document to read locally. Supply your own Wi-Fi credentials
and API key; do not publish a modified script containing them.

The system clock is set only through Python:

```python
import inkpy
inkpy.set_time(2026, 10, 5, 14, 30, 0)  # year, month, day, hour, minute, second
```

Set the correct clock before HTTPS requests. Python is MicroPython, not desktop
CPython; desktop packages are not generally available. Scripts have access to
the card and networking, so run scripts you trust.

## Source and license

[Build instructions](docs/BUILD.md) · [Security](SECURITY.md) ·
[Dependency licenses](THIRD_PARTY.md) · [Development history](docs/README.md)

InkPy's original code is licensed under the [MIT license](LICENSE).
Bundled libraries and font resources retain their own licenses, including
FriBidi's LGPL; the MIT license does not replace those terms.

This is a hobby project developed and tested iteratively on one device.
Reports with a small reproducing document or script are welcome. CrossPoint
served as a hardware and interaction reference; InkPy is an independent project.
