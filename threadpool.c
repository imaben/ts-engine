#include "threadpool.h"
#include "config.h"
#include "memory.h"
#include <strings.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

static void ts_tpevent_init(struct _ts_tpevent *ev) {
    bzero(ev, sizeof(struct _ts_tpevent));
    pthread_mutex_init(&ev->mutex, NULL);
    pthread_cond_init(&ev->cond, NULL);
}

static void ts_tpevent_signal(struct _ts_tpevent *ev) {
    pthread_mutex_lock(&ev->mutex);
    ev->signal = 1;
    pthread_cond_signal(&ev->cond);
    pthread_mutex_unlock(&ev->mutex);
}

static void ts_tpevent_broadcast(struct _ts_tpevent *ev) {
    pthread_mutex_lock(&ev->mutex);
    ev->signal = 1;
    pthread_cond_broadcast(&ev->cond);
    pthread_mutex_unlock(&ev->mutex);
}

static void ts_tpevent_wait(struct _ts_tpevent *ev) {
    pthread_mutex_lock(&ev->mutex);
    while (!ev->signal) {
        pthread_cond_wait(&ev->cond, &ev->mutex);
    }
    ev->signal = 0;
    pthread_mutex_unlock(&ev->mutex);
}

static void ts_tpevent_destroy(struct _ts_tpevent *ev) {
    pthread_mutex_destroy(&ev->mutex);
    pthread_cond_destroy(&ev->cond);
}

static int ts_threadpool_push(ts_threadpool_t *tp, ts_task_t *task) {
    tp->queue.enqueue(&tp->queue, task, 0);
    ts_tpevent_signal(&tp->event);
    return 0;
}

static void *ts_threadpool_loop(void *arg) {
    ts_threadpool_t *tp = (ts_threadpool_t *)arg;
    ts_atomic_incr(&tp->thread_num_alive);
    ts_task_t *task;
    while (!tp->exit) {
        ts_tpevent_wait(&tp->event);
        if (tp->exit) {
            break;
        }
        while (tp->queue.dequeue(&tp->queue,  (void **)&task) > 0) {
            ts_task_callback cb = task->callback;
            void *retval = task->handler(task);
            if (cb) {
                cb(retval);
            }
        }
    }
    ts_atomic_decr(&tp->thread_num_alive);
    return NULL;
}

void ts_threadpool_init(ts_threadpool_t *tp, uint32_t thread_num, uint32_t queue_size) {
    assert(tp != NULL);
    assert(thread_num > 0);
    assert(queue_size >= thread_num);
    bzero(tp, sizeof(ts_threadpool_t));
    ts_tpevent_init(&tp->event);
    ts_queue_init(&tp->queue, thread_num * 2);
    tp->thread_num = thread_num;
    tp->push = ts_threadpool_push;
    tp->threads = ts_malloc(thread_num * sizeof(pthread_t));
    for (int i = 0; i < thread_num; i++) {
        pthread_create(&tp->threads[i], NULL, ts_threadpool_loop, (void *)tp);
    }
    while (tp->thread_num != tp->thread_num_alive);
}

void ts_threadpool_destroy(ts_threadpool_t *tp) {
    tp->exit = 1;
    while (tp->thread_num_alive) {
        ts_tpevent_broadcast(&tp->event);
        usleep(10000); // sleep 10ms
    }
    for (int i = 0; i < tp->thread_num; i++) {
        pthread_join(tp->threads[i], NULL);
    }
    ts_queue_destroy(&tp->queue);
    ts_free(tp->threads);
    ts_tpevent_destroy(&tp->event);
}

