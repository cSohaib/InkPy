# Stage 24 checks

- Host `dict-test` and `dict-ui` passed generated dictionary and reader workflows.
- Plain/gzip/RA dictzip, 32/64-bit records, duplicate words/aliases, UTF-8, text/
  markup/typed/binary-last fields, long definitions, invalid indexes/CRC/data,
  choice persistence, and missing-dictionary recovery exercised.
- Reader word hit tests include bold-split and line-wrapped words, punctuation,
  UTF-8 and aliases; chooser/definition paging, Home/Close and page preservation.
- Existing reader/math workflow regression passed.
- Native combined product compiled/linked; own code uses warnings as errors.
  Esptool checksum/hash valid; expected dictionary/reader/Python/input symbols linked.
- Definition/chooser previews inspected. `git diff --check` passed.

Run host checks after preparing the pinned dependencies:
```
cmake -S prototypes/reader -B prototypes/reader/build/math
cmake --build prototypes/reader/build/math -j 4
python3 prototypes/reader/test-dictionaries.py prototypes/reader/build/math/dict-test prototypes/reader/build/math/dict-ui
```

No hardware execution/flashing. Physical input, SD preparation speed, runtime
memory/stack and sleep/panel behavior are not established by desktop/build checks.
The existing downloadable firmware is unchanged.
