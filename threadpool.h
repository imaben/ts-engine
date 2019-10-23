#pragma once

#include <pthread.h>
#include <stdint.h>
#include "task.h"
#include "queue.h"
#include "atomic.h"

typedef struct _ts_threadpool ts_threadpool_t;

struct _ts_tpevent {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    uint8_t signal;
};

struct _ts_threadpool {
    struct _ts_tpevent event;
    uint32_t thread_num;
    ts_atomic_uint32_t thread_num_alive;
    pthread_t *threads;
    ts_queue_t queue;
    int (*push)(ts_threadpool_t *tp, ts_task_t *task);
    volatile uint8_t exit;
};

void ts_threadpool_init(ts_threadpool_t *tp, uint32_t thread_num, uint32_t queue_size);
void ts_threadpool_destroy(ts_threadpool_t *tp);

