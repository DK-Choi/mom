/*##############################################################################
   Octopus project octopus.clib
   created by dkchoi
   dgchoi2000@gmail.com
   ----------------------------
   - shared_queue.c
   ----------------------------
##############################################################################*/

#include <time.h>
#include "mom_shared_queue.h"
#include "mom_collection_core.c"

long mom_add_shared_queue(QUEUE this, ADDRESS data, size_t size, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), SZ_ERR, long, result, FAIL_UNDEF, "undefined queue");
    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(data), SZ_ERR, long, result, FAIL_NULL, "data is null");
    ASSERT_AND_SET_RESULT(ASSERT_SIZE_COND(size), SZ_ERR, long, result, FAIL_INVALID_SIZE,
                          "data size greater than zero");
    ASSERT_AND_SET_RESULT(mom_concurrent_rwlock(&this->header->concurrent, FALSE) == SUCCESS, SZ_ERR, long,
                          result, FAIL_LOCK, "queue is busy");

    index_info_t *p_index_info = hi_alloc(this->collection);

    if (p_index_info != NULL) {

        data_info_t *p_data_info = hd_alloc(this->collection, sizeof(DATA_T) + size);

        if (p_data_info != NULL) {

            p_index_info->data = get_data_offset(this->collection, p_data_info);

            if (this->header->last >= 0) {
                index_info_t *p_index_info_last
                        = get_index_addr(this->collection, this->header->last);
                p_index_info_last->next = get_index_offset(this->collection, p_index_info);
            }

            if (this->header->start < 0) {
                this->header->start = get_index_offset(this->collection, p_index_info);
            }

            this->header->last = get_index_offset(this->collection, p_index_info);
            this->header->cnt++;

            mom_concurrent_rwunlock(&this->header->concurrent);

            DATA shared_data = mom_create_and_set_shared_data(data, size, FALSE, result);
            if (shared_data != NULL) {
                write_data(this->collection, p_index_info->data, shared_data, sizeof(DATA_T));
                mom_concurrent_broadcast(&this->header->concurrent);
                return this->header->cnt;
            } else {
                return SZ_ERR;
            }
        } else {
            hd_free(this->collection, get_data_addr(this->collection, p_index_info->data));
            hi_free(this->collection, p_index_info);
        }
    }

    mom_concurrent_rwunlock(&this->header->concurrent);

    SET_RESULT(result, FAIL_OVER_MAXIMUM, "queue max size exceeded");

    return SZ_ERR;

}

long mom_push_shared_queue(QUEUE this, ADDRESS data, size_t size, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), SZ_ERR, long, result, FAIL_UNDEF, "undefined queue");
    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(data), SZ_ERR, long, result, FAIL_NULL, "data is null");
    ASSERT_AND_SET_RESULT(ASSERT_SIZE_COND(size), SZ_ERR, long, result, FAIL_INVALID_SIZE,
                          "data size greater than zero");
    ASSERT_AND_SET_RESULT (mom_concurrent_rwlock(&this->header->concurrent, FALSE) == SUCCESS, SZ_ERR, long,
                           result, FAIL_LOCK, "queue is busy");

    index_info_t *p_index_info = hi_alloc(this->collection);
    if (p_index_info != NULL) {

        data_info_t *p_data_info = hd_alloc(this->collection, sizeof(DATA_T) + size);
        if (p_data_info != NULL) {
            p_index_info->data = get_data_offset(this->collection, p_data_info);
            if (this->header->start >= 0) {
                p_index_info->next = this->header->start;
            }

            if (this->header->last < 0) {
                this->header->last = get_index_offset(this->collection, p_index_info);
            }

            this->header->start = get_index_offset(this->collection, p_index_info);
            this->header->cnt++;

            mom_concurrent_rwunlock(&this->header->concurrent);

            DATA shared_data = mom_create_and_set_shared_data(data, size, FALSE, result);
            if (shared_data != NULL) {
                write_data(this->collection, p_index_info->data, shared_data, sizeof(DATA_T));
                mom_concurrent_broadcast(&this->header->concurrent);
                return this->header->cnt;
            } else {
                return SZ_ERR;
            }
        } else {
            hd_free(this->collection, get_data_addr(this->collection, p_index_info->data));
            hi_free(this->collection, p_index_info);
        }
    }

    mom_concurrent_rwunlock(&this->header->concurrent);

    return SZ_ERR;

}

