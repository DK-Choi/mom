/*##############################################################################
   Octopus project octopus.clib
   created by dkchoi
   dgchoi2000@gmail.com
   ----------------------------
   - shared_map.c
   ----------------------------
##############################################################################*/
#include "mom_shared_map.h"
#include "mom_collection_core.c"

MAP_DATA_T empty_map = {0, NULL, {0x00}};

static QUEUE_HEADER __get_bucket__(MAP this, STRING key) {

    unsigned long hash = 0;

    while (*key != '\0') {
        hash = ((hash << 4) + (int) (*key)) % BUCKET_SIZE;
        key++;
    }

    int bucket_idx = (int) (hash % BUCKET_SIZE);

    return &this->header->bucket[bucket_idx];

}


MAP_DATA mom_remove_shared_map(MAP this, STRING key, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), NULL, ADDRESS, result, FAIL_UNDEF, "undefined map");
    ASSERT_AND_SET_RESULT(ASSERT_NOT_EMPTY_STRING_COND(key), NULL, ADDRESS, result, FAIL_UNDEF, "map key is empty");

    QUEUE_HEADER bucket = __get_bucket__(this, key);

    if (bucket->start >= 0) {
        ASSERT_AND_SET_RESULT(mom_concurrent_wrlock(&bucket->concurrent, FALSE) == SUCCESS, NULL, ADDRESS,
                              result, FAIL_LOCK, "map is busy");

        index_info_t *p_index_info = get_index_addr(this->collection, bucket->start);

        index_info_t *p_p_index_info = NULL;
        for (; p_index_info != NULL; p_index_info = get_index_addr(this->collection, p_index_info->next)) {

            data_info_t *p_data_info = get_data_addr(this->collection, p_index_info->data);

            if (p_data_info != NULL) {

                if (strncmp(((MAP_DATA_T *) p_data_info->data)->key, key, MAX_NAME_SZ) == 0) {
                    if (p_p_index_info != NULL) {
                        p_p_index_info->next = p_index_info->next;
                        if (p_p_index_info->next < 0) {
                            bucket->last = get_index_offset(this->collection, p_p_index_info);
                        }
                    } else {
                        if (p_index_info->next >= 0) {
                            bucket->start = get_index_offset(this->collection,
                                                             (index_info_t *) p_index_info->next);
                        }
                    }

                    bucket->cnt--;

                    break;
                }
            }

            p_p_index_info = p_index_info;
        }

        mom_concurrent_rwunlock(&bucket->concurrent);

        if (p_index_info == NULL) {
            return &empty_map;
        }

        data_info_t *p_temp = get_data_addr(this->collection, p_index_info->data);

        MAP_DATA shared_data = (MAP_DATA) mom_create_shared_data(((MAP_DATA_T *) p_temp->data)->size, TRUE, result);
        if (shared_data != NULL) {
            read_data(this->collection, p_index_info->data, (DATA) shared_data, sizeof(MAP_DATA_T));
            hd_free(this->collection, get_data_addr(this->collection, p_index_info->data));
            hi_free(p_index_info);
        }
        return shared_data;
    }

    return &empty_map;
}

