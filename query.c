#include "query.h"
#include <strings.h>
#include <assert.h>

static int ts_lookup_request_append(ts_lookup_request_t *request, char *token) {
    assert(request != NULL);
    assert(token != NULL);
    uint8_t **pp = NULL;
    while ((pp = (uint8_t **)utarray_next(request->tokens, pp))) {
        if (strcmp(*pp, token) == 0) {
            return -1;
        }
    }
    utarray_push_back(request->tokens, &token);
    return 0;
}

void ts_lookup_request_init(ts_lookup_request_t *request) {
    assert(request != NULL);
    bzero(request, sizeof(ts_lookup_request_t));
    utarray_new(request->tokens, &ut_str_icd);
    request->append = ts_lookup_request_append;
}

void ts_lookup_request_destroy(ts_lookup_request_t *request) {
    assert(request != NULL);
    utarray_free(request->tokens);
}

ts_lookup_request_t *ts_lookup_request_new() {
    ts_lookup_request_t *request = malloc(sizeof(ts_lookup_request_t));
    ts_lookup_request_init(request);
    return request;
}

void ts_lookup_request_free(ts_lookup_request_t *request) {
    ts_lookup_request_destroy(request);
    free(request);
}

static int ts_lookup_response_append(
        ts_lookup_response_t *response, ts_document_t *doc) {
    utarray_push_back(response->docs, &doc);
    return 0;
}

void ts_lookup_response_init(ts_lookup_response_t *response) {
    assert(response != NULL);
    bzero(response, sizeof(ts_lookup_response_t));
    utarray_new(response->docs, &ut_ptr_icd);
    response->append = ts_lookup_response_append;
}

void ts_lookup_response_destroy(ts_lookup_response_t *response) {
    assert(response != NULL);

    ts_document_t **pp = NULL;
    while ((pp = (ts_document_t **)utarray_next(response->docs, pp))) {
        ts_document_free(*pp, 1);
    }
    utarray_free(response->docs);
}

