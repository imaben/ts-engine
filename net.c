#include "net.h"
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

ts_connection_t *ts_connection_new(int sockfd) {
    ts_connection_t *conn = calloc(1, sizeof(ts_connection_t));
    assert(conn != NULL);
    conn->sockfd = sockfd;
    conn->status = STATUS_WAIT_RECV;
    return conn;
}

void ts_connection_free(ts_connection_t *conn) {
    if (conn->sockfd) close(conn->sockfd);
    if (conn->packet) free(conn->packet);
    free(conn);
}

