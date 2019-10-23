#include "document.h"
#include "memory.h"
#include <assert.h>

ts_field_t *ts_field_new(uint8_t *name, enum ts_field_type type, void *value) {
    assert(name != NULL);
    assert(value != NULL);

    ts_field_t *field = ts_calloc(1, sizeof(ts_field_t));
    assert(field != NULL);

    field->name = ts_strdup(name);
    field->value_type = type;

    size_t cpylen;
    switch(type) {
        case FIELD_STRING:
            cpylen = strlen(value) + 1;
            break;
        case FIELD_INT32:
            cpylen = sizeof(int32_t);
    }	
    field->value = ts_calloc(1, cpylen);
    memcpy(field->value, value, cpylen);
    return field;
}

void ts_field_free(ts_field_t *field) {
    ts_free(field->name);
    ts_free(field->value);
    ts_free(field);
}

ts_document_t *ts_document_new(ts_docid_generator_t *generator) {
    ts_document_t *doc = ts_calloc(1, sizeof(ts_document_t));
    assert(doc != NULL);
    if (generator != NULL) {
        doc->doc_id = generator->incr(generator);
    }
    return doc;
}

int ts_document_field_add(ts_document_t *doc, ts_field_t *field, uint8_t force) {
    assert(doc != NULL);
    assert(field != NULL);
    ts_field_t *tmp= NULL;
    HASH_FIND_STR(doc->fields, field->name, tmp);
    if (tmp != NULL) {
        if (!force) {
            return -1;
        }
        ts_document_field_del(doc, field->name);
    }
    HASH_ADD_STR(doc->fields, name, field);
    return 0;
}

int ts_document_field_del(ts_document_t *document, uint8_t *name) {
    assert(document != NULL);
    assert(name != NULL);
    ts_field_t *field = NULL;
    HASH_FIND_STR(document->fields, name, field);
    if (field == NULL) {
        return -1;
    }
    HASH_DEL(document->fields, field);
    return 0;
}

void ts_document_free(ts_document_t *document, uint8_t free_fields) {
    ts_field_t *field, *tmp;
    HASH_ITER(hh, document->fields, field, tmp) {
        HASH_DEL(document->fields, field);
        if (free_fields) ts_field_free(field);
    }
    ts_free(document);
}

ts_document_t *ts_document_clone(ts_document_t *document) {
    ts_document_t *new = ts_document_new(NULL);
    new->doc_id = document->doc_id;
    new->pk = document->pk;
    ts_field_t *field, *tmp, *newfield;
    HASH_ITER(hh, document->fields, field, tmp) {
        newfield = ts_field_new(field->name, field->value_type, field->value);
        if (ts_field_indexable(field)) 
            ts_field_set_indexable(newfield);
        ts_document_field_add(new, newfield, 1);
    }
    return new;
}