RESULT mom_put_shared_map(MAP this, STRING key, ADDRESS data, size_t size, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), FAIL_UNDEF, RESULT, result, FAIL_UNDEF, "undefined map");
    ASSERT_AND_SET_RESULT(ASSERT_NOT_EMPTY_STRING_COND(key), FAIL_UNDEF, RESULT, result, FAIL_UNDEF,
                          "map key is empty");
    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(data), FAIL_NULL, RESULT, result, FAIL_NULL, "map data is null");
    ASSERT_AND_SET_RESULT(ASSERT_SIZE_COND(size), FAIL_INVALID_SIZE, RESULT, result, FAIL_INVALID_SIZE,
                          "map data size is invalid(%zu)",
                          size);

    MAP_DATA map_data = mom_remove_shared_map(this, key, result);
    ASSERT_ADDRESS(map_data, SZ_ERR, long);

    QUEUE_HEADER bucket = __get_bucket__(this, key);
    ASSERT_AND_SET_RESULT (mom_concurrent_wrlock(&bucket->concurrent, FALSE) == SUCCESS, FAIL_LOCK, RESULT,
                           result, FAIL_LOCK, "map is busy");

    index_info_t *p_index_info = hi_alloc(this->collection);
    if (p_index_info != NULL) {
        data_info_t *p_data_info = hd_alloc(this->collection, sizeof(MAP_DATA_T) + size);
        if (p_data_info != NULL) {
            p_index_info->data = get_data_offset(this->collection, p_data_info);
            if (bucket->start >= 0) {
                p_index_info->next = bucket->start;
            }

            if (bucket->last < 0) {
                bucket->last = get_index_offset(this->collection, p_index_info);
            }

            bucket->start = get_index_offset(this->collection, p_index_info);
            bucket->cnt++;

            mom_concurrent_rwunlock(&bucket->concurrent);

            MAP_DATA shared_data
                    = (MAP_DATA) mom_create_and_set_shared_data(data, size, TRUE, result);
            if (shared_data != NULL) {
                strncpy(shared_data->key, key, MAX_NAME_SZ);
                write_data(this->collection, p_index_info->data, (DATA) shared_data, sizeof(MAP_DATA_T));
                return SUCCESS;
            } else {
                return result != NULL ? result->code : FAIL_NULL;
            }
        } else {
            hd_free(this->collection, get_data_addr(this->collection, p_index_info->data));
            hi_free(p_index_info);
        }
    }

    mom_concurrent_rwunlock(&bucket->concurrent);

    SET_RESULT(result, FAIL_OVER_MAXIMUM, "map max size exceeded");

    return FAIL_OVER_MAXIMUM;
}

MAP_DATA mom_get_shared_map(MAP this, STRING key, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), NULL, ADDRESS, result, FAIL_UNDEF, "undefined map");
    ASSERT_AND_SET_RESULT(ASSERT_NOT_EMPTY_STRING_COND(key), NULL, ADDRESS, result, FAIL_UNDEF, "map key is empty");

    QUEUE_HEADER bucket = __get_bucket__(this, key);
    ASSERT_AND_SET_RESULT (mom_concurrent_wrlock(&bucket->concurrent, FALSE) == SUCCESS, NULL, ADDRESS,
                           result, FAIL_LOCK, "map is busy");

    if (bucket->start >= 0) {
        ASSERT_AND_SET_RESULT (mom_concurrent_rdlock(&bucket->concurrent, FALSE) == SUCCESS, NULL, ADDRESS,
                               result, FAIL_LOCK, "map is busy");

        index_info_t *p_index_info = NULL;
        for (p_index_info = get_index_addr(this->collection, bucket->start);
             p_index_info->next >= 0; p_index_info = get_index_addr(this->collection, p_index_info->next)) {

            data_info_t *p_data_info = get_data_addr(this->collection, p_index_info->data);

            if (p_data_info != NULL) {
                if (strncmp(((MAP_DATA_T *) p_data_info->data)->key, key, MAX_NAME_SZ) == 0) {
                    break;
                }
            }
        }

        mom_concurrent_rwunlock(&bucket->concurrent);

        if (p_index_info == NULL) {
            return &empty_map;
        }

        data_info_t *p_temp = get_data_addr(this->collection, p_index_info->data);

        MAP_DATA shared_data = (MAP_DATA) mom_create_shared_data(((MAP_DATA_T *) p_temp->data)->size, TRUE, result);
        if (shared_data != NULL) {
            read_data(this->collection, p_index_info->data, (DATA) shared_data, sizeof(MAP_DATA_T));
        }
        return shared_data;
    }

    return &empty_map;

}

static RESULT __clear_list__(MAP this, QUEUE_HEADER bucket) {

    if (mom_concurrent_wrlock(&bucket->concurrent, FALSE) != SUCCESS) {
        return FAIL_LOCK;
    }

    index_info_t *p_index_info = NULL;
    for (p_index_info = get_index_addr(this->collection, bucket->start); p_index_info->next >= 0;) {

        index_info_t *p_temp = get_index_addr(this->collection, p_index_info->next);

        hd_free(this->collection, get_data_addr(this->collection, p_index_info->data));
        hi_free(p_index_info);

        p_index_info = p_temp;
    }

    if (p_index_info != NULL) {
        hd_free(this->collection, get_data_addr(this->collection, p_index_info->data));
        hi_free(p_index_info);
    }

    bucket->start = INIT_OFFSET;
    bucket->last = INIT_OFFSET;
    bucket->cnt = 0;

    mom_concurrent_rwunlock(&bucket->concurrent);

    return SUCCESS;

}

