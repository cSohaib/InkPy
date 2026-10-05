# Third-party software and resources

The root MIT license covers original InkPy code, not the following dependencies.
Keep these notices with source and binary distributions.

| Component | License / notice | Source revision |
| --- | --- | --- |
| MicroPython | [MIT](third_party/notices/MicroPython-LICENSE.txt) | `19e685eca906a5a602135a485976253e705297d0` |
| MicroTeX | [MIT](third_party/notices/MicroTeX-LICENSE.txt) | `0e3707f6dafebb121d98b53c64364d16fefe481d` |
| FreeType | [FreeType License](third_party/notices/FreeType-FTL.txt) | `42608f77f20749dd6ddc9e0536788eaad70ea4b5` |
| tinyxml2 | [zlib](third_party/notices/tinyxml2-LICENSE.txt) | `321ea883b7190d4e85cae5512a12e5eaa8f8731f` |
| md4c | [MIT](third_party/notices/md4c-LICENSE.md) | `c7ba975c34d714966ea910c58a97524e5d674ff7` |
| FriBidi | [LGPL 2.1 or later](components/ink_layout/vendor/fribidi/COPYING) | `68162babff4f39c4e2dc164a5e825af93bda9983` |
| DejaVu Sans Mono | [DejaVu / Bitstream Vera terms](third_party/notices/DejaVu-LICENSE.txt) | Bundled subset |
| Bitmap fallback | [Font notice](components/ink_browser/FONT-LICENSE.txt) | Host/test fallback |
| ESP-IDF | [Apache 2.0](third_party/notices/ESP-IDF-LICENSE.txt), plus component notices | `b774170ff46c393eeb5e495ea37936038d3f4f4f` |

FreeType is used under the FreeType License: portions of this software are
copyright © The FreeType Project (www.freetype.org). All rights reserved.
tinyxml2 uses the zlib license, not MIT.

Math glyph resources are distributed separately in `inkpy-sd-resources.zip`.
Their upstream font notices are included inside `inkpy/math/fonts/licences/`;
the staging script excludes `dsrom10.ttf` and Euler fonts. These resources are
not relicensed as InkPy code. MicroTeX's notice is also included in that ZIP.

The release source archive includes vendored FriBidi and the fetched math/Python
sources. Build instructions identify the exact ESP-IDF source revision. For
static-linked LGPL code, preserve the source and the ability to rebuild/relink
when redistributing modified firmware. Dictionaries and user documents are not
included; their respective publishers set their licenses.
