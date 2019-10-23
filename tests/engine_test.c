#include "engine.h"
#include "segment.h"
#include <stdint.h>

static ts_document_t *make_document(ts_docid_generator_t *generator, uint32_t pk) {
    ts_document_t *document = ts_document_new(generator);
    document->pk = pk;
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
    ts_engine_t *engine = ts_engine_new();
    int retval;
    retval = engine->start(engine);
    printf("engine start retval:%d\n", retval);

    ts_document_t *doc = make_document(&engine->segment_active->generator, 1);
    retval = engine->add(engine, doc);
    printf("engine add retval:%d\n", retval);
    sleep(2);
    ts_segment_print(engine->segment_active);
    sleep(10000);
    return 0;
}

