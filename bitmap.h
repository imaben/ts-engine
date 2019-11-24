#pragma once

#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include "memory.h"

#ifndef TS_BITMAP_PREALLOC
#define TS_BITMAP_PREALLOC 128LL
#endif

typedef struct {
    uint8_t *c;
    uint64_t l;
} ts_bitmap_t;

#define ts_bitmap_p(bm) ((ts_bitmap_t *)(bm))

#define ts_bitmap_init(bm) (bzero((void *)(bm), sizeof(ts_bitmap_t)))

#define ts_bitmap_destroy(bm) { \
    if (ts_bitmap_p(bm)->l > 0) ts_free(ts_bitmap_p(bm)->c); \
}
#define ts_bitmap_clone(src, dst) {\
    bzero((void *)(dst), sizeof(ts_bitmap_t)); \
    ts_bitmap_p(dst)->l = ts_bitmap_p(src)->l; \
    ts_bitmap_p(dst)->c = ts_malloc(ts_bitmap_p(src)->l); \
    assert(ts_bitmap_p(dst)->c != NULL); \
    memcpy(ts_bitmap_p(dst)->c, ts_bitmap_p(src)->c, ts_bitmap_p(src)->l); \
}

#define ts_bmp_align(s) ((s + TS_BITMAP_PREALLOC - 1) & ~(TS_BITMAP_PREALLOC - 1))
#define ts_bmp_dig(p) ((p) >> 0x3)
#define ts_bmp_mod(p) ((p) & 0x7)

#define ts_bitmap_reserve(bm, n) { \
    if ((n) > ((bm)->l)) {  \
        uint64_t __len = ts_bmp_align(n); \
        (bm)->c = ts_realloc((bm)->c, __len); \
        assert((bm)->c != NULL); \
        bzero((void *)((bm)->c + (bm)->l), __len - (bm)->l); \
        (bm)->l = __len; \
    } \
}

#define ts_bitmap_set(bm, p) { \
    ts_bitmap_reserve(ts_bitmap_p(bm), ts_bmp_dig(p) + 1); \
    ts_bitmap_p(bm)->c[ts_bmp_dig(p)] |= (1 << ts_bmp_mod(p)); \
}

#define ts_bitmap_unset(bm, p) { \
    if (ts_bmp_dig(p) < (bm)->l) ts_bitmap_p(bm)->c[ts_bmp_dig(p)] &=  ~(1 << (ts_bmp_mod(p))); \
}

#define ts_bitmap_get(bm, p) ((ts_bmp_dig(p) >= ((ts_bitmap_p(bm))->l) ? 0 : \
            ((ts_bitmap_p(bm)->c[ts_bmp_dig(p)]) >> ts_bmp_mod(p))) & 1)

