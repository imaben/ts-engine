#include "inverted.h"
#include "memory.h"
#include <assert.h>

ts_inverted_t *ts_inverted_new(uint8_t *term) {
    assert(term != NULL);
    ts_inverted_t *inverted = ts_calloc(1, sizeof(ts_inverted_t));
    assert(inverted != NULL);
    inverted->term = ts_strdup(term);
    ts_docid_container_init(&inverted->doc_ids);
    return inverted;
}

int ts_inverted_add_document(ts_inverted_t *inverted, ts_document_t *document) {
    ts_docid_container_append(&inverted->doc_ids, &document->doc_id);
    inverted->num_total++;
    return 0;
}

void ts_inverted_free(ts_inverted_t *inverted) {
    ts_docid_container_destroy(&inverted->doc_ids);
    ts_free(inverted);
}
