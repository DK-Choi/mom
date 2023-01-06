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

#ifndef CHUNK_SIZE
#define CHUNK_SIZE 4096
#endif

typedef struct {
    char c_use;
    long long data;
    long long next;
} index_info_t;

typedef struct {
    char c_use;
    size_t sz;
    unsigned char data[CHUNK_SIZE];
    long long next;
} data_info_t;

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

static data_info_t *get_data_addr(COLLECTION collection, OFFSET offset) {
    ASSERT_ADDRESS(collection, NULL, ADDRESS);
    ASSERT_OFFSET(offset, NULL, ADDRESS);
    return (data_info_t *) (collection->data_base + offset);
}

static OFFSET get_data_offset(COLLECTION collection, data_info_t *p) {
    ASSERT_ADDRESS(collection, SZ_ERR, OFFSET);
    ASSERT_ADDRESS(p, SZ_ERR, OFFSET);
    return (OFFSET) p - (OFFSET) collection->data_base;
}

static index_info_t *hi_alloc(COLLECTION collection) {
    ASSERT_ADDRESS(collection, NULL, ADDRESS);
    for (int i = 0; i < collection->max_size; i++) {
        index_info_t *p = collection->index_base + i * (sizeof(index_info_t));
        if (p != NULL && p->c_use != USE) {
            p->next = INIT_OFFSET;
            p->c_use = USE;
            return p;
        }
    }

    return NULL;
}

static void hi_free(index_info_t *p) {
    ASSERT_ADDRESS(p, NULL, void);
    memset(p, 0x00, sizeof(index_info_t));
}


static data_info_t *hd_alloc(COLLECTION collection, size_t size) {

    ASSERT_ADDRESS(collection, NULL, ADDRESS);
    ASSERT_SIZE(size, NULL, ADDRESS);

    int fragment_size = size / CHUNK_SIZE;

    if (size <= CHUNK_SIZE) {
        fragment_size = 1;
    } else if (fragment_size % CHUNK_SIZE > 0) {
        fragment_size++;
    } else {
        //undo
    }

    data_info_t *p_first = NULL;
    data_info_t *p_last = NULL;

    for (int j = 0; j < fragment_size; j++) {
        int get = 0;
        data_info_t *p = NULL;
        for (int i = 0; i < collection->max_data_size; i++) {
            p = collection->data_base + i * (sizeof(data_info_t));
            if (p != NULL && p->c_use == 0x00) {
                if (p_last != NULL) {
                    p_last->next = get_data_offset(collection, p);
                }

                if (j == 0) {
                    p_first = p;
                }

                get = 1;
                p_last = p;
                p_last->next = INIT_OFFSET;
                p->c_use = USE;
                break;
            }
        }

        if (!get) {
            return NULL;
        }
    }

    return p_first;
}

static void hd_free(COLLECTION collection, data_info_t *p) {

    ASSERT_ADDRESS(collection, NULL, void);
    ASSERT_ADDRESS(p, NULL, void);

    data_info_t *t = p;
    data_info_t *p_next = NULL;
    for (; t != NULL; t = p_next) {
        p_next = get_data_addr(collection, t->next);
        memset(t, 0x00, sizeof(data_info_t));
    }

    if (p_next != NULL) {
        memset(p_next, 0x00, sizeof(data_info_t));
    }
}


static size_t
write_data(COLLECTION collection, OFFSET data_offset, DATA shared_data, size_t shared_data_size) {

    ASSERT_ADDRESS(collection, SZ_ERR, size_t);
    ASSERT_ADDRESS(shared_data, SZ_ERR, size_t);
    ASSERT_OFFSET(data_offset, SZ_ERR, size_t);
    ASSERT_SIZE(shared_data_size, SZ_ERR, size_t);

    data_info_t *p_data = get_data_addr(collection, data_offset);
    OFFSET offset = 0;
    size_t cpy_tot_sz = 0;
    OFFSET real_size = shared_data->size + shared_data_size;

    memcpy(p_data->data, shared_data, shared_data_size);

    for (; p_data != NULL; p_data = get_data_addr(collection, p_data->next)) {

        p_data->sz = real_size - offset > CHUNK_SIZE
                     ? CHUNK_SIZE
                     : real_size - offset;

        cpy_tot_sz += p_data->sz;
        if (offset == 0) {
            memcpy(p_data->data + shared_data_size, shared_data->data + offset, p_data->sz);
        } else {
            memcpy(p_data->data, shared_data->data + offset, p_data->sz);
        }

        offset += CHUNK_SIZE;
    }

    return cpy_tot_sz;

}

static size_t
read_data(COLLECTION collection, OFFSET data_offset, DATA shared_data, size_t shared_data_size) {

    ASSERT_ADDRESS(collection, SZ_ERR, size_t);
    ASSERT_ADDRESS(shared_data, SZ_ERR, size_t);
    ASSERT_OFFSET(data_offset, SZ_ERR, size_t);
    ASSERT_SIZE(shared_data_size, SZ_ERR, size_t);

    OFFSET offset = 0;
    size_t cpy_tot_sz = 0;
    OFFSET real_size;

    data_info_t *p_data = get_data_addr(collection, data_offset);

    DATA tmp = (DATA) (p_data->data);

    memcpy(shared_data, tmp, shared_data_size);

    real_size = shared_data_size + tmp->size;

    shared_data->data = malloc(tmp->size);

    for (; p_data != NULL; p_data = get_data_addr(collection, p_data->next)) {

        size_t cp_sz = real_size - offset > CHUNK_SIZE
                       ? CHUNK_SIZE
                       : real_size - offset;

        if (cp_sz > p_data->sz) {
            cp_sz = p_data->sz;
        }

        if (cp_sz > 0) {
            if (offset == 0) {
                memcpy(shared_data->data + offset, p_data->data + shared_data_size, cp_sz);
            } else {
                memcpy(shared_data->data + offset, p_data->data, cp_sz);
            }
            cpy_tot_sz += cp_sz;
        }
        offset += CHUNK_SIZE;

    }

    return cpy_tot_sz;

}

#endif //__COLLECTION_CORE__
