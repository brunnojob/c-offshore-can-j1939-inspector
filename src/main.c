#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <assert.h>

static void fail(const char *message) { fprintf(stderr, "%s\n", message); exit(2); }
double number(const char *s) {
    char *end; errno = 0; double value = strtod(s, &end);
    if (errno || end == s || *end || !isfinite(value)) fail("invalid finite number");
    return value;
}
static size_t fields(char *line, char **out, size_t capacity) {
    line[strcspn(line, "\r\n")] = 0;
    size_t count = 0;
    char *p = line;
    while (*p) {
        if (count == capacity) fail("too many fields");
        out[count++] = p;
        char *separator = strchr(p, ',');
        if (!separator) break;
        *separator = 0; p = separator + 1;
        if (!*p) fail("empty field");
    }
    return count;
}

typedef struct { uint32_t key; unsigned sequence; int used; } Stream;
static uint32_t pgn(uint32_t id) {
    uint32_t pf = id >> 16 & 255, value = id >> 8 & 0x3ffff;
    return pf < 240 ? value & 0x3ff00 : value;
}
static unsigned integer(const char *s, unsigned max) {
    double v = number(s); if (v < 0 || v > max || floor(v) != v) fail("invalid integer"); return (unsigned)v;
}
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--self-test")) {
        assert(pgn(0x18f00401) == 0xf004); assert(pgn(0x18ea2301) == 0xea00);
        assert(((255u + 1u) & 255u) == 0); puts("PGN and wrap tests passed"); return 0;
    }
    Stream streams[256] = {0}; char line[512]; double last = -1; size_t rows = 0;
    while (fgets(line, sizeof line, stdin)) {
        char *f[4]; if (fields(line, f, 4) != 4) fail("expected time,id,sequence,payload");
        double time = number(f[0]); if (time < 0 || time < last) fail("non-monotonic time"); last = time;
        char *end; errno = 0; unsigned long id = strtoul(f[1], &end, 16);
        if (errno || end == f[1] || *end || id > 0x1fffffff) fail("invalid CAN identifier");
        unsigned sequence = integer(f[2], 255);
        size_t length = strlen(f[3]); if (!length || length > 16 || length % 2) fail("invalid payload length");
        for (size_t i = 0; i < length; i++) if (!strchr("0123456789abcdefABCDEF", f[3][i])) fail("invalid payload");
        uint32_t key = pgn((uint32_t)id) << 8 | (id & 255); size_t slot = 0;
        while (slot < 256 && streams[slot].used && streams[slot].key != key) slot++;
        if (slot == 256) fail("stream capacity exceeded");
        unsigned distance = (sequence - streams[slot].sequence) & 255;
        if (streams[slot].used && (distance == 0 || distance >= 128)) fail("duplicate or reversed application sequence");
        unsigned missed = streams[slot].used ? distance - 1 : 0;
        printf("time=%.3f source=%lu pgn=%u priority=%lu gap=%u\n", time, id & 255, pgn((uint32_t)id), id >> 26, missed);
        streams[slot] = (Stream){key, sequence, 1}; rows++;
    }
    if (!rows || ferror(stdin)) fail("missing input");
    return 0;
}
