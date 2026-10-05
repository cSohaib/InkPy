# Stage 24: StarDict reader lookup

The reader supports tap-word lookup; Power's AZ tile also provides keyboard input
and paged definitions. Both use the same Change dictionary picker. Selection retries the same
word and preserves the book page; Home dismisses the topmost prompt first. The
choice is saved on microSD and restored after reboot. No settings/cloud/transfer
feature was added.

## Files on microSD

Unpack a downloaded dictionary archive on the computer, then copy its files to:

```
dictionaries/en-fr/en-fr.ifo
dictionaries/en-fr/en-fr.idx     # or en-fr.idx.gz
dictionaries/en-fr/en-fr.dict    # or en-fr.dict.dz
dictionaries/en-fr/en-fr.syn     # optional
```

Files in each family must have the same basename. Discovery reads `.ifo` files
under `/sd/dictionaries`, including up to eight nested directory levels, ignoring
hidden entries and `res` directories. Dictionary names come from `bookname`;
long names are cropped for the UI. Lists are paged in filesystem discovery order.
The first discovered dictionary is the default when no choice has been saved.
Invalid dictionaries report an error and leave Change dictionary available.

## Supported data

- Normal StarDict 2.4.2/3.0.0 dictionaries, 32/64-bit offset fields, `.syn` aliases.
- Plain `.idx`/`.dict`, gzip `.idx.gz` and dictzip `.dict.dz` with CRC/size checks.
- Repeated headwords and repeated aliases; ASCII case-insensitive lookup, exact
  UTF-8 non-ASCII keys. No automatic stemming, lemmatization or locale collation.
- `sametypesequence` and per-field type markers, including omitted last-field
  terminators/lengths. Text fields are streamed; HTML/Pango/XDXF tags are stripped,
  common/numeric entities decoded. This is text extraction, not an HTML renderer.
- Binary/media fields and resource references are skipped with an explicit label;
  locale-encoded `l` data is also labelled omitted. Audio, images/resource databases,
  tree dictionaries and StarDict plugin behavior are outside this text-reader scope.

The independent C implementation follows the upstream
[StarDict format specification](https://github.com/huzheng001/stardict-3/blob/master/dict/doc/StarDictFileFormat).
Gzip extraction reuses the already pinned FreeType `FT_Stream_OpenGzip`; no new
third-party code revision or GPL application code was imported.

## Storage and ownership

`/sd/.inkpy-dict` is an owner-marked disposable cache plus saved selection.
Compressed data is streamed to SD, not loaded into RAM. Fixed-width offset files
enable binary search without an in-memory word list. Definition text and page
offsets also live on SD; only 16 visible lines are retained in RAM.

Indexes are validated for counts, ordering, UTF-8 keys and data bounds. First
selection prepares the index/decompresses the selected data. A source signature
(paths, sizes, modification times) and ordinal lengths enable disk-cache reuse
on reopen/reboot, rebuilding when sources change or indexes are missing/truncated.
The active dictionary is reused across book sessions during
that boot. Source `.ifo`/`.idx`/`.dict`/`.syn` files are opened read-only. Editing
files in the dictionary folder first releases cached source handles. Ordinary
lookups do not rewrite dictionary files or require networking.

First preparation can take time and requires free SD space for uncompressed data
and offset files. It yields to input capture, but UI dispatch waits for completion.
Asynchronous preparation/cancellation remains a future optimization. Native file
seeks limit files/cache offsets to LONG_MAX (2 GiB
minus one on this target), despite accepting 64-bit offset records within that range.
Deleting the generated cache forces preparation/default selection again; preserve
source dictionary folders. A foreign/unmarked cache directory is never overwritten.

## Reader and validation limits

Word hit testing uses the same cached text geometry, handles style-split and
line-wrapped words, strips edge punctuation and excludes math bitmaps. Definition
and chooser paging uses side buttons; Close/Home returns without changing the
book page. Power remains an overlay on completed reader/lookup/chooser views.

UTF-8 data uses the bundled Unicode font; no SD text-font loading. Typographic
apostrophe normalization and full Unicode word segmentation/case folding are not
implemented. Tests cover the supported paths, not every possible dictionary.

Host tests use generated original fixtures (no downloaded dictionary licence
ambiguity), including gzip/RA dictzip chunks, aliases/homographs, typed fields,
UTF-8, markup, long definitions, corrupt data and reader modal behavior. Native
build/link, esptool checksum/hash and linked-symbol checks passed. Previews were
inspected. See `docs/results/stage24/`. No device was connected or flashed; physical
tap accuracy, preparation time, SD/RAM/stack margins and sleep remain unverified.
The downloadable firmware remains Stage 20 while integration continues.
