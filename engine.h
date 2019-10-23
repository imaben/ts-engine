#pragma once

#include <stdint.h>
#include <pthread.h>
#include "segment.h"
#include "atomic.h"
#include "queue.h"
#include "task.h"
#include "merger.h"
#include "lib/utlist.h"

#define ERROR_CODE_SUCCESS 0
#define ERROR_CODE_INVALID_ARGUMENT -1
#define ERROR_CODE_QUEUE_FULL -2

typedef struct _ts_engine ts_engine_t;
struct _ts_engine {
    /**
     * 所有的段，双向链表结构
     */
    ts_segment_t *segments;

    /**
     * 当前活动的段
     * 接受正在写入
     */
    ts_segment_t *segment_active;

    ts_queue_t input_queue;
    pthread_t input_processor;

    pthread_t lookup_processor;

    pthread_t merger_thread;
    ts_merger_t *merger;

    /**
     * 子线程退出标记位
     */
    volatile uint32_t exit;

    int (*start)(struct _ts_engine *engine);
    void (*shutdown)(struct _ts_engine *engine);

    int (*add)(struct _ts_engine *engine, ts_document_t *doc);
    int (*remove)(struct _ts_engine *engine, uint64_t pk);
    int (*lookup)(struct _ts_engine *engine, ts_lookup_request_t *request,
            ts_lookup_response_t *response);
};

ts_engine_t *ts_engine_new();
void ts_engine_free(ts_engine_t *engine);

typedef struct {
    ts_engine_t *engine;
    ts_document_t *doc;
} ts_task_document_add_args_t;

typedef struct {
    ts_engine_t *engine;
    uint64_t pk;
} ts_task_document_remove_args_t;

#define ts_task_document_add_args_new() ({ \
        ts_task_document_add_args_t *__args = \
            ts_calloc(1, sizeof(ts_task_document_add_args_t)); \
        assert(__args != NULL); \
        __args; \
})
#define ts_task_document_add_args_free(a) (ts_free(a))

#define ts_task_document_remove_args_new() ({ \
        ts_task_document_remove_args_t *__args = \
            ts_calloc(1, sizeof(ts_task_document_remove_args_t)); \
        assert(__args != NULL); \
        __args; \
})
#define ts_task_document_remove_args_free(a) (ts_free(a))

#ifdef _DEBUG
void ts_engine_print(ts_engine_t *engine);
#endif
