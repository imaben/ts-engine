#include "queue.h"
#include "memory.h"
#include <assert.h>
#include <strings.h>

static struct _ts_queue_element *ts_queue_element_new(void *data) {
    struct _ts_queue_element *el = ts_calloc(1, sizeof(struct _ts_queue_element));
    assert(el != NULL);
    el->data = data;
    return el;
}

static void ts_queue_element_free(struct _ts_queue_element *el) {
    ts_free(el);
}

static int ts_queue_enqueue(ts_queue_t *queue, void *data, uint8_t force) {
    struct _ts_queue_element *el = ts_queue_element_new(data);
    queue->lock.lock(&queue->lock);
    if (queue->max_size > 0 && queue->size >= queue->max_size) {
        if (!force) {
            queue->lock.unlock(&queue->lock);
            return -1;
        }
    }
    if (queue->head == NULL) {
        queue->head = queue->tail = el;
    } else {
        queue->tail->next = el;
        queue->tail = el;
    }
    queue->size++;
    queue->lock.unlock(&queue->lock);
    return queue->size;
}

static int ts_queue_dequeue(ts_queue_t *queue, void **data) {
    queue->lock.lock(&queue->lock);
    if (queue->size > 0) {
        struct _ts_queue_element *el = queue->head;
        queue->head = el->next;
        *data = el->data;
        ts_queue_element_free(el);
        int size = queue->size--;
        queue->lock.unlock(&queue->lock);
        return size;
    }
    queue->lock.unlock(&queue->lock);
    return queue->size;
}

void ts_queue_init(ts_queue_t *queue, uint32_t max_size) {
    bzero(queue, sizeof(ts_queue_t));
    ts_spinlock_init(&queue->lock);
    queue->max_size = max_size;
    queue->enqueue = ts_queue_enqueue;
    queue->dequeue = ts_queue_dequeue;
}

void ts_queue_destroy(ts_queue_t *queue) {
    struct _ts_queue_element *el, *tmp;
    for (el = queue->head; el && ((tmp = el->next), 1); el = tmp) {
        ts_queue_element_free(el);
    }
    ts_spinlock_destroy(&queue->lock);
    bzero(queue, sizeof(ts_queue_t));
}

