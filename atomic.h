#pragma once

#include <stdint.h>

typedef volatile uint32_t ts_atomic_uint32_t;
typedef volatile uint64_t ts_atomic_uint64_t;

#define ts_atomic_incr(val) __sync_fetch_and_add(val, 1)
#define ts_atomic_decr(val) __sync_fetch_and_sub(val, 1)

