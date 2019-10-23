#include "docid.h"
#include <stdio.h>

int main() {
    ts_docid_generator_t generator;
    ts_docid_generator_init(&generator);
    printf("===== generator =====\n");
    printf("generator:%d\n", generator.incr(&generator));
    printf("generator:%d\n", generator.incr(&generator));
    printf("generator:%d\n", generator.incr(&generator));

    int i;
    ts_docid_container_t container;
    ts_docid_container_init(&container);
    printf("===== container =====\n");
#define TEST_SIZE 32
    ts_docid_t docids[TEST_SIZE];
    for (i = 0; i < TEST_SIZE; i++) {
        docids[i] = generator.incr(&generator);
        ts_docid_container_append(&container, &docids[i]);
    }
    printf("total:%d, len:%d\n", container.total, container.len);
    struct _ts_docid_bucket *bkt, *tmp;
    ts_docid_bucket_foreach((ts_docid_container_t*)&container, bkt, tmp) {
        printf("bkt size:%d, offset:%d\n", bkt->size, bkt->offset);
    }
    ts_docid_t *tt;
    ts_docid_foreach(&container, tt) {
        printf("id:%d\n", *tt);
    } 
    printf("===== container iterator =====\n");
    ts_docid_container_iterator_t it;
    ts_docid_container_iterator_init(&container, &it);
    while ((tt = ts_docid_container_next(&it)) != NULL) {
        printf("id:%d\n", *tt);
    }
    ts_docid_container_destroy(&container);
    return 0;
}

