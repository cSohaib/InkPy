#include "ink_image.h"
#include "ink_inflate.h"
#include "vendor/tjpgd.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void fit(unsigned sw, unsigned sh, unsigned mw, unsigned mh, unsigned *w, unsigned *h) {
    *w = sw < mw ? sw : mw;
    *h = (unsigned)((uint64_t)sh * *w / sw);
    if (*h > mh) {
        *h = mh;
        *w = (unsigned)((uint64_t)sw * mh / sh);
    }
    if (!*w) *w = 1;
    if (!*h) *h = 1;
}
static void dot(uint8_t *bits, unsigned w, unsigned x, unsigned y, unsigned r, unsigned g, unsigned b,
                unsigned alpha) {
    static const unsigned char order[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
    unsigned gray = (77 * r + 150 * g + 29 * b) >> 8;
    gray = (gray * alpha + 255 * (255 - alpha)) / 255;
    if (gray < order[(y % 4) * 4 + x % 4] * 16u + 8u)
        bits[y * ((w + 7) / 8) + x / 8] |= (uint8_t)(128 >> (x % 8));
}
typedef struct {
    FILE *file;
    uint8_t *bits;
    unsigned sw, sh, w, h;
    void (*progress)(void);
} Jpeg;
static size_t jpeg_read(JDEC *d, uint8_t *buffer, size_t size) {
    Jpeg *j = d->device;
    if (buffer) return fread(buffer, 1, size, j->file);
    return fseek(j->file, (long)size, SEEK_CUR) ? 0 : size;
}
static int jpeg_write(JDEC *d, void *buffer, JRECT *rect) {
    Jpeg *j = d->device;
    unsigned stride = (rect->right - rect->left + 1) * 3;
    uint8_t *rgb = buffer;
    for (unsigned y = (unsigned)(((uint64_t)rect->top * j->h + j->sh - 1) / j->sh);
         y < j->h && (uint64_t)y * j->sh / j->h <= rect->bottom; y++) {
        unsigned sy = (unsigned)((uint64_t)y * j->sh / j->h);
        if (sy < rect->top || sy > rect->bottom) continue;
        for (unsigned x = (unsigned)(((uint64_t)rect->left * j->w + j->sw - 1) / j->sw);
             x < j->w && (uint64_t)x * j->sw / j->w <= rect->right; x++) {
            unsigned sx = (unsigned)((uint64_t)x * j->sw / j->w);
            if (sx < rect->left || sx > rect->right) continue;
            uint8_t *p = rgb + (sy - rect->top) * stride + (sx - rect->left) * 3;
            dot(j->bits, j->w, x, y, p[0], p[1], p[2], 255);
        }
    }
    if (j->progress && rect->left == 0) j->progress();
    return 1;
}
static uint32_t be(const unsigned char *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
static int paeth(int a, int b, int c) {
    int p = a + b - c, pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
    return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
}
static int png(FILE *f, unsigned mw, unsigned mh, uint8_t *bits, unsigned *w, unsigned *h,
               void (*progress)(void)) {
    unsigned char head[13], palette[768] = {0}, alpha[256];
    memset(alpha, 255, sizeof(alpha));
    unsigned sw = 0, sh = 0, depth = 0, type = 0, channels = 0, rowbytes = 0, pixelbytes = 0, y = 0, dy = 0,
             fill = 0;
    unsigned colors = 0;
    unsigned char *rows = NULL, *row = NULL, *previous = NULL, input[4096];
    void *z = NULL;
    int result = -1;
    bool ended = false;
    while (!ended) {
        unsigned char chunk[8];
        if (fread(chunk, 1, 8, f) != 8) goto done;
        uint32_t len = be(chunk);
        unsigned crc = ink_crc32(0, chunk + 4, 4);
        if (len > 0x7fffffff) goto done;
        if (!memcmp(chunk + 4, "IHDR", 4)) {
            if (sw || len != 13 || fread(head, 1, 13, f) != 13) goto done;
            crc = ink_crc32(crc, head, 13);
            sw = be(head);
            sh = be(head + 4);
            depth = head[8];
            type = head[9];
            channels = type == 0 || type == 3 ? 1 : type == 2 ? 3 : type == 4 ? 2 : type == 6 ? 4 : 0;
            if (!sw || sw > 8192 || !sh || sh > 32768 || !channels || head[10] || head[11] || head[12] ||
                (depth != 8 && !(channels == 1 && (depth == 1 || depth == 2 || depth == 4))))
                goto done;
            rowbytes = (sw * channels * depth + 7) / 8;
            pixelbytes = (channels * depth + 7) / 8;
            if (!pixelbytes) pixelbytes = 1;
            rows = calloc(2, 32769);
            if (!rows) goto done;
            row = rows;
            previous = rows + 32769;
            fit(sw, sh, mw, mh, w, h);
            z = ink_inflate_open(1);
            if (!z) goto done;
        } else if (!memcmp(chunk + 4, "PLTE", 4)) {
            if (!len || len > 768 || len % 3 || fread(palette, 1, len, f) != len) goto done;
            colors = len / 3;
            crc = ink_crc32(crc, palette, len);
        } else if (!memcmp(chunk + 4, "tRNS", 4) && type == 3) {
            if (len > 256 || fread(alpha, 1, len, f) != len) goto done;
            crc = ink_crc32(crc, alpha, len);
        } else if (!memcmp(chunk + 4, "IDAT", 4)) {
            if (!z) goto done;
            while (len) {
                unsigned take = len > sizeof(input) ? sizeof(input) : len;
                if (fread(input, 1, take, f) != take) goto done;
                crc = ink_crc32(crc, input, take);
                len -= take;
                size_t at = 0;
                while (at < take) {
                    size_t in = take - at, out = rowbytes + 1 - fill;
                    int status = ink_inflate_step(z, input + at, &in, row + fill, &out);
                    at += in;
                    fill += (unsigned)out;
                    if (status < 0 || (!in && !out)) goto done;
                    if (fill == rowbytes + 1) {
                        if (y >= sh || row[0] > 4) goto done;
                        for (unsigned i = 1; i <= rowbytes; i++) {
                            int a = i > pixelbytes ? row[i - pixelbytes] : 0, b = previous[i],
                                c = i > pixelbytes ? previous[i - pixelbytes] : 0;
                            row[i] = (unsigned char)(row[i] + (row[0] == 1   ? a
                                                               : row[0] == 2 ? b
                                                               : row[0] == 3 ? (a + b) / 2
                                                               : row[0] == 4 ? paeth(a, b, c)
                                                                             : 0));
                        }
                        if (dy < *h && (unsigned)((uint64_t)dy * sh / *h) == y) {
                            for (unsigned x = 0; x < *w; x++) {
                                unsigned sx = (unsigned)((uint64_t)x * sw / *w),
                                         v = depth == 8 ? row[1 + sx * channels]
                                                        : ((row[1 + sx * depth / 8] >>
                                                            (8 - depth - sx * depth % 8)) &
                                                           ((1u << depth) - 1));
                                unsigned r = v, g = v, b = v, a = 255;
                                if (type == 3) {
                                    if (v >= colors) goto done;
                                    r = palette[v * 3];
                                    g = palette[v * 3 + 1];
                                    b = palette[v * 3 + 2];
                                    a = alpha[v];
                                } else if (type == 0 && depth < 8)
                                    r = g = b = v * 255 / ((1u << depth) - 1);
                                else if (type == 2 || type == 6) {
                                    g = row[1 + sx * channels + 1];
                                    b = row[1 + sx * channels + 2];
                                    if (type == 6) a = row[1 + sx * channels + 3];
                                } else if (type == 4)
                                    a = row[1 + sx * 2 + 1];
                                dot(bits, *w, x, dy, r, g, b, a);
                            }
                            dy++;
                        }
                        unsigned char *swap = row;
                        row = previous;
                        previous = swap;
                        fill = 0;
                        y++;
                        if (progress && y % 16 == 0) progress();
                    }
                    if (status == 1) {
                        if (y != sh || fill || at != take || len) goto done;
                        ended = true;
                        break;
                    }
                }
            }
        } else {
            while (len) {
                unsigned take = len > sizeof(input) ? sizeof(input) : len;
                if (fread(input, 1, take, f) != take) goto done;
                crc = ink_crc32(crc, input, take);
                len -= take;
            }
        }
        unsigned char check[4];
        if (fread(check, 1, 4, f) != 4 || be(check) != crc) goto done;
    }
    result = dy == *h ? 0 : -1;
done:
    ink_inflate_close(z);
    free(rows);
    return result;
}
int ink_image_render(const char *path, unsigned mw, unsigned mh, uint8_t *bits, unsigned *w, unsigned *h,
                     void (*progress)(void)) {
    *w = *h = 0;
    if (!mw || mw > 800 || !mh || mh > 800 || (uint64_t)((mw + 7) / 8) * mh > 48000) return -1;
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    unsigned char sig[8];
    int result = -1;
    memset(bits, 0, 48000);
    if (fread(sig, 1, 8, f) != 8) goto done;
    if (!memcmp(sig, "\x89PNG\r\n\x1a\n", 8))
        result = png(f, mw, mh, bits, w, h, progress);
    else if (sig[0] == 255 && sig[1] == 216) {
        rewind(f);
        void *work = malloc(8192);
        JDEC d;
        Jpeg j = {.file = f, .bits = bits, .progress = progress};
        if (work && jd_prepare(&d, jpeg_read, work, 8192, &j) == JDR_OK && d.width && d.height) {
            j.sw = d.width;
            j.sh = d.height;
            fit(j.sw, j.sh, mw, mh, &j.w, &j.h);
            *w = j.w;
            *h = j.h;
            result = jd_decomp(&d, jpeg_write, 0) == JDR_OK ? 0 : -1;
        }
        free(work);
    }
done:
    fclose(f);
    if (result) *w = *h = 0;
    return result;
}
