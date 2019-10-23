#include "docid.h"
#include "memory.h"
#include <assert.h>
#include <strings.h>

static uint32_t ts_docid_incr(ts_docid_generator_t *generator);

void ts_docid_generator_init(ts_docid_generator_t *generator) {
    generator->start = 1;
    generator->incr = ts_docid_incr;
}

static uint32_t ts_docid_incr(ts_docid_generator_t *generator) {
    return generator->start++;
}

static inline struct _ts_docid_bucket *ts_docid_bucket_new(uint32_t size) {
    size_t alloc = sizeof(struct _ts_docid_bucket) + sizeof(ts_docid_t *) * size;
    struct _ts_docid_bucket *bkt = ts_calloc(1, alloc);
    assert(bkt != NULL);
    bkt->size = size;
    return bkt;
}

static inline void ts_docid_container_reserve(ts_docid_container_t *container) {
    if (!container->len) {
        struct _ts_docid_bucket *bkt = ts_docid_bucket_new(container->alloc_size);
        container->head = bkt;
        container->tail = bkt;
        container->len++;
        return;
    }
    if ((container->tail->size - container->tail->offset) > 0) {
        return;
    }
    // expand 
    container->alloc_size <<= 1;
    struct _ts_docid_bucket *bkt = ts_docid_bucket_new(container->alloc_size);
    container->tail->next = bkt;
    container->tail = bkt;
    container->len++;
}

void ts_docid_container_init(ts_docid_container_t *container) {
    bzero(container, sizeof(*container));
    container->alloc_size = BUCKET_SIZE;
    ts_docid_container_reserve(container);
}

void inline ts_docid_container_append(ts_docid_container_t *container, ts_docid_t *docid) {
    ts_docid_container_reserve(container);
    container->tail->ids[container->tail->offset] = docid;
    container->tail->offset++;
    container->total++;
}

void ts_docid_container_destroy(ts_docid_container_t *container) {
    struct _ts_docid_bucket *bkt, *tmp;
    ts_docid_bucket_foreach(container, bkt, tmp) {
        ts_free(bkt);
    }
}

