#pragma once

#include "docid.h"
#include "bitmap.h"
#include <stdint.h>

int ts_seek_intersect(ts_docid_container_t **ids_list, uint32_t len, 
        ts_bitmap_t *deletion, ts_docid_container_t *out, uint32_t copied_len, uint32_t *total);