DATA mom_poll_shared_queue(QUEUE this, TIMESTAMP timeout, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), NULL, ADDRESS, result, FAIL_UNDEF, "undefined queue");

    if (this->header->start < 0) {
        mom_concurrent_wait(&this->header->concurrent, TRUE, timeout);
    }

    if (this->header->start >= 0) {
        ASSERT_AND_SET_RESULT (mom_concurrent_rwlock(&this->header->concurrent, FALSE) == SUCCESS, NULL, ADDRESS,
                               result, FAIL_LOCK, "queue is busy");

        index_info_t *p_index_info = get_index_addr(this->collection, this->header->start);
        this->header->start = p_index_info->next;
        if (this->header->start < 0) {
            this->header->last = INIT_OFFSET;
        }

        this->header->cnt--;

        mom_concurrent_rwunlock(&this->header->concurrent);

        data_info_t *p_temp = get_data_addr(this->collection, p_index_info->data);

        DATA data = mom_create_shared_data(((DATA_T *) p_temp->data)->size, FALSE, result);
        if (data != NULL) {
            read_data(this->collection, p_index_info->data, data, sizeof(DATA_T));
            hd_free(this->collection, p_temp);
            hi_free(this->collection, p_index_info);
        }
        return data;
    }

    return NULL;

}


DATA mom_get_shared_queue(QUEUE this, int idx, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), NULL, ADDRESS, result, FAIL_UNDEF, "undefined queue");

    if (this->header->start >= 0 && idx >= 0 && idx < this->header->cnt) {
        ASSERT_AND_SET_RESULT (mom_concurrent_rdlock(&this->header->concurrent, FALSE) == SUCCESS, NULL, ADDRESS,
                               result, FAIL_LOCK, "queue is busy");

        index_info_t *p_index_info = get_index_addr(this->collection, this->header->start);
        if (idx > 0) {
            for (int i = 1; i <= idx; i++) {
                p_index_info = get_index_addr(this->collection, p_index_info->next);
                if (p_index_info == NULL) {
                    break;
                }
            }
        }

        mom_concurrent_rwunlock(&this->header->concurrent);
        if (p_index_info == NULL) {
            return NULL;
        }

        data_info_t *p_temp = get_data_addr(this->collection, p_index_info->data);

        DATA data = mom_create_shared_data(((DATA) p_temp->data)->size, FALSE, result);
        if (data != NULL) {
            read_data(this->collection, p_index_info->data, data, sizeof(DATA_T));
        }
        return data;
    }

    return NULL;

}

DATA mom_remove_shared_queue(QUEUE this, int idx, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), NULL, ADDRESS, result, FAIL_UNDEF, "undefined queue");

    if (this->header->start >= 0 && idx >= 0 && idx < this->header->cnt) {

        ASSERT_AND_SET_RESULT (mom_concurrent_rwlock(&this->header->concurrent, FALSE) == SUCCESS, NULL, ADDRESS,
                               result, FAIL_LOCK, "queue is busy");

        index_info_t *p_index_info = get_index_addr(this->collection, this->header->start);
        if (idx > 0) {
            for (int i = 0; i < idx - 1; i++) {
                p_index_info = get_index_addr(this->collection, p_index_info->next);
                if (p_index_info == NULL) {
                    break;
                }
            }

            if (p_index_info->next == this->header->last) {
                this->header->last = get_index_offset(this->collection, p_index_info);
            }

            index_info_t *p_index_info_n = get_index_addr(this->collection, p_index_info->next);

            p_index_info->next = p_index_info_n->next;
            p_index_info = p_index_info_n;

        } else {
            this->header->start = p_index_info->next;
            if (this->header->start < 0) {
                this->header->last = INIT_OFFSET;
            }
        }

        if (p_index_info != NULL) {
            this->header->cnt--;
        }

        mom_concurrent_rwunlock(&this->header->concurrent);

        if (p_index_info == NULL) {
            return NULL;
        }

        data_info_t *p_temp = get_data_addr(this->collection, p_index_info->data);

        DATA shared_data = mom_create_shared_data(((DATA_T *) p_temp->data)->size, FALSE, result);
        if (shared_data != NULL) {
            read_data(this->collection, p_index_info->data, shared_data, sizeof(DATA_T));
            hd_free(this->collection, p_temp);
            hi_free(this->collection, p_index_info);
        }
        return shared_data;

    }

    return NULL;

}

