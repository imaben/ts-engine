#include "document.h"

int main() {
    ts_document_t *document = ts_document_new(NULL);
    document->doc_id = 1000;
    int ival1 = 1, ival2 = 2;
    ts_field_t *field1 = ts_field_new_str("key1", "value1");
    ts_field_t *field2 = ts_field_new_str("key2", "value2");
    ts_field_t *field3 = ts_field_new_int("key3", &ival1);
    ts_field_t *field4 = ts_field_new_int("key4", &ival2);

    ts_field_set_indexable(field4);

    ts_document_field_add(document, field1, 0);
    ts_document_field_add(document, field2, 0);
    ts_document_field_add(document, field3, 0);
    ts_document_field_add(document, field4, 0);

    ts_document_print(document);

    printf("===== del field=====\n");
    ts_document_field_del(document, "key1");
    ts_document_field_del(document, "key3");

    ts_document_print(document);

    ts_document_free(document, 1);
    return 0;
}
