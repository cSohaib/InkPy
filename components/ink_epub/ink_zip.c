#include "ink_zip.h"
#include "ink_inflate.h"
#include <limits.h>
#include <string.h>
static uint32_t n(const unsigned char *p, unsigned count) {
    uint32_t v = 0;
    for (unsigned i = 0; i < count; i++)
        v |= (uint32_t)p[i] << (8 * i);
    return v;
}
int ink_zip_open(ink_zip *z, const char *path, void (*progress)(void)) {
    memset(z, 0, sizeof(*z));
    z->progress = progress;
    z->file = fopen(path, "rb");
    if (!z->file || fseek(z->file, 0, SEEK_END)) goto bad;
    long size = ftell(z->file);
    if (size < 22 || size > 0x7fffffffU) goto bad;
    z->size = (uint32_t)size;
    unsigned char b[22];
    long first = size > 65557 ? size - 65557 : 0;
    for (long at = size - 22; at >= first; at--) {
        if (fseek(z->file, at, SEEK_SET) || fread(b, 1, 22, z->file) != 22) goto bad;
        if (n(b, 4) != 0x06054b50 || at + 22 + n(b + 20, 2) != size) continue;
        if (n(b + 4, 2) || n(b + 6, 2) || n(b + 8, 2) != n(b + 10, 2)) goto bad;
        z->entries = n(b + 10, 2);
        z->central = n(b + 16, 4);
        if (z->entries == 65535 || z->central > z->size || n(b + 12, 4) > z->size - z->central) goto bad;
        return 0;
    }
bad:
    if (z->file) fclose(z->file);
    z->file = NULL;
    return -1;
}
int ink_zip_extract(ink_zip *z, const char *name, FILE *out) {
    unsigned char h[46];
    uint32_t at = z->central, compressed = 0, size = 0, crc = 0, local = 0;
    unsigned method = 0;
    for (unsigned i = 0; i < z->entries; i++) {
        if (at > z->size - 46 || fseek(z->file, at, SEEK_SET) || fread(h, 1, 46, z->file) != 46 ||
            n(h, 4) != 0x02014b50)
            return -1;
        unsigned names = n(h + 28, 2), extra = n(h + 30, 2), comment = n(h + 32, 2);
        char path[512];
        if ((uint64_t)at + 46 + names + extra + comment > z->size) return -1;
        if (names < sizeof(path) && fread(path, 1, names, z->file) == names) {
            path[names] = 0;
            if (!strcmp(path, name)) {
                if (n(h + 8, 2) & 1) return -1;
                method = n(h + 10, 2);
                compressed = n(h + 20, 4);
                size = n(h + 24, 4);
                crc = n(h + 16, 4);
                local = n(h + 42, 4);
                break;
            }
        }
        at += 46 + names + extra + comment;
        if (i + 1 == z->entries) return -1;
    }
    if (!z->entries || local > z->size - 30 || fseek(z->file, local, SEEK_SET) ||
        fread(h, 1, 30, z->file) != 30 || n(h, 4) != 0x04034b50)
        return -1;
    uint64_t start = (uint64_t)local + 30 + n(h + 26, 2) + n(h + 28, 2);
    if (start > z->size || compressed > z->size - start || (method != 0 && method != 8) || size > 0x7fffffffU)
        return -1;
    if (fseek(z->file, (long)start, SEEK_SET)) return -1;
    unsigned char input[4096], output[4096];
    size_t used = 0, have = 0;
    uint32_t written = 0, check = 0;
    int status = 0;
    void *inflate = method == 8 ? ink_inflate_open(0) : NULL;
    if (method == 8 && !inflate) return -1;
    while (status != 1) {
        if (used == have) {
            have = compressed > sizeof(input) ? sizeof(input) : compressed;
            used = 0;
            if (have && fread(input, 1, have, z->file) != have) goto bad;
            compressed -= (uint32_t)have;
        }
        size_t take = have - used, got = sizeof(output);
        if (method == 8)
            status = ink_inflate_step(inflate, input + used, &take, output, &got);
        else {
            got = take;
            memcpy(output, input + used, got);
            status = compressed ? 0 : 1;
        }
        used += take;
        if (status < 0 || got > size - written || fwrite(output, 1, got, out) != got) goto bad;
        written += (uint32_t)got;
        check = ink_crc32(check, output, got);
        if (z->progress) z->progress();
        if (!take && !got && status != 1) goto bad;
    }
    ink_inflate_close(inflate);
    return written == size && check == crc && !compressed && used == have && !fflush(out) ? 0 : -1;
bad:
    ink_inflate_close(inflate);
    return -1;
}