RESULT mom_clear_shared_queue(QUEUE this, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), FAIL_UNDEF, RESULT, result, FAIL_UNDEF, "undefined queue");
    ASSERT_AND_SET_RESULT (mom_concurrent_rwlock(&this->header->concurrent, FALSE) == SUCCESS, FAIL_LOCK, RESULT,
                           result, FAIL_LOCK, "queue is busy");

    index_info_t *p_index_info = NULL;
    for (p_index_info = get_index_addr(this->collection, this->header->start); p_index_info->next >= 0;) {

        index_info_t *p_temp = get_index_addr(this->collection, p_index_info->next);

        hd_free(this->collection, get_data_addr(this->collection, p_index_info->data));
        hi_free(this->collection, p_index_info);

        p_index_info = p_temp;
    }

    if (p_index_info != NULL) {
        hd_free(this->collection, get_data_addr(this->collection, p_index_info->data));
        hi_free(this->collection, p_index_info);
    }

    this->header->start = INIT_OFFSET;
    this->header->last = INIT_OFFSET;
    this->header->cnt = 0;

    mom_concurrent_rwunlock(&this->header->concurrent);

    return SUCCESS;

}

long mom_size_shared_queue(QUEUE this, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), SZ_ERR, long, result, FAIL_UNDEF, "undefined queue");

    return this->header->cnt;

}

QUEUE mom_create_shared_queue(RESOURCE resource, size_t max_size, RESULT_DETAIL result) {

    QUEUE this = NULL;

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(resource), NULL, ADDRESS, result, FAIL_UNDEF, "undefined resource");
    ASSERT_AND_SET_RESULT(ASSERT_SIZE_COND(max_size), NULL, ADDRESS, result, FAIL_INVALID_SIZE, "check max size (%zu)",
                          max_size);
    ASSERT_AND_SET_RESULT (mom_concurrent_lock(&resource->concurrent, FALSE) == SUCCESS, NULL, ADDRESS,
                           result, FAIL_LOCK, "queue is busy");


    if (resource->collection_type == COLLECTION_TYPE_UNDEF) {
        resource->collection_type = COLLECTION_TYPE_LIST;
    } else {
        ASSERT_IF_FAIL_CALL_AND_SET_RESULT(resource->collection_type == COLLECTION_TYPE_LIST,
                                           mom_concurrent_unlock(&resource->concurrent), NULL, ADDRESS,
                                           result, FAIL_INVALID_TYPE, "already defined other(%d) type",
                                           resource->collection_type);
    }

    this = (QUEUE) malloc(sizeof(QUEUE_T));
    ASSERT_IF_FAIL_CALL_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), mom_concurrent_unlock(&resource->concurrent), NULL,
                                       ADDRESS,
                                       result, FAIL_RESOURCE_INSUFFICIENT, "malloc fail");

    memset(this, 0x00, sizeof(QUEUE_T));
    this->collection = mom_create_collection(resource, max_size, sizeof(QUEUE_HEADER_T));
    ASSERT_IF_FAIL_CALL_AND_SET_RESULT (ASSERT_ADDRESS_COND(this->collection),
                                        mom_destroy_shared_queue(this, result); mom_concurrent_unlock(
                                                &resource->concurrent),
                                        NULL, ADDRESS,
                                        result, FAIL_SYS, "queue collection create fail");

    this->header = this->collection->header_base;
    this->collection->p_index_cache = this->header->next_index_cache;
    this->collection->p_data_cache = this->header->next_data_cache;
    this->collection->p_index_pos = &this->header->next_index_pos;
    this->collection->p_data_pos = &this->header->next_data_pos;

    if (this->header->c_use != USE) {
        this->header->c_use = USE;
        this->header->start = INIT_OFFSET;
        this->header->last = INIT_OFFSET;
        this->header->cnt = 0;
        RESULT rc;
        ASSERT_IF_FAIL_CALL_AND_SET_RESULT ((rc = mom_concurrent_init(&this->header->concurrent, TRUE)) == SUCCESS,
                                            mom_destroy_shared_queue(this, result); mom_concurrent_unlock(
                                                    &resource->concurrent),
                                            NULL, ADDRESS,
                                            result, rc, "map concurrent init fail");

        for (int i = 0; i < ALLOC_CACHE_SIZE; i++) {
            this->header->next_index_cache[i] = ALLOC_CACHE_SIZE - i - 1;
            this->header->next_data_cache[i] = ALLOC_CACHE_SIZE - i - 1;
            this->header->next_index_pos = ALLOC_CACHE_SIZE;
            this->header->next_data_pos = ALLOC_CACHE_SIZE;
        }
    }

    mom_concurrent_unlock(&resource->concurrent);

    return this;

}

RESULT mom_destroy_shared_queue(QUEUE this, RESULT_DETAIL result) {
    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), FAIL_UNDEF, RESULT, result, FAIL_UNDEF, "undefined resource");
    RESULT res;
    ASSERT_AND_SET_RESULT((res = mom_destroy_collection(this->collection)) == SUCCESS, res, RESULT, result, res,
                          "queue destroy fail");
    free(this);
    return SUCCESS;
}
