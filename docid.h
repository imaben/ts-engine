#pragma once

#include <stdint.h>

#ifndef BUCKET_SIZE
#define BUCKET_SIZE 8
#endif

typedef uint32_t ts_docid_t;

struct _ts_docid_generator {
    ts_docid_t start;
    uint32_t (*incr)(struct _ts_docid_generator *generator);
};

struct _ts_docid_bucket {
    uint32_t size;
    volatile uint32_t offset;
    struct _ts_docid_bucket *next;
    ts_docid_t *ids[0];
};

struct _ts_docid_container {
    struct _ts_docid_bucket *head;
    struct _ts_docid_bucket *tail;
    uint32_t alloc_size;
    volatile uint32_t len;
    volatile uint32_t total;
};

struct _ts_docid_container_iterator {
    struct _ts_docid_bucket *bkt;
    uint32_t offset;
};

typedef struct _ts_docid_generator ts_docid_generator_t;
typedef struct _ts_docid_container ts_docid_container_t;
typedef struct _ts_docid_container_iterator ts_docid_container_iterator_t;

void ts_docid_generator_init(ts_docid_generator_t *generator);
void ts_docid_container_init(ts_docid_container_t *container);
void ts_docid_container_append(ts_docid_container_t *container, ts_docid_t *docid);
void ts_docid_container_destroy(ts_docid_container_t *container);

#define ts_docid_bucket_foreach(c, b, t) \
    for ((b) = (c)->head; (b) && (((t) = (b)->next), 1); (b) = (t))

#define ts_docid_foreach(c, i) \
    for (struct _ts_docid_bucket *__tmp, *__bkt = ((ts_docid_container_t *)(c))->head; \
            __bkt && (((__tmp) = __bkt->next), 1);  \
            __bkt = __tmp) \
        for (uint32_t __o = 0; __o < __bkt->offset && ((i) = __bkt->ids[__o], 1); __o++)

#define ts_docid_container_init_batch(cc, l) { \
    for (int i = 0; i < l; i++) ts_docid_container_init(cc[i]); \
}

#define ts_docid_container_destroy_batch(cc, l) { \
    for (int i = 0; i < l; i++) ts_docid_container_destroy(cc[i]); \
}

#define ts_docid_container_reset(c) { \
    ts_docid_container_destroy(c); \
    ts_docid_container_init(c); \
}

#define ts_docid_container_copy(d, s) { \
    ts_docid_t *__doc_id; \
    ts_docid_foreach(s, __doc_id) { \
        ts_docid_container_append(d, __doc_id); \
    } \
}

#define ts_docid_cip(i) ((ts_docid_container_iterator_t *)i)
#define ts_docid_container_iterator_init(c, i) { \
    ts_docid_cip(i)->bkt = ((ts_docid_container_t *)c)->head; \
    ts_docid_cip(i)->offset = 0; \
}

#define ts_docid_container_next(i) ({ \
        if (ts_docid_cip(i)->bkt && \
                ts_docid_cip(i)->offset >= ts_docid_cip(i)->bkt->offset) { \
            ts_docid_cip(i)->bkt = ts_docid_cip(i)->bkt->next; \
            ts_docid_cip(i)->offset = 0; \
        } \
        ts_docid_cip(i)->bkt == NULL ? NULL : ts_docid_cip(i)->bkt->ids[ts_docid_cip(i)->offset++]; \
})

