#pragma once

#include "inverted.h"
#include "lock.h"
#include "atomic.h"
#include "bitmap.h"
#include "docid.h"
#include "query.h"

#define SEGMENT_STATE_ACTIVE  0
#define SEGMENT_STATE_VALID   1
#define SEGMENT_STATE_MERGING 2

typedef struct {
    uint64_t pk;
    ts_docid_container_t doc_ids;
    UT_hash_handle hh;
} ts_pk_mapping_t;

struct _ts_segment {
    /**
     * 倒排文档
     */
    ts_inverted_t *docs;

    /**
     * 主键 -> docid的索引
     */
    ts_pk_mapping_t *pk_mapping;

    /**
     * 已删除doc
     */
    ts_bitmap_t deletion;
    ts_rwlock_t deletion_lock;

    /**
     * docid生成器
     */
    ts_docid_generator_t generator;

    /**
     * 当前段正在活跃的处理数
     */
    ts_atomic_uint32_t processes;

    /**
     * 总文档数
     */
    uint64_t num_total_docs;

    /**
     * 总删除数
     */
    uint64_t num_total_deleted;

    /**
     * 总倒排数
     */
    uint64_t num_total_inverted;

    struct _ts_segment *prev;
    struct _ts_segment *next;

    /**
     * 当前段状态
     */
    volatile uint8_t state;
};
typedef struct _ts_segment ts_segment_t;

ts_segment_t *ts_segment_new();
int ts_segment_add_document(ts_segment_t *seg, ts_document_t *doc);
int ts_segment_remove_document(ts_segment_t *seg, uint64_t pk);
int ts_segment_lookup(ts_segment_t *seg, UT_array *tokens, 
        uint32_t limit, ts_docid_container_t *ctn, uint32_t *total);
void ts_segment_free(ts_segment_t *seg, uint8_t free_inverted);

#ifdef _DEBUG
#include <stdio.h>
#define ts_segment_print(seg) { \
    printf("=====segment %p=====\n", seg); \
    printf("state:%d\n", seg->state); \
    printf("num_total_docs:%llu\n", seg->num_total_docs); \
    printf("num_total_deleted:%llu\n", seg->num_total_deleted); \
    printf("num_total_inverted:%llu\n", seg->num_total_inverted); \
}
#endif
