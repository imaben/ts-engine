#include <stdio.h>
#include <strings.h>
#include "bitmap.h"

#define TEST_COUNT 10240
int main() {
    ts_bitmap_t bm, clone;
    ts_bitmap_init(&bm);
    char *testdata = malloc(TEST_COUNT);
    bzero(testdata, TEST_COUNT);
    int i, r;

    for (i = 0; i < TEST_COUNT; i++) {
        if (i & 1) {
            ts_bitmap_set(&bm, i);
            testdata[i] = 1;
        }
    }

    // unset
    for (i = 0; i < TEST_COUNT; i++) {
        if ((i & 3) == 3) {
            ts_bitmap_unset(&bm, i);
            testdata[i] = 0;
        }
    }

    // check
    for (i = 0; i < TEST_COUNT; i++) {
        if (ts_bitmap_get(&bm, i) == testdata[i]) {
            printf("i:%d pass, value:%d\n", i, testdata[i]);
        } else {
            printf("check fail, i:%d\n", i);
            exit(1);
        }
    }

    ts_bitmap_clone(&bm, &clone);
    // check clone
    for (i = 0; i < TEST_COUNT; i++) {
        if (ts_bitmap_get(&clone, i) == testdata[i]) {
            printf("clone i:%d pass, value:%d\n", i, testdata[i]);
        } else {
            printf("clone check fail, i:%d\n", i);
            exit(1);
        }
    }

    ts_bitmap_destroy(&bm);
    ts_bitmap_destroy(&clone);
    free(testdata);
    return 0;
}
