#include "engine.h"
#include "memory.h"
#include "config.h"
#include "ts.h"
#include "lib/utlist.h"
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

static int ts_engine_start(ts_engine_t *engine);
static void ts_engine_shutdown(ts_engine_t *engine);
static int ts_engine_add_document(ts_engine_t *engine, ts_document_t *doc);
static int ts_engine_remove_document(ts_engine_t *engine, uint64_t pk);
static int ts_engine_lookup(ts_engine_t *engine, ts_lookup_request_t *request, 
        ts_lookup_response_t *response);

static void *ts_engine_add_document_handler(ts_task_t *task);
static void *ts_engine_remove_document_handler(ts_task_t *task);

ts_engine_t *ts_engine_new() {
    ts_engine_t *engine = ts_calloc(1, sizeof(ts_engine_t));
    assert(engine != NULL);
    ts_segment_t *seg = ts_segment_new();
    engine->segments = engine->segment_active = seg;

    ts_queue_init(&engine->input_queue, QUEUE_SIZE_INPUT);

    engine->merger = ts_merger_new(engine);

    engine->start = ts_engine_start;
    engine->add = ts_engine_add_document;
    engine->remove = ts_engine_remove_document;
    engine->lookup = ts_engine_lookup;
    return engine;
}

void ts_engine_free(ts_engine_t *engine) {
    ts_engine_shutdown(engine);
    ts_queue_destroy(&engine->input_queue);
    ts_merger_free(engine->merger);
    ts_free(engine);
}

static void *ts_engine_input_processor(void *argument) {
    ts_engine_t *engine = (ts_engine_t *)argument;
    ts_task_t *task;

    while (!engine->exit) {
        while (engine->input_queue.dequeue(&engine->input_queue,  (void **)&task) > 0) {
            ts_task_callback cb = task->callback;
            void *retval = task->handler(task);
            if (cb) {
                cb(retval);
            }
        }
        usleep(10 * 1000); // no task, sleep 10ms
    }
    return NULL;
}

static int ts_engine_start(ts_engine_t *engine) {
#define CHECK_RETVAL(r) { \
    if (r != 0) return -1; \
}
    // 启动输入处理线程
    int retval;
    retval = pthread_create(&engine->input_processor, NULL, 
            ts_engine_input_processor, (void *)engine);
    CHECK_RETVAL(retval);

    // 启动merger线程
    retval = engine->merger->start(engine);
    CHECK_RETVAL(retval);
    return 0;
}

static void ts_engine_shutdown(ts_engine_t *engine) {
    engine->exit = 1;
    pthread_join(engine->input_processor, NULL);
    pthread_join(engine->merger_thread, NULL);
}

static int ts_engine_add_document(ts_engine_t *engine, ts_document_t *doc) {
    assert(engine != NULL);
    assert(doc != NULL);  
    // 验证doc参数
    if (!ts_document_check(doc)) {
        return ERROR_CODE_INVALID_ARGUMENT;
    }

    ts_task_document_add_args_t *args = ts_task_document_add_args_new();
    args->engine = engine;
    args->doc = doc;
    ts_task_t *task = ts_task_new();
    task->handler = ts_engine_add_document_handler;
    task->argument = args;
    if (engine->input_queue.enqueue(&engine->input_queue, task, 0) < 0) {
        return ERROR_CODE_QUEUE_FULL;
    }
    return ERROR_CODE_SUCCESS;
}

static int ts_engine_remove_document(ts_engine_t *engine, uint64_t pk) {
    assert(engine != NULL);
    assert(pk != 0);

    ts_task_document_remove_args_t *args = ts_task_document_remove_args_new();
    args->engine = engine;
    args->pk = pk;
    ts_task_t *task = ts_task_new();
    task->handler = ts_engine_remove_document_handler;
    task->argument = args;
    if (engine->input_queue.enqueue(&engine->input_queue, task, 0) < 0) {
        return ERROR_CODE_QUEUE_FULL;
    }
    return ERROR_CODE_SUCCESS;
}

static void *ts_engine_add_document_handler(ts_task_t *task) {
    ts_task_document_add_args_t *args = task->argument;
    if (args->engine->segment_active->num_total_docs >= SEGMENT_MAX_DOC) {
        args->engine->segment_active->state++;
        ts_segment_t *seg = ts_segment_new();
        DL_PREPEND(args->engine->segments, seg);
        args->engine->segment_active = seg;
    }
    // 删除当前pk的所有旧doc
    ts_segment_t *seg = NULL;
    DL_FOREACH(args->engine->segments, seg) {
        ts_segment_remove_document(seg, args->doc->pk);
    }
    ts_segment_add_document(args->engine->segment_active, args->doc);
    ts_task_document_add_args_free(args);
    ts_task_free(task);
    return NULL;
}

static void *ts_engine_remove_document_handler(ts_task_t *task) {
    ts_task_document_remove_args_t *args = task->argument;
    ts_segment_t *seg = NULL;
    DL_FOREACH(args->engine->segments, seg) {
        ts_segment_remove_document(seg, args->pk);
    }
    if (args->engine->merger->merge_stage > 0) {
        utarray_push_back(args->engine->merger->merging_deletion, &args->pk);
    }
    ts_task_document_remove_args_free(args);
    ts_task_free(task);
    return NULL;
}

static int ts_engine_lookup(ts_engine_t *engine, ts_lookup_request_t *request, 
        ts_lookup_response_t *response) {
    if (request->limit > LOOKUP_MAX_SIZE) {
        return ERROR_CODE_INVALID_ARGUMENT;
    }

    ts_docid_t *docid;
    ts_document_t *doc, *newdoc;
    ts_segment_t *seg = NULL;
    uint32_t limit = request->limit;
    uint32_t total;
    DL_FOREACH(engine->segments, seg) {
        ts_atomic_incr(&seg->processes);
        request->limit = limit - utarray_len(response->docs);
        ts_docid_container_t container;
        ts_docid_container_init(&container);

        // 此处防止合并线程读写冲突，故对所有查到的doc做clone
        ts_segment_lookup(seg, request->tokens, request->limit, &container, &total);
        ts_docid_foreach(&container, docid) {
            doc = ts_container_of(docid, ts_document_t, doc_id);
            newdoc = ts_document_clone(doc);
            response->append(response, newdoc);
        }

        // 这个地方处理总数会不准确，后续优化
        response->total += total;

        ts_docid_container_destroy(&container);
        ts_atomic_decr(&seg->processes);
        if (utarray_len(response->docs) >= limit) {
            break;
        }
    }
    return ERROR_CODE_SUCCESS;
}

void ts_engine_print(ts_engine_t *engine) {
    printf("********* engine %p **********\n", engine);
    uint32_t count = 0;
    ts_segment_t *seg = NULL;
    DL_COUNT(engine->segments, seg, count); 
    printf("segment count:%d\n", count); 

    seg = NULL;
    DL_FOREACH(engine->segments, seg) {
        ts_segment_print(seg);
    }
}

