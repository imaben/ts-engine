#include "intersect.h"

// 默认就是有序的
static void linear_intersect(ts_docid_container_t *input1, 
        ts_docid_container_t *input2, ts_bitmap_t *deletion,
        ts_docid_container_t *out) {
    if (input1->total == 0 || input2->total == 0) {
        return;
    }
    ts_docid_container_iterator_t it1;
    ts_docid_container_iterator_t it2;
    ts_docid_container_iterator_init(input1, &it1);
    ts_docid_container_iterator_init(input2, &it2);

    ts_docid_t *id1, *id2;
    id1 = ts_docid_container_next(&it1);
    id2 = ts_docid_container_next(&it2);
    do {
        if (*id1 < *id2) {
            id1 = ts_docid_container_next(&it1);
        } else if (*id1 > *id2) {
            id2 = ts_docid_container_next(&it2);
        } else {
            if (!ts_bitmap_get(deletion, *id1)) {
                ts_docid_container_append(out, id1);
            }
            id1 = ts_docid_container_next(&it1);
            id2 = ts_docid_container_next(&it2);
        }
    } while (id1 && id2);
}

int ts_seek_intersect(ts_docid_container_t **ids_list, uint32_t len, 
        ts_bitmap_t *deletion, ts_docid_container_t *out, uint32_t copied_len, uint32_t *total) {
    assert(ids_list != NULL);
    assert(len > 1);
    assert(deletion != NULL);
    assert(out != NULL);

    ts_docid_container_t tmp[2], *p = ids_list[0];
    ts_docid_container_t *cc[2] = { &tmp[0], &tmp[1] };

    ts_docid_container_init_batch(cc, 2);
    uint32_t i, idx;
    for (i = 1; i < len; i++) {
        idx = i & 1;
        ts_docid_container_reset(cc[idx]);
        linear_intersect(p, ids_list[i], deletion, cc[idx]);
        if (cc[idx]->total == 0) { // 交集为0，则直接跳出
            goto end;
        }
        p = cc[idx];
    }

    ts_docid_t *doc_id;
    uint32_t copied = 0;
    *total = p->total;
    ts_docid_foreach(p, doc_id) {
        ts_docid_container_append(out, doc_id);
        if ((++copied) >= copied_len) goto end;
    } 
end:
    ts_docid_container_destroy_batch(cc, 2);
    return 0;
}
