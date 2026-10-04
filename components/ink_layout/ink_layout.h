#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
/* Experimental host core, one owner. Outputs are disposable caches, never source
 * edits. Caller publishes them only after success; errors leave partial output.
 * Monospaced codepoint metrics today, not a complete shaping/font engine. */
enum { INK_BLOCK_BYTES=8192, INK_MD_HEAP_BYTES=131072, INK_TITLE_BYTES=192 };
enum { INK_BOLD=1, INK_ITALIC=2, INK_CODE=4, INK_LITERAL=8, INK_MATH=16, INK_IMAGE=32, INK_RULE=64, INK_BITMAP=32768 };
/* Optional math callback writes a 480x800, 1=black bitmap, stride 60 bytes.
 * Return nonzero for literal fallback. Storage remains owned by layout. */
typedef int (*ink_layout_math)(const char *source, int display, unsigned pixels,
                              uint8_t *bitmap, unsigned *width, unsigned *height,
                              unsigned *baseline);
typedef int (*ink_layout_image)(const char *resource,unsigned max_width,unsigned max_height,
                                uint8_t *bitmap,unsigned *width,unsigned *height);
typedef struct {
    unsigned width, height, font_pixels, read_bytes;
    ink_layout_math render_math;
    ink_layout_image render_image;
    FILE *chapter_spool; /* EPUB source anchors: u64 offset,u16 title length,192 title bytes. */
    FILE *table_spool; /* Optional caller-owned row scratch file, separate from bitmaps. */
    FILE *bitmap_spool; /* Optional caller-owned seekable scratch file. */
    bool (*cancelled)(void);
    void (*progress)(void); /* Optional cooperative scheduling during indexing. */
} ink_layout_config;
typedef struct {
    uint64_t source_bytes, pages, chapters, literal_blocks, runs, formulas, math_fallbacks;
    size_t context_bytes, parser_peak_bytes;
    uint64_t error_offset;
    char error[160];
} ink_layout_stats;
/* draw: [u16 x,y,cell,style; u32 byte_count; u64 source; UTF8 payload]
 * INK_BITMAP runs: cell is image width, payload is u16 height followed by packed
 * rows (ceil(width/8) bytes each), 1=black. Requires cache version 2.
 * INK_RULE runs: cell is rectangle width, payload is u16 height (solid black).
 * pages: fixed 32 bytes [u64 draw_begin,draw_end,source; u32 chapter,reserved]
 * chapters: fixed 212 bytes [u64 source; u32 page,id; u16 length,truncated; 192 bytes]
 * Integers little-endian. Page and chapter IDs are one-based; chapter 0 means none.
 * Page anchors refer to the first run, not an arbitrary raw-file byte boundary.
 * Run source anchors are approximate for synthetic markers and normalized text. */
int ink_layout_run(FILE *source, FILE *draw, FILE *pages, FILE *chapters,
                   const ink_layout_config *config, ink_layout_stats *stats);

/* Incremental owner: each step finishes at a physical-line/parser boundary.
 * A long Markdown block can produce several screens in one step. */
void *ink_layout_begin(FILE *source,FILE *draw,FILE *pages,FILE *chapters,
    const ink_layout_config *config,ink_layout_stats *stats);
int ink_layout_step(void *layout); /* -1 error, 0 more input, 1 EOF */
int ink_layout_save(void *layout,FILE *state);
void *ink_layout_restore(FILE *state,FILE *source,FILE *draw,FILE *pages,FILE *chapters,
    const ink_layout_config *config,ink_layout_stats *stats);
void ink_layout_end(void *layout);
