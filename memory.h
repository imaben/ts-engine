#pragma once

/**
 * 内存操作的二次封装，方便后续做内存池管理
 */
#include <stdlib.h>

#define ts_memory_init()

#define ts_malloc  malloc
#define ts_calloc  calloc
#define ts_realloc realloc
#define ts_free    free
#define ts_strdup  strdup

