#pragma once

#include <stdint.h>
#include "lib/uthash.h"
#include "docid.h"

/**
 * 字段类型
 */
enum ts_field_type {
    FIELD_STRING, 
    FIELD_INT32
};

typedef struct {
    /**
     * 字段名称
     */
    uint8_t *name;

    /**
     * 字段值
     */
    void *value;

    /**
     * 值类型
     */
    enum ts_field_type value_type;

    /**
     * hash
     */
    UT_hash_handle hh;

    /**
     * 是否参与索引
     */
    uint8_t indexable;
} ts_field_t;

typedef struct {
    /**
     * 文档id，全局唯一
     */
    ts_docid_t doc_id;

    /**
     * 主键id
     */
    uint64_t pk;

    /**
     * 字段
     */
    ts_field_t *fields;
} ts_document_t;

// 字段相关api
ts_field_t *ts_field_new(uint8_t *name, enum ts_field_type type, void *value);
void ts_field_free(ts_field_t *field);

// 文档相关宏
#define ts_document_check(doc) (doc != NULL && doc->pk != 0 && doc->fields != NULL)

// 字段相关宏
#define ts_field_new_str(name, value) ts_field_new(name, FIELD_STRING, value)
#define ts_field_new_int(name, value) ts_field_new(name, FIELD_INT32, value)
#define ts_field_indexable(field) (field->indexable == 1)
#define ts_field_set_indexable(field) (field->indexable = 1)
#define ts_field_unset_indexable(field) (field->indexable = 0)

// 文档相关api
ts_document_t *ts_document_new(ts_docid_generator_t *generator);
int ts_document_field_add(ts_document_t *doc, ts_field_t *field, uint8_t force);
int ts_document_field_del(ts_document_t *doc, uint8_t *name);
void ts_document_free(ts_document_t *doc, uint8_t free_fields);
ts_document_t *ts_document_clone(ts_document_t *document);

#ifdef _DEBUG
#include <stdio.h>
#define ts_field_print(field) { \
    printf("=====field print=====\n"); \
    printf("name:%s\n", field->name); \
    switch(field->value_type) { \
        case FIELD_STRING: \
                           printf("type:string\n"); \
        printf("value:%s\n", (char*)field->value); \
        break; \
        case FIELD_INT32: \
                          printf("type:int32\n"); \
        printf("value:%d\n", *(int *)field->value); \
        break; \
    } \
    printf("indexable:%d\n\n", field->indexable); \
}

#define ts_document_print(document) { \
    printf("=====document print=====\n"); \
    printf("pk:%llu\n", document->pk); \
    printf("docid:%llu\n", document->doc_id); \
    ts_field_t *__f, *__tmp; \
    HASH_ITER(hh, document->fields, __f, __tmp) { \
        ts_field_print(__f); \
    } \
}
#endif

