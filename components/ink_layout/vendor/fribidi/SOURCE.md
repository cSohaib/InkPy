# FriBidi reader core

Upstream: https://github.com/fribidi/fribidi
Pinned v1.0.16 commit: 68162babff4f39c4e2dc164a5e825af93bda9983.
LGPL-2.1-or-later; retained COPYING and source copyright notices. Only bidi types,
brackets, mirroring, joining, Arabic shaping and run internals are compiled.
Charset converters, CLI, deprecated APIs and log2vis convenience/debug code are
not compiled. Original library sources/headers are unchanged.

The generated .tab.i files use upstream gen.tab generators, compression level 2,
and the pinned repository's bundled Unicode 16.0.0 data. fribidi-unicode-version.h
uses its gen-unicode-version with ReadMe.txt/BidiMirroring.txt. Generator builds
use DONT_HAVE_FRIBIDI_CONFIG_H, HAVE_STDLIB_H, HAVE_STRING_H, HAVE_STRINGS_H,
HAVE_STRINGIZE and STDC_HEADERS=1, matching cmake/bidi-sources.cmake. Generated
tables are committed to avoid requiring host generators in firmware builds.
The generation tool's version label is "unknown" because configure is not used;
the actual pinned library/data versions are recorded above.

InkPy's adapter is ink_bidi.c: forced per-displayed-line base direction, bounded
128-codepoint input/output, retained visual-to-logical map. No document-wide RTL
mode, dependency-specific settings or additional script/UI features.

Full corresponding source for this static core and InkPy application is in the
repository, with build recipes for relinking it. The original source and generator
recipes/data are also available at the exact upstream revision above.
