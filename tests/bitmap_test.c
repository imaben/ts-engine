#include <stdio.h>
#include <strings.h>
#include "bitmap.h"

int main() {
    ts_bitmap_t bm;
    ts_bitmap_init(&bm);
    int i;
    for (i = 0; i < 32; i++) {
        ts_bitmap_set(&bm, i);
    }
    for (i = 0; i < 32; i++) {
        printf("bit:%d = %d\n", i, ts_bitmap_get(&bm, i));
    }
    for (i = 0; i < 32; i++) {
        if (i % 2 == 0) {
            ts_bitmap_unset(&bm, i); 
        }
    }
    for (i = 0; i < 32; i++) {
        printf("unset bit:%d = %d\n", i, ts_bitmap_get(&bm, i));
    }
    ts_bitmap_destroy(&bm);
    return 0;
}
