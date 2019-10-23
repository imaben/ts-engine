#pragma once

#include <stdint.h>
#include "document.h"
#include "lib/utarray.h"

struct _ts_lookup_request {
    UT_array *tokens;
    uint32_t limit;
    int (*append)(struct _ts_lookup_request *request, char *token);
};

struct _ts_lookup_response {
    uint32_t total;
    UT_array *docs;
    int (*append)(struct _ts_lookup_response *response, ts_document_t *doc);
};

typedef struct _ts_lookup_request ts_lookup_request_t;
typedef struct _ts_lookup_response ts_lookup_response_t;

void ts_lookup_request_init(ts_lookup_request_t *request);
void ts_lookup_request_destroy(ts_lookup_request_t *request);
ts_lookup_request_t *ts_lookup_request_new();
void ts_lookup_request_free(ts_lookup_request_t *request);

void ts_lookup_response_init(ts_lookup_response_t *response);
void ts_lookup_response_destroy(ts_lookup_response_t *response);

