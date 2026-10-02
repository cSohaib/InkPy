#pragma once
#include <stdint.h>
#include <stdio.h>
/* Experimental host core, one owner. Outputs are disposable caches, never source
 * edits. Caller publishes them only after success; errors leave partial output.
 * Monospaced codepoint metrics today, not a complete shaping/font engine. */
enum { INK_BLOCK_BYTES=8192, INK_MD_HEAP_BYTES=131072, INK_TITLE_BYTES=192 };
enum { INK_BOLD=1, INK_ITALIC=2, INK_CODE=4, INK_LITERAL=8, INK_MATH=16, INK_IMAGE=32 };
typedef struct { unsigned width, height, font_pixels, read_bytes; } ink_layout_config;
typedef struct {
    uint64_t source_bytes, pages, chapters, literal_blocks, runs;
    size_t context_bytes, parser_peak_bytes;
    uint64_t error_offset;
    char error[160];
} ink_layout_stats;
/* draw: [u16 x,y,cell,style; u32 byte_count; u64 source; UTF8 payload]
 * pages: fixed 32 bytes [u64 draw_begin,draw_end,source; u32 chapter,reserved]
 * chapters: fixed 212 bytes [u64 source; u32 page,id; u16 length,truncated; 192 bytes]
 * Integers little-endian. Page and chapter IDs are one-based; chapter 0 means none.
 * Page anchors refer to the first run, not an arbitrary raw-file byte boundary.
 * Run source anchors are approximate for synthetic markers and normalized text. */
int ink_layout_run(FILE *source, FILE *draw, FILE *pages, FILE *chapters,
                   const ink_layout_config *config, ink_layout_stats *stats);
