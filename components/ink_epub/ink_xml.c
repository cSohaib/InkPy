#include "ink_xml.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
void ink_xml_entities(char *s) {
    char *out = s;
    for (char *p = s; *p;) {
        if (*p == '&') {
            char *end = strchr(p, ';');
            unsigned cp = 0;
            if (end && end - p < 20) {
                if (p[1] == '#') {
                    char *tail;
                    cp = (unsigned)strtoul(p + 2 + (p[2] == 'x' || p[2] == 'X'), &tail,
                                           p[2] == 'x' || p[2] == 'X' ? 16 : 10);
                    if (tail != end) cp = 0;
                }
                const struct {
                    const char *name;
                    unsigned cp;
                } names[] = {{"amp", 38},   {"lt", 60},      {"gt", 62},      {"quot", 34},    {"apos", 39},
                             {"nbsp", 160}, {"mdash", 8212}, {"ndash", 8211}, {"hellip", 8230}};
                for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); i++)
                    if ((size_t)(end - p - 1) == strlen(names[i].name) &&
                        !memcmp(p + 1, names[i].name, end - p - 1))
                        cp = names[i].cp;
                if (cp >= 32 && cp <= 0x10ffff && !(cp >= 0xd800 && cp <= 0xdfff)) {
                    if (cp < 128)
                        *out++ = (char)cp;
                    else if (cp < 2048) {
                        *out++ = (char)(192 | (cp >> 6));
                        *out++ = (char)(128 | (cp & 63));
                    } else if (cp < 65536) {
                        *out++ = (char)(224 | (cp >> 12));
                        *out++ = (char)(128 | ((cp >> 6) & 63));
                        *out++ = (char)(128 | (cp & 63));
                    } else {
                        *out++ = (char)(240 | (cp >> 18));
                        *out++ = (char)(128 | ((cp >> 12) & 63));
                        *out++ = (char)(128 | ((cp >> 6) & 63));
                        *out++ = (char)(128 | (cp & 63));
                    }
                    p = end + 1;
                    continue;
                }
            }
        }
        *out++ = *p++;
    }
    *out = 0;
}
int ink_xml_next(ink_xml *x) {
restart:;
    int c = fgetc(x->file);
    if (c == EOF) return ferror(x->file) ? -1 : 0;
    if (c != '<') {
        size_t n = 0;
        do {
            x->text[n++] = (char)c;
            if (n == sizeof(x->text) - 1) break;
            c = fgetc(x->file);
        } while (c != EOF && c != '<');
        if (c == '<') ungetc(c, x->file);
        x->text[n] = 0;
        return 2;
    }
    c = fgetc(x->file);
    if (c == '!' || c == '?') {
        int quote = 0, brackets = 0;
        unsigned tail = 0;
        bool comment = false;
        if (c == '!') {
            int a = fgetc(x->file), b = fgetc(x->file);
            comment = a == '-' && b == '-';
            if (!comment) {
                if (b != EOF) ungetc(b, x->file);
                if (a != EOF) ungetc(a, x->file);
            }
        }
        while ((c = fgetc(x->file)) != EOF) {
            if (comment) {
                tail = ((tail << 8) | (unsigned)c) & 0xffffff;
                if (tail == 0x2d2d3e) break;
                continue;
            }
            if (quote) {
                if (c == quote) quote = 0;
                continue;
            }
            if (c == '\'' || c == '"')
                quote = c;
            else if (c == '[')
                brackets++;
            else if (c == ']' && brackets)
                brackets--;
            else if (c == '>' && !brackets)
                break;
        }
        if (c == EOF) return -1;
        goto restart;
    }
    x->closing = c == '/';
    if (x->closing) c = fgetc(x->file);
    size_t n = 0;
    while (c != EOF && !isspace(c) && c != '>' && c != '/') {
        if (n + 1 >= sizeof(x->name)) return -1;
        x->name[n++] = (char)c;
        c = fgetc(x->file);
    }
    x->name[n] = 0;
    char *local = strrchr(x->name, ':');
    if (local) memmove(x->name, local + 1, strlen(local + 1) + 1);
    n = 0;
    int quote = 0;
    while (c != EOF) {
        if (c == '>' && !quote) break;
        if(n+1<sizeof(x->attributes))x->attributes[n++]=(char)c;
        if (quote) {
            if (c == quote) quote = 0;
        } else if (c == '\'' || c == '"')
            quote = c;
        c = fgetc(x->file);
    }
    if (c == EOF || quote || !x->name[0]) return -1;
    while (n && isspace((unsigned char)x->attributes[n - 1]))
        n--;
    x->empty = n && x->attributes[n - 1] == '/';
    if (x->empty) n--;
    x->attributes[n] = 0;
    return 1;
}
void ink_xml_attr(const ink_xml *x, const char *key, char *value, size_t cap) {
    value[0] = 0;
    const char *p = x->attributes;
    while (*p) {
        while (isspace((unsigned char)*p))
            p++;
        const char *name = p;
        while (*p && !isspace((unsigned char)*p) && *p != '=')
            p++;
        size_t n = p - name;
        while (isspace((unsigned char)*p))
            p++;
        if (*p != '=') return;
        p++;
        while (isspace((unsigned char)*p))
            p++;
        int quote = *p;
        if (quote != '\'' && quote != '"') return;
        p++;
        const char *start = p;
        while (*p && *p != quote)
            p++;
        const char *local=name;
        for(size_t i=0;i<n;i++)if(name[i]==':')local=name+i+1;
        const char *wanted=strrchr(key,':');wanted=wanted?wanted+1:key;
        if ((strlen(key)==n&&!memcmp(name,key,n))||
            (strlen(wanted)==(size_t)(name+n-local)&&!memcmp(local,wanted,name+n-local))) {
            size_t len = p - start;
            if (len >= cap) len = cap - 1;
            memcpy(value, start, len);
            value[len] = 0;
            ink_xml_entities(value);
            return;
        }
        if (*p) p++;
    }
}
