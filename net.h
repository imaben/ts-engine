#pragma once

#include <stdint.h>

enum ts_conn_status {
    STATUS_WAIT_RECV = 0,
    STATUS_RECEIVING,
    STATUS_RECVED,
    STATUS_SENDING,
    STATUS_FINISH
};

typedef struct {
    uint32_t len;
    uint32_t offset;
    uint8_t data[0];
} ts_net_packet_t;

typedef struct {
    int sockfd;
    enum ts_conn_status status;
    ts_net_packet_t *packet;
} ts_connection_t;

ts_connection_t *ts_connection_new(int sockfd);
void ts_connection_free(ts_connection_t *conn);
