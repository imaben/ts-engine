#include "segment.h"
#include "memory.h"
#include "intersect.h"
#include <assert.h>
#include <strings.h>

ts_segment_t *ts_segment_new() {
    ts_segment_t *seg = ts_calloc(1, sizeof(ts_segment_t));
    assert(seg != NULL);
    ts_bitmap_init(&seg->deletion);
    ts_docid_generator_init(&seg->generator);
    ts_rwlock_init(&seg->deletion_lock);
    return seg;
}

static inline ts_inverted_t *ts_inverted_get_or_new(ts_segment_t *seg, uint8_t *term) {
    ts_inverted_t *inverted;
    HASH_FIND_STR(seg->docs, term, inverted);
    if (inverted == NULL) {
        inverted = ts_inverted_new(term);
        HASH_ADD_STR(seg->docs, term, inverted);
        seg->num_total_inverted++;
    }
    return inverted;
}

static inline ts_pk_mapping_t *ts_pk_mapping_get_or_new(ts_segment_t *seg, uint64_t pk) {
    ts_pk_mapping_t *mp;
    HASH_FIND(hh, seg->pk_mapping, &pk, sizeof(uint64_t), mp);
    if (mp == NULL) {
        mp = ts_calloc(1, sizeof(ts_pk_mapping_t));
        assert(mp != NULL);
        mp->pk = pk;
        ts_docid_container_init(&mp->doc_ids);
        HASH_ADD(hh, seg->pk_mapping, pk, sizeof(uint64_t), mp);
    }
    return mp;
}

static void ts_pk_mapping_free(ts_pk_mapping_t **mp) {
    ts_pk_mapping_t *m, *tmp;
    HASH_ITER(hh, *mp, m, tmp) {
        ts_docid_container_destroy(&m->doc_ids);
    }
    ts_free(*mp);
    *mp = NULL;
}

int ts_segment_add_document(ts_segment_t *seg, ts_document_t *doc) {
    assert(seg != NULL);
    assert(doc != NULL);
    doc->doc_id = seg->generator.incr(&seg->generator);
    if (!ts_document_check(doc)) {
        return -1;
    }
    ts_field_t *field, *tmp;
    HASH_ITER(hh, doc->fields, field, tmp) {
        if (ts_field_indexable(field)) {
            ts_inverted_t *inverted = ts_inverted_get_or_new(seg, field->name);
            ts_inverted_add_document(inverted, doc);
        }
    }
    ts_pk_mapping_t *mp = ts_pk_mapping_get_or_new(seg, doc->pk);
    ts_docid_container_append(&mp->doc_ids, &doc->doc_id);
    seg->num_total_docs++;
    return 0;
}

int ts_segment_remove_document(ts_segment_t *seg, uint64_t pk) {
    assert(seg != NULL);
    assert(pk != 0);
    ts_pk_mapping_t *mp;
    HASH_FIND(hh, seg->pk_mapping, &pk, sizeof(uint64_t), mp);
    if (mp == NULL) {
        return -1;
    }
    ts_docid_t *doc_id = NULL;
    seg->deletion_lock.wrlock(&seg->deletion_lock);
    ts_docid_foreach(&mp->doc_ids, doc_id) {
        if (!ts_bitmap_get(&seg->deletion, *doc_id)) {
            ts_bitmap_set(&seg->deletion, *doc_id);
            seg->num_total_deleted++;
        }
    }
    seg->deletion_lock.unlock(&seg->deletion_lock);
    return 0;
}

int ts_segment_lookup(ts_segment_t *seg, UT_array *terms, 
        uint32_t limit, ts_docid_container_t *ctn, uint32_t *total) {
    assert(seg != NULL);
    assert(terms != NULL);
    assert(ctn != NULL);
    assert(total != NULL);
    if (limit < 1) {
        return -1;
    }
    if (seg->num_total_docs == 0) {
        return 0;
    }
    if (seg->num_total_docs == seg->num_total_deleted) {
        return 0;
    }
    // 此处要对查询出来的doc对比deletion做剔除
    // 100w文档的deletion最大 1000000/8/1024=122KB
    // 相对频繁lock/unlock带来的开销，直接clone一份效率更高
    // 后续再考虑优化
    ts_bitmap_t deletion;
    seg->deletion_lock.rdlock(&seg->deletion_lock);
    ts_bitmap_clone(&seg->deletion, &deletion);
    seg->deletion_lock.unlock(&seg->deletion_lock);

    uint32_t terms_len = utarray_len(terms);
    if (terms_len == 0) {
        // 没有过滤条件，直接遍历所有pk mapping
        ts_pk_mapping_t *mp, *tmp;
        ts_docid_t *doc_id;
        uint32_t count = 0;
        *total = seg->num_total_docs - seg->num_total_deleted;
        HASH_ITER(hh, seg->pk_mapping, mp, tmp) {
            ts_docid_foreach(&mp->doc_ids, doc_id) {
                if (count >= limit) {
                    goto end;
                }
                if (ts_bitmap_get(&deletion, *doc_id)) {
                    continue;
                }
                ts_docid_container_append(ctn, doc_id);
                count++;
            }
        }
    } else {
        ts_docid_container_t **ids_list = ts_calloc(
                terms_len, sizeof(ts_docid_container_t *));
        assert(ids_list != NULL);
        uint8_t **pp = NULL;
        ts_inverted_t *inverted = NULL;
        uint8_t end = 0;
        uint32_t idx = 0;
        while ((pp = (uint8_t **)utarray_next(terms, pp))) { 
            HASH_FIND_STR(seg->docs, *pp, inverted);
            // 如果某一个倒排未找到
            // 认为整个query的结果为0,直接跳出循环
            if (inverted == NULL || inverted->doc_ids.total == 0) {
                end = 1;
                break;
            }
            ids_list[idx++] = &inverted->doc_ids;
        }
        if (end) {
            *total = 0;
            goto notfound;
        }
        if (terms_len == 1) {
            ts_docid_t *doc_id;
            uint32_t copied = 0;
            *total = inverted->num_total;
            ts_docid_foreach(ids_list[0], doc_id) {
                ts_docid_container_append(ctn, doc_id);
                if ((++copied) >= limit) goto notfound;
            } 
        } else {
            // 求交集
            ts_seek_intersect(ids_list, terms_len, &deletion, 
                    ctn, limit, total);
        }
notfound:
        ts_free(ids_list);
    }
end:
    ts_bitmap_destroy(&deletion);
    return 0;
}

void ts_segment_free(ts_segment_t *seg, uint8_t free_inverted) {
    if (free_inverted) {
        ts_inverted_t *inverted, *tmp;
        HASH_ITER(hh, seg->docs, inverted, tmp) {
            ts_inverted_free(inverted);
        }
    }
    ts_bitmap_destroy(&seg->deletion);
    ts_pk_mapping_free(&seg->pk_mapping);
    ts_rwlock_destroy(&seg->deletion_lock);
    ts_free(seg);
}

