#pragma once

#include <strings.h>

typedef struct _ts_task ts_task_t;

typedef void *(*ts_task_handler)(ts_task_t *task);
typedef void (*ts_task_callback)(void *argument);

struct _ts_task {
	ts_task_handler handler;
    ts_task_callback callback;
	void *argument;
};

#define ts_task_new() ({ \
        ts_task_t *__task = ts_calloc(1, sizeof(ts_task_t)); \
        assert(__task != NULL); \
        __task; \
})
#define ts_task_free(t) (ts_free(t))
#define ts_task_init(t) (bzero((void *)t, sizeof(ts_task_t)))
#define ts_task_destroy(t)

