#pragma once

#include "lib/uthash.h"
#include "lib/utarray.h"
#include "document.h"
#include "docid.h"

typedef struct {
    /**
     * 倒排词
     */ 
    uint8_t *term;

    /**
     * 关联文档列表
     */
    ts_docid_container_t doc_ids;

    uint64_t num_total;

    UT_hash_handle hh;
} ts_inverted_t;

ts_inverted_t *ts_inverted_new(uint8_t *term);
int ts_inverted_add_document(ts_inverted_t *inverted, ts_document_t *document);
void ts_inverted_free(ts_inverted_t *inverted);
