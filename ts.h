#pragma once

#include <stdint.h>

#define ts_container_of(ptr, type, member) \
    (type *)((char *)ptr - (char *)&((type *)0)->member)

typedef struct {
    char *host;
    int port;
    uint8_t daemon;
} ts_setting_t;

