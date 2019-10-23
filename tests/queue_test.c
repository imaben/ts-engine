#include "queue.h"
#include <stdio.h>

int main() {
    ts_queue_t queue;
    ts_queue_init(&queue, 8);

#define TEST_SIZE 10

    int data[TEST_SIZE];
    int r;
    for (int i = 0; i < TEST_SIZE; i++) {
        data[i] = i;
        r = queue.enqueue(&queue, &data[i], 0);
        printf("enqueue %d result:%d\n", i, r);
    }
    r = queue.enqueue(&queue, &data[0], 1);
    printf("enqueue %d result:%d\n", 0, r);

    printf("queue size:%d\n", queue.size);

    int *i;
    while ((r = queue.dequeue(&queue, (void **)&i)) > 0) {
        printf("dequeue :%d, result:%d\n", *i, r);
    }

    ts_queue_destroy(&queue);
    return 0;
}

