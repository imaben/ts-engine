#include "segment.h"
#include "ts.h"
#include "lib/utarray.h"
#include <stdio.h>
#include <strings.h>

#define info(fmt, ...) do {             \
    fprintf(stdout, fmt, ##__VA_ARGS__); \
} while (0)

static ts_document_t *make_document(ts_docid_generator_t *generator) {
    ts_document_t *document = ts_document_new(generator);
    int ival1 = 1, ival2 = 2;
    ts_field_t *field1 = ts_field_new_str("key1", "value1");
    ts_field_t *field2 = ts_field_new_str("key2", "value2");
    ts_field_t *field3 = ts_field_new_int("key3", &ival1);
    ts_field_t *field4 = ts_field_new_int("key4", &ival2);

    ts_field_set_indexable(field2);
    ts_field_set_indexable(field3);
    ts_field_set_indexable(field4);

    ts_document_field_add(document, field1, 0);
    ts_document_field_add(document, field2, 0);
    ts_document_field_add(document, field3, 0);
    ts_document_field_add(document, field4, 0);

    return document;
}

int main() {
    ts_segment_t *seg = ts_segment_new();
    ts_document_t *doc = make_document(&seg->generator);
    doc->pk = 1;
    int retval = ts_segment_add_document(seg, doc);
    info("retval:%d\n", retval);
    ts_segment_print(seg);

    printf("===== lookup =====\n");
    ts_docid_container_t container;
    ts_docid_container_init(&container);
    char *k1 = "key2", *k2 = "key3";
    UT_array *tokens;
    utarray_new(tokens, &ut_str_icd);
    utarray_push_back(tokens, &k1);
    utarray_push_back(tokens, &k2);
    uint32_t total;
    ts_segment_lookup(seg, tokens, 10, &container, &total);
    printf("lookup, count:%d\n", container.total);
    utarray_free(tokens);
    ts_docid_t *docid;
    ts_document_t *lookup_doc;
    for (int i = 0; i < container.total; i++) {
        ts_docid_foreach(&container, docid) {
            lookup_doc = ts_container_of(docid, ts_document_t, doc_id);
            ts_document_print(lookup_doc);
        }
    }

    ts_docid_container_destroy(&container);
    ts_segment_free(seg, 1);
    return 0;
}

