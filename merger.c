#include "ts.h"
#include "engine.h"
#include "config.h"
#include "lib/utlist.h"
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

UT_icd uint64_icd = {sizeof(uint64_t), NULL, NULL, NULL };

#define deleted_radio(s) \
    (((ts_segment_t *)s)->num_total_deleted / ((ts_segment_t *)s)->num_total_docs)

static void *ts_merger_replace_segs_handler(ts_task_t *task);
static int ts_merger_get_segments(ts_segment_t *segments, 
        ts_segment_t **mseg1, ts_segment_t **mseg2);
static void ts_segment_migrate(ts_segment_t *old, 
        ts_segment_t *new, UT_array *deleted);
static void *ts_merger_redelete_handler(ts_task_t *task);

static void ts_merge_proc_check(ts_merger_t *merger);
static void ts_merge_proc_migrate(ts_merger_t *merger);
static void ts_merge_proc_release(ts_merger_t *merger);
static void ts_merge_proc_redelete(ts_merger_t *merger);
static void ts_merge_proc_reset(ts_merger_t *merger);
static ts_merge_proc_t ts_merge_procs[] = {
    ts_merge_proc_check,
    ts_merge_proc_migrate,
    ts_merge_proc_release,
    ts_merge_proc_redelete,
    ts_merge_proc_reset,
    NULL
};

static void ts_merge_proc_check(ts_merger_t *merger) {
    if (merger->merge_stage != 0) {
        return;
    }

    ts_segment_t *mseg1 = NULL, *mseg2 = NULL;
    if (ts_merger_get_segments(merger->engine->segments, &mseg1, &mseg2) < 0) {
        return;
    }
    merger->migrate_segs.mseg1 = mseg1;
    merger->migrate_segs.mseg2 = mseg2;
    merger->merge_stage = 1;
}

static void ts_merge_proc_migrate(ts_merger_t *merger) {
    if (merger->merge_stage != 1) {
        return;
    }

    merger->replace_segs.newseg = ts_segment_new();
    utarray_new(merger->replace_segs.deleted, &ut_ptr_icd);
    ts_segment_migrate(merger->migrate_segs.mseg1, 
            merger->replace_segs.newseg, merger->replace_segs.deleted);
    ts_segment_migrate(merger->migrate_segs.mseg2, 
            merger->replace_segs.newseg, merger->replace_segs.deleted);
    ts_task_init(&merger->replace_segs.task);
    merger->replace_segs.task.handler = ts_merger_replace_segs_handler;
    merger->replace_segs.task.argument = (void *)merger;
    merger->engine->input_queue.enqueue(&merger->engine->input_queue, 
            (void *)&merger->replace_segs.task, 1);
    merger->merge_stage = 2;
}

static void *ts_merger_replace_segs_handler(ts_task_t *task) {
    ts_merger_t *merger = (ts_merger_t *)task->argument;
    DL_DELETE(merger->engine->segments, merger->migrate_segs.mseg1);
    DL_DELETE(merger->engine->segments, merger->migrate_segs.mseg2);
    // 如果新合并出来的段文档数为0，则不再加入段列表
    if (merger->replace_segs.newseg->num_total_docs > 0) {
        merger->replace_segs.newseg->state++;
        DL_PREPEND(merger->engine->segments, merger->replace_segs.newseg);
    }
    merger->merge_stage = 3;
    return NULL;
}

static void ts_merge_proc_release(ts_merger_t *merger) {
    if (merger->merge_stage != 3) {
        return;
    }
    if (merger->migrate_segs.mseg1->processes == 0) {
        ts_segment_free(merger->migrate_segs.mseg1, 1);
        merger->migrate_segs.mseg1 = NULL;
    }
    if (merger->migrate_segs.mseg2->processes == 0) {
        ts_segment_free(merger->migrate_segs.mseg2, 1);
        merger->migrate_segs.mseg2 = NULL;
    }
    if (merger->migrate_segs.mseg1 == NULL && 
            merger->migrate_segs.mseg2 == NULL) {
        // 清理合并过程中被删除的文档
        ts_document_t **doc = NULL;
        while ((doc = (ts_document_t **)
                    utarray_next(merger->replace_segs.deleted, doc))) {
            ts_document_free(*doc, 1);
        }
        if (merger->replace_segs.newseg->num_total_docs > 0) {
            merger->merge_stage = 4;
        } else {
            // 如果新合并出来的段文档数为0，则直接跳到reset
            ts_segment_free(merger->replace_segs.newseg, 1);
            merger->merge_stage = 6;
        }
    }
}

