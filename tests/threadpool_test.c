#include "threadpool.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static void *handler(ts_task_t *task) {
    printf("running on thread %lu\n", pthread_self());
    return NULL;
}

int main() {
    ts_threadpool_t threadpool;
    ts_threadpool_init(&threadpool, 10, 10);
    sleep(1);
    for (int i = 0; i < 16; i++) {
        ts_task_t *task = malloc(sizeof(ts_task_t));
        ts_task_init(task);
        task->handler = handler;
        threadpool.push(&threadpool, task);
    }
    sleep(1);
    ts_threadpool_destroy(&threadpool);
    return 0;
}
