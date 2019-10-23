#pragma once

#include "lib/utarray.h"
#include "segment.h"
#include "task.h"

typedef struct _ts_merger ts_merger_t;

typedef void (*ts_merge_proc_t)(ts_merger_t *merger);

struct _ts_merger {
    struct _ts_engine *engine;
    volatile uint32_t merge_stage;
    UT_array *merging_deletion;
    struct {
        ts_segment_t *mseg1;
        ts_segment_t *mseg2;
    } migrate_segs;
    struct {
        ts_segment_t *newseg;
        UT_array *deleted;
        ts_task_t task;
    } replace_segs;
    ts_task_t redelete;
    int (*start)(struct _ts_engine *engine);
};

ts_merger_t *ts_merger_new(struct _ts_engine *engine);
void ts_merger_free(ts_merger_t *merger);

