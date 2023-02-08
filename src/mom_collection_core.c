/*##############################################################################
   Octopus project octopus.clib
   created by dkchoi
   dgchoi2000@gmail.com
   ----------------------------
   - collection_core.c
   ----------------------------
##############################################################################*/

#ifndef __MOM_COLLECTION_CORE__
#define __MOM_COLLECTION_CORE__

#include "mom_collection.h"

typedef struct {
    char c_use;
    OFFSET data;
    OFFSET next;
} index_info_t;

typedef struct {
    char c_use;
    size_t sz;
    OFFSET next;
} data_info_t;

typedef struct {
    data_info_t info;
    char data[];
} data_t;


static index_info_t *get_index_addr(COLLECTION collection, OFFSET offset) {

    ASSERT_ADDRESS(collection, NULL, ADDRESS);
    ASSERT_OFFSET(offset, NULL, ADDRESS);

    return (index_info_t *) (collection->index_base + offset);

}

static OFFSET get_index_offset(COLLECTION collection, index_info_t *p) {
    ASSERT_ADDRESS(collection, SZ_ERR, OFFSET);
    ASSERT_ADDRESS(p, SZ_ERR, OFFSET);
    return (OFFSET) p - (OFFSET) collection->index_base;
}

static data_t *get_data_addr(COLLECTION collection, OFFSET offset) {
    ASSERT_ADDRESS(collection, NULL, ADDRESS);
    ASSERT_OFFSET(offset, NULL, ADDRESS);
    return (data_t *) (collection->data_base + offset);
}

static OFFSET get_data_offset(COLLECTION collection, data_t *p) {
    ASSERT_ADDRESS(collection, SZ_ERR, OFFSET);
    ASSERT_ADDRESS(p, SZ_ERR, OFFSET);
    return (OFFSET) p - (OFFSET) collection->data_base;
}

static index_info_t *hi_node_alloc(COLLECTION collection) {

    *collection->p_index_pos = *collection->p_index_pos - 1;
    if (*collection->p_index_pos < 0) {
        *collection->p_index_pos = 0;
        BOOL has_resource = FALSE;
        for (size_t i = 0;
             i < collection->max_size && *collection->p_index_pos < ALLOC_CACHE_SIZE; i++) {
            index_info_t *p = collection->index_base + i * (sizeof(index_info_t));
            if (p != NULL && p->c_use != USE) {
                collection->p_index_cache[ALLOC_CACHE_SIZE - *collection->p_index_pos - 1] = i;
                *collection->p_index_pos = *collection->p_index_pos + 1;
                has_resource = TRUE;
            }
        }
        return has_resource ? hi_node_alloc(collection) : NULL;
    }
    size_t idx = collection->p_index_cache[*collection->p_index_pos];

    index_info_t *p = collection->index_base + idx * (sizeof(index_info_t));
    if (p != NULL && p->c_use != USE) {
        p->next = INIT_OFFSET;
        p->c_use = USE;
        return p;
    }

    return NULL;
}

static void hi_node_free(COLLECTION collection, index_info_t *p) {
    memset(p, 0x00, sizeof(index_info_t));
    p->c_use = NOT_USE;
    p->next = INIT_OFFSET;
    if (*collection->p_index_pos >= ALLOC_CACHE_SIZE) {
        return;
    }
    collection->p_index_cache[*collection->p_index_pos] = get_index_offset(collection, p) / sizeof(index_info_t);
    *collection->p_index_pos = *collection->p_index_pos + 1;
}

static index_info_t *hi_alloc(COLLECTION collection) {

    ASSERT_ADDRESS(collection, NULL, ADDRESS);
    ASSERT_BOOL(mom_concurrent_lock(&collection->resource->concurrent, FALSE) == SUCCESS, NULL, ADDRESS);
    index_info_t *indexInfo = hi_node_alloc(collection);
    mom_concurrent_unlock(&collection->resource->concurrent);
    return indexInfo;
}

static void hi_free(COLLECTION collection, index_info_t *p) {
    ASSERT_ADDRESS(p, NULL, void);
    ASSERT_BOOL(mom_concurrent_lock(&collection->resource->concurrent, FALSE) == SUCCESS, NULL, void);
    hi_node_free(collection, p);
    mom_concurrent_unlock(&collection->resource->concurrent);
}

static data_t *hd_node_alloc(COLLECTION collection) {
    *collection->p_data_pos = *collection->p_data_pos - 1;
    if (*collection->p_data_pos < 0) {
        *collection->p_data_pos = 0;
        BOOL has_resource = FALSE;
        for (size_t i = 0;
             i < collection->max_data_size && *collection->p_data_pos < ALLOC_CACHE_SIZE; i++) {
            data_t *p = collection->data_base + i * (sizeof(data_info_t) + collection->chunk_size);
            if (p != NULL && p->info.c_use != USE) {
                collection->p_data_cache[ALLOC_CACHE_SIZE - *collection->p_data_pos - 1] = i;
                *collection->p_data_pos = *collection->p_data_pos + 1;
                has_resource = TRUE;
            }
        }
        return has_resource ? hd_node_alloc(collection) : NULL;
    }
    size_t idx = collection->p_data_cache[*collection->p_data_pos];
    return collection->data_base + idx * (sizeof(data_info_t) + collection->chunk_size);
}

