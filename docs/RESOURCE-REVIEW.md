# Stage 33 minimalism and resource review

Scope: first-party application, reader/layout, fonts/math adapter, editor/text,
StarDict, Python integration, input/display/sleep and product build configuration.
This is a source review with targeted checks, not a measured claim of optimal
hardware performance. Third-party mathematical layout/FreeType/MicroPython
internals retain their pinned implementations and licences.

Changes:

- Batch display rows through the existing 4 KiB DMA buffer: 12 SPI transactions
  per SSD plane or 15 per UC plane, formerly 480/600. Byte order, white padding,
  waveform commands and buffer size stay the same; stub test compares every byte.

- Deleted global orientation state and its shared pixel-scaling header. Browser,
  editor, console, keyboard and Power always use fixed portrait geometry.
- Deleted landscape-only font squeezing and the special text-grid style flag.
  Reader owns one landscape flag; page pixels rotate directly, with no scaling.
  Its menus/translation prompts stay portrait. Rotation reindexes and locates
  the old source anchor by binary search; opening another book resets portrait.
- Reader menus no longer read/rasterize a page only to clear it immediately.
- Python snapshots skip the roughly 16 KiB console copy when revision is unchanged.
  Previously this copy happened on every UI loop, including idle polling.
- Close releases the Python worker after acknowledged VM/I/O cleanup: 256 KiB
  PSRAM heap plus 48 KiB internal task-stack allocation and task/mutex overhead.
  RTOS idle cleanup can defer stack reclamation briefly. Stop and sleep retain
  an active worker. Removed the separate retained-worker reopen state machine.
- Font names derive from their paths. Removed the fixed 19,456-byte names/paths
  arrays; the optional 16 KiB path catalog is allocated only if SD fonts exist
  (ordinary allocation prefers PSRAM in the product profile). No SD fonts means
  no catalog allocation. Font size is reset only when it changes.

Kept deliberately:

- Windowed text/editor data and SD-backed gap buffer/caches: large files must not
  determine RAM use. Safe replacement/backup on Save remains.
- Markdown parser budget, streaming runs, bitmap spool and explicit cache owners.
- Dictionary order/definition indexes and gzip/dictzip support: bounded RAM,
  actual requested StarDict compatibility, and source file protection.
- Independent input capture and FIFO: catches typing while display/indexing blocks.
- Panel detection/refresh paths: X4 Pro hardware variants need these. No custom LUT
  experiments, increased SPI clock or reduced sleep safety margins in this pass.
- Lazy mathematical initialization, PSRAM preference, 64 KiB internal reserve,
  GC stack/register handling and current task-stack sizes. Lowering these without
  device measurements would reintroduce the allocation/stack crashes just fixed.
- Prototype/diagnostic files and licences: excluded from the product build, useful
  verification references, no product runtime cost.

Remaining performance limits:

- Markdown reindexes the whole document on open/rotation. UI dispatch waits for
  indexing; input still queues. Reusable caches need invalidation design.
- Editor/plain-text previous-page navigation rescans from the beginning; jumping
  the SD gap over large distances also costs I/O. Sparse page anchors could help.
- Glyph rasterization repeats across redraws. A bounded cache may help, but needs
  device timing to justify additional memory and invalidation code.
- Firmware sends complete frames using partial panel waveforms. Region updates
  require per-panel validation; waveform time may dominate drawing improvements.
- Wi-Fi/TLS latency, post-Close internal-heap recovery and sleep current still need
  physical measurements. Do not infer these from build/desktop results.

Next: test repeated Python Close/reopen, math after Close, rotation/word taps and
portrait Power/reader menus, then prioritise measured bottlenecks. EPUB and math
command expansion remain separate feature stages.
