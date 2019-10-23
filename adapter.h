#pragma once

#include "document.h"
#include "protocol/protocol.pb-c.h"
#include "net.h"
#include "query.h"

ts_document_t *ts_adapter_document(Document *document);
ts_net_packet_t *ts_adapter_response_operate_new(ErrorCode code, char *msg);
ts_lookup_request_t *ts_adapter_request(RequestLookup *lookup);
ts_net_packet_t *ts_adapter_response_lookup_new(ts_lookup_response_t *response, int code);

