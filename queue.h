#pragma once

#include <stdint.h>
#include "lock.h"

struct _ts_queue_element {
    void *data;
    struct _ts_queue_element *next;
};

struct _ts_queue {
    struct _ts_queue_element *head;
    struct _ts_queue_element *tail;
    uint32_t size;
    uint32_t max_size;
    ts_spinlock_t lock;
    int (*enqueue)(struct _ts_queue *queue, void *data, uint8_t force);
    int (*dequeue)(struct _ts_queue *queue, void **data);
};

typedef struct _ts_queue ts_queue_t;

void ts_queue_init(ts_queue_t *queue, uint32_t max_size);
void ts_queue_destroy(ts_queue_t *queue);

