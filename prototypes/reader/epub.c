#include "ink_font.h"
#include "ink_math.h"
#include "ink_reader.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static int render(const char *s, int display, unsigned pixels, uint8_t *bits, unsigned *w, unsigned *h,
                  unsigned *base) {
    ink_math_result r;
    int status = ink_math_render(s, display, (int)pixels, bits, &r);
    *w = r.width;
    *h = r.height;
    *base = r.baseline;
    return status;
}
static unsigned images(ink_reader *r) {
    rewind(r->draw);
    unsigned char h[20];
    unsigned count = 0;
    while (fread(h, 1, 20, r->draw) == 20) {
        unsigned style = h[6] | h[7] << 8, size = h[8] | h[9] << 8 | h[10] << 16 | h[11] << 24;
        if (style == INK_BITMAP)
            count++;
        assert(!fseek(r->draw, (long)size, SEEK_CUR));
    }
    return count;
}
static void pages(ink_reader *r, const char *root, const char *prefix) {
    unsigned total = (unsigned)r->stats.pages;
    uint8_t frame[48000];
    char path[1024];
    for (r->page = 1; r->page <= total; r->page++) {
        assert(!ink_reader_draw(r, frame));
        snprintf(path, sizeof(path), "%s/%s-%u.bin", root, prefix, r->page);
        FILE *f = fopen(path, "wb");
        assert(f);
        assert(fwrite(frame, 1, sizeof(frame), f) == sizeof(frame));
        assert(!fclose(f));
    }
    r->page = 1;
}
int main(int argc, char **argv) {
    assert(argc == 5);
    ink_reader r;
    char error[160];
    assert(!ink_font_init());
    assert(!ink_math_init(argv[3], error, sizeof(error)));
    assert(!ink_reader_open(&r, argv[1], argv[2], render, NULL));
    bool epub = strstr(argv[1], ".epub") != NULL;
    if (epub) {
        assert(r.stats.chapters == 3 && r.stats.formulas == 2 && !r.stats.math_fallbacks);
        const char *names[] = {"Chapter One", "TOC Section", "Chapter Two"};
        unsigned char chapter[212];
        rewind(r.chapters);
        for (unsigned i = 0; i < 3; i++) {
            assert(fread(chapter, 1, sizeof(chapter), r.chapters) == sizeof(chapter));
            assert(!strcmp((char *)chapter + 20, names[i]));
        }
    }
    assert(images(&r) == (epub ? 6u : 4u));
    pages(&r, argv[2], argv[4]);
    if(epub) {
    ink_reader_home(&r);
    assert(!ink_reader_tap(&r, 48, 200));
    assert(!ink_reader_tap(&r, 48, 120));
    assert(r.view == INK_READER_PAGE);
    }
    assert(!ink_reader_rotate(&r, argv[1], argv[2], render, NULL));
    assert(images(&r) == (epub ? 6u : 4u));
    char prefix[128];
    snprintf(prefix, sizeof(prefix), "%s-landscape", argv[4]);
    pages(&r, argv[2], prefix);
    ink_reader_close(&r);
    ink_math_shutdown();
    puts("PASS: EPUB import/TOC chapters (H2 excluded), tables/math, PNG/JPEG/inline images, page rendering "
         "and landscape reflow");
}