static void hd_node_free(COLLECTION collection, data_t *p) {
    memset(p, 0x00, sizeof(data_info_t));
    p->info.c_use = NOT_USE;
    p->info.next = INIT_OFFSET;
    if (*collection->p_data_pos >= ALLOC_CACHE_SIZE) {
        return;
    }
    collection->p_data_cache[*collection->p_data_pos] =
            get_data_offset(collection, p) / (sizeof(data_info_t) + collection->chunk_size);
    *collection->p_data_pos = *collection->p_data_pos + 1;
}

static data_t *hd_alloc(COLLECTION collection, size_t size) {

    ASSERT_ADDRESS(collection, NULL, ADDRESS);
    ASSERT_SIZE(size, NULL, ADDRESS);
    ASSERT_BOOL(mom_concurrent_lock(&collection->resource->concurrent, FALSE) == SUCCESS, NULL, ADDRESS);

    int fragment_size = size / collection->chunk_size;

    if (size <= collection->chunk_size) {
        fragment_size = 1;
    } else if (fragment_size % collection->chunk_size > 0) {
        fragment_size++;
    } else {
        //undo
    }

    data_t *p_first = NULL;
    data_t *p_last = NULL;

    for (int j = 0; j < fragment_size; j++) {
        int alloc = FALSE;
        data_t *p = hd_node_alloc(collection);
        if (p != NULL) {
            if (p_last != NULL) {
                p_last->info.next = get_data_offset(collection, p);
            }
            if (j == 0) {
                p_first = p;
            }
            alloc = TRUE;
            p->info.next = INIT_OFFSET;
            p->info.c_use = USE;
            p_last = p;
        }
        if (!alloc) {
            p_first = NULL;
            break;
        }
    }

    mom_concurrent_unlock(&collection->resource->concurrent);

    return p_first;
}


static void hd_free(COLLECTION collection, data_t *p) {

    ASSERT_ADDRESS(collection, NULL, void);
    ASSERT_ADDRESS(p, NULL, void);
    ASSERT_BOOL(mom_concurrent_lock(&collection->resource->concurrent, FALSE) == SUCCESS, NULL, void);

    data_t *t = p;
    data_t *p_next = NULL;
    for (; t != NULL; t = p_next) {
        p_next = get_data_addr(collection, t->info.next);
        hd_node_free(collection, t);
    }
    if (p_next != NULL) {
        hd_node_free(collection, p_next);
    }
    mom_concurrent_unlock(&collection->resource->concurrent);
}


static size_t
write_data(COLLECTION collection, OFFSET data_offset, DATA shared_data, size_t shared_data_size) {

    ASSERT_ADDRESS(collection, SZ_ERR, size_t);
    ASSERT_ADDRESS(shared_data, SZ_ERR, size_t);
    ASSERT_OFFSET(data_offset, SZ_ERR, size_t);
    ASSERT_SIZE(shared_data_size, SZ_ERR, size_t);

    data_t *p_data = get_data_addr(collection, data_offset);
    OFFSET offset = 0;
    size_t cpy_tot_sz = 0;
    OFFSET real_size = shared_data->size + shared_data_size;
    memcpy(p_data->data, shared_data, shared_data_size);

    for (; p_data != NULL; p_data = get_data_addr(collection, p_data->info.next)) {

        p_data->info.sz = real_size - offset > collection->chunk_size
                          ? collection->chunk_size
                          : real_size - offset;

        cpy_tot_sz += p_data->info.sz;
        if (offset == 0) {
            memcpy(p_data->data + shared_data_size, shared_data->data, p_data->info.sz - shared_data_size);
        } else {
            memcpy(p_data->data, shared_data->data + offset - shared_data_size, p_data->info.sz);
        }

        offset += collection->chunk_size;
    }

    return cpy_tot_sz;

}

static size_t
read_data(COLLECTION collection, OFFSET data_offset, DATA shared_data, BOOL is_map, size_t shared_data_size) {

    ASSERT_ADDRESS(collection, SZ_ERR, size_t);
    ASSERT_ADDRESS(shared_data, SZ_ERR, size_t);
    ASSERT_OFFSET(data_offset, SZ_ERR, size_t);
    ASSERT_SIZE(shared_data_size, SZ_ERR, size_t);

    OFFSET offset = 0;
    size_t cpy_tot_sz = 0;
    OFFSET real_size;

    data_t *p_data = get_data_addr(collection, data_offset);

    DATA tmp = (DATA) (p_data->data);
    shared_data->size = tmp->size;
    if (is_map) {
        strncmp(((MAP_DATA) shared_data)->key, ((MAP_DATA) (p_data->data))->key, MAX_NAME_SZ);
    }

    real_size = tmp->size - shared_data_size;

    for (; p_data != NULL; p_data = get_data_addr(collection, p_data->info.next)) {

        size_t cp_sz = real_size - offset > collection->chunk_size
                       ? collection->chunk_size
                       : real_size - offset;

//        if (cp_sz > p_data->info.sz) {
//            cp_sz = p_data->info.sz;
//        }

        //if (cp_sz > 0) {
        if (offset == 0) {
            memcpy(shared_data->data, p_data->data + shared_data_size, p_data->info.sz - shared_data_size);
        } else {
            memcpy(shared_data->data + offset - shared_data_size, p_data->data, p_data->info.sz);
        }
        cpy_tot_sz += p_data->info.sz;
        //}
        offset += collection->chunk_size;

    }

    return cpy_tot_sz;

}

#endif //__COLLECTION_CORE__
