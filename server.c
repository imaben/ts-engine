#include "server.h"
#include "engine.h"
#include "ae/anet.h"
#include "ae/ae.h"
#include "protocol/protocol.pb-c.h"
#include "protocol/protocol.h"
#include "threadpool.h"
#include "config.h"
#include "net.h"
#include "adapter.h"
#include "memory.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

static struct _ts_server {
    ts_engine_t *engine;
    ts_setting_t *setting;
    ts_threadpool_t tp;
    /* 网络相关 */
    int fd;
    aeEventLoop *el;
} __ts_server, *ts_server = &__ts_server;

char ts_server_err[256];
static void ts_server_on_accept_handler(aeEventLoop *el, int fd, void *args, int mask);
static void ts_server_on_read_handler(aeEventLoop *el, int fd, void *args, int mask);
static void ts_server_on_write_handler(aeEventLoop *el, int fd, void *args, int mask);
static void *ts_server_request_process_handler(ts_task_t *task);

int ts_server_init(ts_setting_t *setting) {
    ts_server->setting = setting;
    ts_server->el = aeCreateEventLoop(1024);
    ts_server->engine = ts_engine_new();
    ts_threadpool_init(&ts_server->tp, LOOKUP_THREAD_NUM, LOOKUP_QUEUE_SIZE);
    return 0;
}

int ts_server_start() {
    ts_server->engine->start(ts_server->engine);
    ts_server->fd = anetTcpServer(ts_server_err, 
            ts_server->setting->port, ts_server->setting->host, 512);
    if (ts_server->fd == ANET_ERR) {
        return -1;
    }
    if (anetNonBlock(ts_server_err, ts_server->fd) != ANET_OK) {
        return -1;
    }
    if (anetEnableTcpNoDelay(ts_server_err, ts_server->fd) != ANET_OK) {
        return -1;
    }
    aeCreateFileEvent(ts_server->el, ts_server->fd, AE_READABLE, 
            ts_server_on_accept_handler, (void *)ts_server);
    aeMain(ts_server->el);

    close(ts_server->fd);
    aeDeleteEventLoop(ts_server->el);
    ts_engine_free(ts_server->engine);
    ts_threadpool_destroy(&ts_server->tp);
    return 0;
}

void ts_server_shutdown() {
    aeStop(ts_server->el);
}

static char ipbuf[46]; /* 兼容ipv6 */
static int portbuf;
static void ts_server_on_accept_handler(aeEventLoop *el, int fd, void *args, int mask) {
    int newfd = anetTcpAccept(ts_server_err, fd, ipbuf, sizeof(ipbuf), &portbuf);
    if (newfd == ANET_ERR) { 
        // todo add log
        return;
    }
    if (anetNonBlock(ts_server_err, fd) != AE_OK) {
        // todo add log
        close(newfd);
        return;
    }
    ts_connection_t *conn = ts_connection_new(newfd);
    if (aeCreateFileEvent(el, newfd, AE_READABLE, ts_server_on_read_handler, 
                (void *)conn) != AE_OK) {
        // todo add log
        ts_connection_free(conn);
    }
}

static void ts_server_on_read_handler(aeEventLoop *el, int fd, void *args, int mask) {
    ts_connection_t *conn = (ts_connection_t *)args;
    if (conn->status == STATUS_WAIT_RECV) {
        conn->status = STATUS_RECEIVING;
        int packet_len;
        if (read(conn->sockfd, (char *)&packet_len, sizeof(packet_len)) < 0) {
            // todo add log
            goto failed;
        }
        if (packet_len <= 0 || packet_len > MAX_PACKET_SIZE) {
            // todo add log
            goto failed;
        }
        ts_net_packet_t *packet = ts_calloc(1, sizeof(ts_net_packet_t) + packet_len);
        assert(packet != NULL);
        packet->len = packet_len;
        conn->packet = packet;
    }
    int nread = read(conn->sockfd, conn->packet->data + conn->packet->offset, 
            conn->packet->len - conn->packet->offset);
    if (nread < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        // todo add log
        goto failed;
    } else {
        conn->packet->offset += nread;
        if (conn->packet->len == conn->packet->offset) {
            conn->status = STATUS_RECVED;
            // 加入线程池处理队列
            ts_task_t *task = ts_task_new();
            task->handler = ts_server_request_process_handler;
            task->argument = (void *)conn;
            if (ts_server->tp.push(&ts_server->tp, task) < 0) {
                // todo add log
                goto failed;
            }
            aeDeleteFileEvent(el, fd, AE_READABLE);
        }
    }
    return;
failed:
    ts_connection_free(conn);
    aeDeleteFileEvent(el, fd, AE_READABLE);
}

