#include <stdio.h>
#include <strings.h>
#include "bitmap.h"

int main() {
    ts_bitmap_t bm;
    ts_bitmap_init(&bm);
    int i;
    for (i = 0; i < 5120; i++) {
        ts_bitmap_set(&bm, i);
    }
    for (i = 0; i < 512; i++) {
        printf("bit:%d = %d\n", i, ts_bitmap_get(&bm, i));
    }
    for (i = 0; i < 512; i++) {
        if (i % 2 == 0) {
            ts_bitmap_unset(&bm, i); 
        }
    }
    for (i = 0; i < 513; i++) {
        printf("bit:%d = %d\n", i, ts_bitmap_get(&bm, i));
    }
    for (i = 0; i < 1024; i++) {
        ts_bitmap_unset(&bm, i); 
    }
    ts_bitmap_get(&bm, 2048); 
    ts_bitmap_destroy(&bm);
    return 0;
}