RESULT mom_clear_shared_map(MAP this, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), FAIL_UNDEF, RESULT, result, FAIL_UNDEF, "undefined map");

    for (int i = 0; i < BUCKET_SIZE; i++) {
        RESULT rc;
        if ((rc = __clear_list__(this, &this->header->bucket[i])) != SUCCESS) {
            return rc;
        }
    }

    return SUCCESS;

}

long mom_size_shared_map(MAP this, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), SZ_ERR, long, result, FAIL_UNDEF, "undefined map");

    long cnt = 0;
    for (int i = 0; i < BUCKET_SIZE; i++) {
        cnt += this->header->bucket[i].cnt;
    }

    return cnt;

}


MAP mom_create_shared_map(RESOURCE resource, size_t max_size, RESULT_DETAIL result) {

    MAP this = NULL;

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(resource), NULL, ADDRESS, result, FAIL_UNDEF, "undefined resource");
    ASSERT_AND_SET_RESULT(ASSERT_SIZE_COND(max_size), NULL, ADDRESS, result, FAIL_INVALID_SIZE, "check max size (%zu)",
                          max_size);
    ASSERT_AND_SET_RESULT(mom_concurrent_lock(&resource->concurrent, FALSE) == SUCCESS, NULL, ADDRESS,
                          result, FAIL_LOCK, "queue is busy");

    if (resource->collection_type == COLLECTION_TYPE_UNDEF) {
        resource->collection_type = COLLECTION_TYPE_MAP;
    }

    ASSERT_IF_FAIL_CALL_AND_SET_RESULT(resource->collection_type == COLLECTION_TYPE_MAP,
                                       mom_concurrent_unlock(&resource->concurrent), NULL, ADDRESS,
                                       result, FAIL_INVALID_TYPE, "already defined other(%d) type",
                                       resource->collection_type);

    this = (MAP) malloc(sizeof(MAP_T));
    ASSERT_IF_FAIL_CALL_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), mom_concurrent_unlock(&resource->concurrent), NULL,
                                       ADDRESS,
                                       result, FAIL_RESOURCE_INSUFFICIENT, "malloc fail");


    memset(this, 0x00, sizeof(MAP_T));
    this->collection = mom_create_collection(resource, max_size, sizeof(MAP_HEADER_T));
    ASSERT_IF_FAIL_CALL_AND_SET_RESULT (ASSERT_ADDRESS_COND(this->collection),
                                        mom_destroy_shared_map(this, result); mom_concurrent_unlock(
                                                &resource->concurrent),
                                        NULL, ADDRESS,
                                        result, FAIL_SYS, "map collection create fail");

    this->header = this->collection->header_base;

    if (this->header->c_use != USE) {
        this->header->c_use = USE;
        for (int i = 0; i < BUCKET_SIZE; i++) {
            this->header->bucket[i].c_use = USE;
            this->header->bucket[i].start = INIT_OFFSET;
            this->header->bucket[i].last = INIT_OFFSET;
            this->header->bucket[i].cnt = 0;
        }
        RESULT rc;
        ASSERT_IF_FAIL_CALL_AND_SET_RESULT ((rc = mom_concurrent_init(&this->header->concurrent, TRUE)) == SUCCESS,
                                            mom_destroy_shared_map(this, result); mom_concurrent_unlock(
                                                    &resource->concurrent),
                                            NULL, ADDRESS,
                                            result, rc, "map concurrent init fail");
    }

    mom_concurrent_unlock(&resource->concurrent);
    return this;
}


RESULT mom_destroy_shared_map(MAP this, RESULT_DETAIL result) {
    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), FAIL_UNDEF, RESULT, result, FAIL_UNDEF, "undefined map");
    RESULT res;
    ASSERT_AND_SET_RESULT((res = mom_destroy_collection(this->collection)) == SUCCESS, res, RESULT, result, res,
                          "map destroy fail");
    free(this);
    return SUCCESS;

}
