#include "intersect.h"
#include "docid.h"
#include "bitmap.h"
#include <stdio.h>

#define TEST_SIZE 32

int main() {
    ts_docid_generator_t generator;
    ts_docid_generator_init(&generator);

    ts_docid_container_t input1;
    ts_docid_container_init(&input1);
    ts_docid_container_t input2;
    ts_docid_container_init(&input2);
    ts_docid_container_t out;
    ts_docid_container_init(&out);

    ts_docid_t docids[TEST_SIZE];
    for (int i = 0; i < TEST_SIZE; i++) {
        docids[i] = generator.incr(&generator);
        if (i < 20) {
            ts_docid_container_append(&input1, &docids[i]);
        }
        if (i > 10) {
            ts_docid_container_append(&input2, &docids[i]);
        }
    }
    ts_bitmap_t deletion;
    uint32_t total;
    ts_bitmap_init(&deletion);
    ts_bitmap_set(&deletion, 15);
    ts_bitmap_set(&deletion, 16);
    ts_docid_container_t *ids_list[2] = { &input1, &input2 };
    ts_seek_intersect(ids_list, 2, &deletion, &out, 100, &total);

    ts_docid_t *tt;
    printf("===== input 1 =====\n");
    ts_docid_foreach(&input1, tt) {
        printf("id:%d\n", *tt);
    } 

    printf("===== input 2 =====\n");
    ts_docid_foreach(&input2, tt) {
        printf("id:%d\n", *tt);
    } 

    printf("===== intersect =====\n");
    ts_docid_foreach(&out, tt) {
        printf("id:%d\n", *tt);
    } 
    ts_bitmap_destroy(&deletion);
}
