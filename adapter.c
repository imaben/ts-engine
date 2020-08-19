#include "adapter.h"
#include "memory.h"

ts_document_t *ts_adapter_document(Document *document) {
    ts_document_t *newdoc = ts_document_new(NULL);
    for (int i = 0; i < document->n_fields; i++) {
        Field *field = document->fields[i];
        ts_field_t *newfield = field->type == 0 ?
                ts_field_new_str(field->term, field->valuestr) : ts_field_new_int(field->term, &field->valueint32);
        if (field->indexable) {
            ts_field_set_indexable(newfield);
        }
        ts_document_field_add(newdoc, newfield, 1);
    }
    newdoc->pk = document->pk;
    return newdoc;
}

inline ts_net_packet_t *ts_adapter_response_operate_new(ErrorCode code, char *msg) {
    ts_net_packet_t *packet = NULL;
    uint32_t len, packet_size;
    ResponseOperate response = RESPONSE_OPERATE__INIT;
    response.code = code;
    response.msg = msg;
    packet_size = response_operate__get_packed_size(&response);
    len = sizeof(uint32_t) + packet_size;
    packet = ts_malloc(sizeof(ts_net_packet_t) + len);
    assert(packet != NULL);
    packet->len = len;
    packet->offset = 0;
    memcpy(packet->data, &packet_size, sizeof(uint32_t));
    response_operate__pack(&response, packet->data + sizeof(uint32_t));
    return packet;
}

ts_lookup_request_t *ts_adapter_request(RequestLookup *lookup) {
    ts_lookup_request_t *request = ts_lookup_request_new();
    for (int i = 0; i < lookup->n_terms; i++) {
        request->append(request, lookup->terms[i]);
    }
    request->limit = lookup->limit;
    return request;
}

Field *ts_adapter_field2proto(ts_field_t *field) {
    Field *newfield = ts_calloc(1, sizeof(Field));
    field__init(newfield);
    newfield->type = field->value_type;
    newfield->term = field->name;
    if (field->value_type == FIELD_STRING) {
        newfield->valuestr = field->value;
    } else if (field->value_type == FIELD_INT32) {
        newfield->valueint32 = *((int32_t *)field->value);
    }
    newfield->indexable = field->indexable;
    return newfield;
}

Document *ts_adapter_document2proto(ts_document_t *document) {
    Document *newdoc = ts_malloc(sizeof(Document));
    document__init(newdoc);
    newdoc->pk = document->pk;
    newdoc->n_fields = HASH_COUNT(document->fields);
    newdoc->fields = ts_malloc(newdoc->n_fields * sizeof(Field *));
    ts_field_t *field, *tmp;
    int i = 0;
    HASH_ITER(hh, document->fields, field, tmp) {
        newdoc->fields[i++] = ts_adapter_field2proto(field);
    }
    return newdoc;
}

ts_net_packet_t *ts_adapter_response_lookup_new(ts_lookup_response_t *response, int code) {
    ts_net_packet_t *packet = NULL;
    uint32_t len, packet_size;
    ResponseLookup newresp = RESPONSE_LOOKUP__INIT;
    if (code < 0) {
        newresp.code = code;
    } else {
        newresp.code = ERROR_CODE__ERROR_CODE_SUCCESS;
    }
    newresp.n_documents = utarray_len(response->docs);
    newresp.total = response->total;
    // 此处内存分配太频率，后期需要引入内存池来进行优化
    newresp.documents = ts_calloc(newresp.n_documents, sizeof(Document *));
    ts_document_t **pp = NULL;
    int i = 0;
    while ((pp = (ts_document_t **)utarray_next(response->docs, pp))) {
        newresp.documents[i++] = ts_adapter_document2proto(*pp);
    }
    packet_size = response_lookup__get_packed_size(&newresp);
    len = sizeof(uint32_t) + packet_size;
    packet = ts_malloc(sizeof(ts_net_packet_t) + len);
    assert(packet != NULL);
    packet->len = len;
    packet->offset = 0;
    memcpy(packet->data, &packet_size, sizeof(uint32_t));
    response_lookup__pack(&newresp, packet->data + sizeof(uint32_t));

    // 此处再释放
    for (int i = 0; i < newresp.n_documents; i++) {
        for (int j = 0; j < newresp.documents[i]->n_fields; j++) {
            ts_free(newresp.documents[i]->fields[j]);
        }
        ts_free(newresp.documents[i]->fields);
    }
    ts_free(newresp.documents);
    return packet;
}