static void *ts_merger_redelete_handler(ts_task_t *task) {
    ts_merger_t *merger = (ts_merger_t *)task->argument;
    uint64_t *pk = NULL;
    while ((pk = (uint64_t *)utarray_next(merger->merging_deletion, pk))) {
        ts_segment_remove_document(merger->replace_segs.newseg, *pk);
    }
    merger->merge_stage = 6;
    return NULL;
}

static void ts_merge_proc_redelete(ts_merger_t *merger) {
    if (merger->merge_stage != 4) {
        return;
    }
    ts_task_init(&merger->redelete);
    merger->redelete.handler = ts_merger_redelete_handler;
    merger->redelete.argument = (void *)merger;
    merger->engine->input_queue.enqueue(&merger->engine->input_queue, 
            (void *)&merger->redelete, 1);
    merger->merge_stage = 5;
}

static void ts_merge_proc_reset(ts_merger_t *merger) {
    if (merger->merge_stage != 6) {
        return;
    }
    // 清理所有的资源
    utarray_free(merger->replace_segs.deleted);
    merger->merge_stage = 0;
}

static void ts_segment_migrate(ts_segment_t *old, ts_segment_t *new, UT_array *deleted) {
    ts_bitmap_t deletion;
    old->deletion_lock.rdlock(&old->deletion_lock);
    ts_bitmap_clone(&old->deletion, &deletion);
    old->deletion_lock.unlock(&old->deletion_lock);

    ts_pk_mapping_t *mp, *tmp;
    ts_docid_t *doc_id;
    ts_document_t *doc;
    HASH_ITER(hh, old->pk_mapping, mp, tmp) {
        ts_docid_foreach(&mp->doc_ids, doc_id) {
            doc = ts_container_of(doc_id, ts_document_t, doc_id);
            if (ts_bitmap_get(&deletion, *doc_id)) {
                utarray_push_back(deleted, &doc);
                continue;
            }
            ts_segment_add_document(new, doc);
        }
    }
    ts_bitmap_destroy(&deletion);
}

/**
 * 获取需要合并的两个段
 * 目前的策略很简单，可能会有合并后超出segment最大doc数的情况
 * 后续优化
 */
static int ts_merger_get_segments(ts_segment_t *segments, 
        ts_segment_t **mseg1, ts_segment_t **mseg2) {
    ts_segment_t *seg;

    uint32_t count;
    DL_COUNT(segments, seg, count);
    if (count < 3) {
        return -1;
    }
    uint8_t found = 0;
    DL_FOREACH(segments, seg) {
        // 当前活动段不参与合并
        if (seg->state == SEGMENT_STATE_ACTIVE) {
            continue;
        }
        if (*mseg1 == NULL) {
            *mseg1 = seg;
            continue;
        }
        if (deleted_radio(seg) >= deleted_radio(*mseg1)) {
            *mseg2 = *mseg1;
            *mseg1 = seg;
            found = 1;
        }
    }
    if (found && deleted_radio(*mseg1) >= MERGE_DELETED_RATIO) {
        return 0;
    }
    return -1;
}

static void *ts_merger_start_proc(void *arg) {
    ts_engine_t *engine = (ts_engine_t *)arg;
    ts_merger_t *merger = engine->merger;
    int idx;
    ts_merge_proc_t proc;
    while (!engine->exit) {
        sleep(1);
#ifdef _DEBUG
        ts_engine_print(engine);
#endif
        idx = 0;
        while ((proc = ts_merge_procs[idx++]) != NULL) {
            proc(merger);
        }
    }
    return NULL;
}

static int ts_merger_start(ts_engine_t *engine) {
    assert(engine != NULL);
    return pthread_create(&engine->merger_thread, NULL, 
            ts_merger_start_proc, (void *)engine);
}

ts_merger_t *ts_merger_new(ts_engine_t *engine) {
    ts_merger_t *merger = ts_calloc(1, sizeof(ts_merger_t));
    merger->engine = engine;
    utarray_new(merger->merging_deletion, &uint64_icd);
    merger->start = ts_merger_start;
    return merger;
}

void ts_merger_free(ts_merger_t *merger) {
    utarray_free(merger->merging_deletion);
    ts_free(merger);
}