static void ts_server_on_write_handler(aeEventLoop *el, int fd, void *args, int mask) {
    ts_connection_t *conn = (ts_connection_t *)args;
    if (conn->status == STATUS_SENDING) {
        ts_net_packet_t *packet = conn->packet;
        while (packet->len - packet->offset > 0) {
            int written = write(conn->sockfd, packet->data + packet->offset, 
                    packet->len - packet->offset);
            if (written < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return;
                }
                // todo add log
                goto failed;
            } 
            packet->offset += written;
        }
        if (packet->offset == packet->len) {
            conn->status = STATUS_FINISH;
        }
    }
    if (conn->status == STATUS_FINISH) {
        ts_connection_free(conn);
        aeDeleteFileEvent(el, fd, AE_WRITABLE);
    }
    return;
failed:
    ts_connection_free(conn);
    aeDeleteFileEvent(el, fd, AE_WRITABLE);
}

static void ts_server_request_process_add_document(ts_connection_t *conn) {
    ErrorCode error_code = ERROR_CODE__ERROR_CODE_SUCCESS;
    char *msg = "ok";
    RequestDocumentAdd *request = request_document_add__unpack(NULL,
            conn->packet->len - sizeof(protocol_action_t), 
            conn->packet->data + sizeof(protocol_action_t));
    if (request == NULL) {
        ts_connection_free(conn);
        return;
    }
    for (int i = 0; i < request->n_documents; i++) {
        // 没有字段的不处理
        if (request->documents[i]->n_fields == 0) {
            continue;
        }
        ts_document_t *newdoc = ts_adapter_document(request->documents[i]);
        int r;
        if ((r = ts_server->engine->add(ts_server->engine, newdoc)) < 0) {
            // todo add log
            ts_document_free(newdoc, 1);
            continue;
        }
    }
    request_document_add__free_unpacked(request, NULL);

    free(conn->packet);
    conn->packet = ts_adapter_response_operate_new(error_code, msg);
    conn->status = STATUS_SENDING;
    if (aeCreateFileEvent(ts_server->el, conn->sockfd, AE_WRITABLE, ts_server_on_write_handler, 
                (void *)conn) != AE_OK) {
        // todo add log
        ts_connection_free(conn);
    }
}

static void ts_server_request_process_remove_document(ts_connection_t *conn) {
    ErrorCode error_code = ERROR_CODE__ERROR_CODE_SUCCESS;
    char *msg = "ok";
    
    RequestDocumentRemove *request = request_document_remove__unpack(NULL,
            conn->packet->len - sizeof(protocol_action_t), 
            conn->packet->data + sizeof(protocol_action_t));
    if (request == NULL) {
        ts_connection_free(conn);
        return;
    }
    for (int i = 0; i < request->n_pks; i++) {
        if (request->pks[i] == 0) {
            continue;
        }
        if (ts_server->engine->remove(ts_server->engine, request->pks[i]) < 0) {
            // todo add log
            continue;
        }
    }
    request_document_remove__free_unpacked(request, NULL);
    free(conn->packet);
    conn->packet = ts_adapter_response_operate_new(error_code, msg);
    conn->status = STATUS_SENDING;
    if (aeCreateFileEvent(ts_server->el, conn->sockfd, AE_WRITABLE, ts_server_on_write_handler, 
                (void *)conn) != AE_OK) {
        // todo add log
        ts_connection_free(conn);
    }
}

static void ts_server_request_process_lookup(ts_connection_t *conn) {
    int code = 0;
    RequestLookup *request = request_lookup__unpack(NULL,
            conn->packet->len - sizeof(protocol_action_t), 
            conn->packet->data + sizeof(protocol_action_t));
    if (request == NULL) {
        ts_connection_free(conn);
        return;
    }
    ts_lookup_request_t *engine_request = ts_adapter_request(request);
    ts_lookup_response_t response;
    ts_lookup_response_init(&response);

    code = ts_server->engine->lookup(ts_server->engine, 
            engine_request, &response);
    free(conn->packet);
    conn->packet = ts_adapter_response_lookup_new(&response, code);
    conn->status = STATUS_SENDING;

    ts_lookup_response_destroy(&response);
    ts_lookup_request_free(engine_request);
    request_lookup__free_unpacked(request, NULL);

    if (aeCreateFileEvent(ts_server->el, conn->sockfd, AE_WRITABLE, ts_server_on_write_handler, 
                (void *)conn) != AE_OK) {
        // todo add log
        ts_connection_free(conn);
    }
}

static void *ts_server_request_process_handler(ts_task_t *task) {
    ts_connection_t *conn = (ts_connection_t *)task->argument;
    ts_task_free(task);
    if (conn->packet->len < sizeof(protocol_action_t)) {
        ts_connection_free(conn);
        return NULL;
    }

    protocol_action_t action;
    memcpy(&action, conn->packet->data, sizeof(protocol_action_t));

    switch (action) {
        case PROTOCOL_ACTION_DOCUMENT_ADD:
            ts_server_request_process_add_document(conn);
            break;
        case PROTOCOL_ACTION_DOCUMENT_REMOVE:
            ts_server_request_process_remove_document(conn);
            break;
        case PROTOCOL_ACTION_DOCUMENT_LOOKUP:
            ts_server_request_process_lookup(conn);
            break;
        default:
            printf("unknow action:%d\n", action);
            ts_connection_free(conn);
    }
    return NULL;
}

