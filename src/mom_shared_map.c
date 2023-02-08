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

static BUCKET __get_bucket__(MAP this, STRING key) {
    return &this->header->bucket[mom_get_hash_idx(key, BUCKET_SIZE)];
}

static void __init_bucket__(BUCKET bucket) {

    bucket->c_use = USE;
    bucket->start = INIT_OFFSET;
    bucket->last = INIT_OFFSET;
    bucket->cnt = 0;
    mom_concurrent_init(&bucket->concurrent, TRUE);
}

index_info_t *__remove_shared_map__(MAP this, STRING key, BUCKET bucket, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), NULL, ADDRESS, result, FAIL_UNDEF, "undefined map");
    ASSERT_AND_SET_RESULT(ASSERT_NOT_EMPTY_STRING_COND(key), NULL, ADDRESS, result, FAIL_UNDEF, "map key is empty");

    if (bucket->cnt > 0) {
        index_info_t *p_index_info = get_index_addr(this->collection, bucket->start);
        for (; p_index_info != NULL; p_index_info = get_index_addr(this->collection, p_index_info->next)) {
            data_t *p_data_info = get_data_addr(this->collection, p_index_info->data);
            if (p_data_info != NULL) {
                if (strncmp(((MAP_DATA_T *) p_data_info->data)->key, key, MAX_NAME_SZ) == 0) {
                    break;
                }
            }
        }
        if (p_index_info != NULL) {
            data_info_t *p_data = get_data_addr(this->collection, p_index_info->data);
            if (p_data != NULL) {
                hd_free(this->collection, p_data);
            }
        }
        return p_index_info;
    }

    return NULL;
}

MAP_DATA mom_remove_shared_map(MAP this, STRING key, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), NULL, ADDRESS, result, FAIL_UNDEF, "undefined map");
    ASSERT_AND_SET_RESULT(ASSERT_NOT_EMPTY_STRING_COND(key), NULL, ADDRESS, result, FAIL_UNDEF, "map key is empty");

    BUCKET bucket = __get_bucket__(this, key);

    if (bucket->cnt > 0) {
        ASSERT_AND_SET_RESULT(mom_concurrent_rwlock(&bucket->concurrent, FALSE) == SUCCESS, NULL, ADDRESS,
                              result, FAIL_LOCK, "map is busy");

        index_info_t *p_index_info = get_index_addr(this->collection, bucket->start);

        index_info_t *p_p_index_info = NULL;
        for (; p_index_info != NULL; p_index_info = get_index_addr(this->collection, p_index_info->next)) {
            data_t *p_data = get_data_addr(this->collection, p_index_info->data);
            if (p_data != NULL) {
                if (strncmp(((MAP_DATA_T *) p_data->data)->key, key, MAX_NAME_SZ) == 0) {
                    if (p_p_index_info != NULL) {
                        p_p_index_info->next = p_index_info->next;
                        if (p_p_index_info->next < 0) {
                            bucket->last = get_index_offset(this->collection, p_p_index_info);
                        }
                    } else {
                        if (p_index_info->next >= 0) {
                            bucket->start = p_index_info->next;
                        } else {
                            bucket->start = INIT_OFFSET;
                            bucket->last = INIT_OFFSET;
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
        data_t *p_data = get_data_addr(this->collection, p_index_info->data);
        MAP_DATA data = NULL;
        if (p_data != NULL) {
            data = (MAP_DATA) mom_create_shared_data(((MAP_DATA_T *) p_data->data)->size, TRUE, result);
            if (data != NULL) {
                read_data(this->collection, p_index_info->data, (DATA) data, TRUE, sizeof(MAP_DATA_T));
            }
            hd_free(this->collection, p_data);
        }
        hi_free(this->collection, p_index_info);
        return data;
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

    BUCKET bucket = __get_bucket__(this, key);
    ASSERT_AND_SET_RESULT (mom_concurrent_rwlock(&bucket->concurrent, FALSE) == SUCCESS, FAIL_LOCK, RESULT,
                           result, FAIL_LOCK, "map is busy");

    BOOL exist = TRUE;
    index_info_t *p_index_info = __remove_shared_map__(this, key, bucket, result);
    if (p_index_info == NULL) {
        exist = FALSE;
        p_index_info = hi_alloc(this->collection);
    }

    if (p_index_info != NULL) {
        data_info_t *p_data = hd_alloc(this->collection, sizeof(MAP_DATA_T) + size);
        if (p_data != NULL) {
            p_index_info->data = get_data_offset(this->collection, p_data);

            if (exist == FALSE) {
                OFFSET curr_offset = get_index_offset(this->collection, p_index_info);
                if (bucket->last >= 0) {
                    index_info_t *p_p_index_info = get_index_addr(this->collection, bucket->last);
                    if (p_p_index_info != NULL) {
                        p_p_index_info->next = curr_offset;
                    }
                }
                bucket->last = curr_offset;
                if (bucket->start < 0) {
                    bucket->start = bucket->last;
                }
                bucket->cnt++;
            }
            mom_concurrent_rwunlock(&bucket->concurrent);
            MAP_DATA shared_data
                    = (MAP_DATA) mom_create_and_set_shared_data(data, size, TRUE, result);
            if (shared_data != NULL) {
                strncpy(shared_data->key, key, MAX_NAME_SZ);
                write_data(this->collection, p_index_info->data, (DATA) shared_data, sizeof(MAP_DATA_T));
                mom_destroy_shared_data((DATA) shared_data, result);
                return SUCCESS;
            } else {
                return result != NULL ? result->code : FAIL_NULL;
            }
        } else {
            hd_free(this->collection, get_data_addr(this->collection, p_index_info->data));
            hi_free(this->collection, p_index_info);
        }
    }

    mom_concurrent_rwunlock(&bucket->concurrent);

    SET_RESULT(result, FAIL_OVER_MAXIMUM, "map max size exceeded");

    return FAIL_OVER_MAXIMUM;
}

MAP_DATA mom_get_shared_map(MAP this, STRING key, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), NULL, ADDRESS, result, FAIL_UNDEF, "undefined map");
    ASSERT_AND_SET_RESULT(ASSERT_NOT_EMPTY_STRING_COND(key), NULL, ADDRESS, result, FAIL_UNDEF, "map key is empty");

    BUCKET bucket = __get_bucket__(this, key);
    if (bucket->cnt > 0) {
        ASSERT_AND_SET_RESULT (mom_concurrent_rdlock(&bucket->concurrent, FALSE) == SUCCESS, NULL, ADDRESS,
                               result, FAIL_LOCK, "map is busy");

        index_info_t *p_index_info = NULL;
        for (p_index_info = get_index_addr(this->collection, bucket->start);
             p_index_info->next >= 0; p_index_info = get_index_addr(this->collection, p_index_info->next)) {

            data_t *p_data = get_data_addr(this->collection, p_index_info->data);

            if (p_data != NULL) {
                if (strncmp(((MAP_DATA_T *) p_data->data)->key, key, MAX_NAME_SZ) == 0) {
                    break;
                }
            }
        }

        mom_concurrent_rwunlock(&bucket->concurrent);
        if (p_index_info == NULL) {
            return &empty_map;
        }

        data_t *p_data = get_data_addr(this->collection, p_index_info->data);
        MAP_DATA data = NULL;
        if (p_data != NULL) {
            data = (MAP_DATA) mom_create_shared_data(((MAP_DATA_T *) p_data->data)->size, TRUE, result);
            if (data != NULL) {
                read_data(this->collection, p_index_info->data, (DATA) data, TRUE, sizeof(MAP_DATA_T));
            }
        }
        return data;
    }

    return &empty_map;

}

static RESULT __clear_list__(MAP this, BUCKET bucket) {

    if (mom_concurrent_rwlock(&bucket->concurrent, FALSE) != SUCCESS) {
        return FAIL_LOCK;
    }

    index_info_t *p_index_info = NULL;
    for (p_index_info = get_index_addr(this->collection, bucket->start); p_index_info->next >= 0;) {

        index_info_t *p_temp = get_index_addr(this->collection, p_index_info->next);

        hd_free(this->collection, get_data_addr(this->collection, p_index_info->data));
        hi_free(this->collection, p_index_info);

        p_index_info = p_temp;
    }

    if (p_index_info != NULL) {
        hd_free(this->collection, get_data_addr(this->collection, p_index_info->data));
        hi_free(this->collection, p_index_info);
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


MAP mom_create_shared_map(RESOURCE resource, size_t max_size, size_t chunk_size, BOOL recreate_mode, RESULT_DETAIL result) {

    MAP this = NULL;

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(resource), NULL, ADDRESS, result, FAIL_UNDEF, "undefined resource");
    ASSERT_AND_SET_RESULT(ASSERT_SIZE_COND(max_size), NULL, ADDRESS, result, FAIL_INVALID_SIZE, "check max size (%zu)",
                          max_size);
    ASSERT_AND_SET_RESULT(ASSERT_CHUNK_COND(chunk_size), NULL, ADDRESS, result, FAIL_INVALID_SIZE, "check chunk size (%zu)",
                          chunk_size);
    ASSERT_AND_SET_RESULT(mom_concurrent_lock(&resource->concurrent, FALSE) == SUCCESS, NULL, ADDRESS,
                          result, FAIL_LOCK, "queue is busy");

    if (resource->collection_type == COLLECTION_TYPE_UNDEF) {
        resource->collection_type = COLLECTION_TYPE_MAP;
    } else {
        ASSERT_IF_FAIL_CALL_AND_SET_RESULT(resource->collection_type == COLLECTION_TYPE_MAP,
                                           mom_concurrent_unlock(&resource->concurrent), NULL, ADDRESS,
                                           result, FAIL_INVALID_TYPE, "already defined other(%d) type",
                                           resource->collection_type);
    }

    this = (MAP) malloc(sizeof(MAP_T));
    ASSERT_IF_FAIL_CALL_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), mom_concurrent_unlock(&resource->concurrent), NULL,
                                       ADDRESS,
                                       result, FAIL_RESOURCE_INSUFFICIENT, "malloc fail");


    memset(this, 0x00, sizeof(MAP_T));
    this->collection = mom_create_collection(resource, max_size, chunk_size, sizeof(MAP_HEADER_T));
    ASSERT_IF_FAIL_CALL_AND_SET_RESULT (ASSERT_ADDRESS_COND(this->collection),
                                        mom_destroy_shared_map(this, result); mom_concurrent_unlock(
                                                &resource->concurrent),
                                        NULL, ADDRESS,
                                        result, FAIL_SYS, "map collection create fail");

    this->header = this->collection->header_base;
    this->collection->p_index_cache = this->header->resource_cache.next_index_cache;
    this->collection->p_data_cache = this->header->resource_cache.next_data_cache;
    this->collection->p_index_pos = &this->header->resource_cache.next_index_pos;
    this->collection->p_data_pos = &this->header->resource_cache.next_data_pos;

    if ((TRUE == recreate_mode && this->header->resource_cache.max_size != max_size) || this->header->c_use != USE) {
        this->header->c_use = USE;
        this->header->resource_cache.max_size = max_size;
        this->header->resource_cache.chunk_size = chunk_size;
        for (int i = 0; i < BUCKET_SIZE; i++) {
            __init_bucket__(&this->header->bucket[i]);
        }
        RESULT rc;
        ASSERT_IF_FAIL_CALL_AND_SET_RESULT ((rc = mom_concurrent_init(&this->header->concurrent, TRUE)) == SUCCESS,
                                            mom_destroy_shared_map(this, result); mom_concurrent_unlock(
                                                    &resource->concurrent),
                                            NULL, ADDRESS,
                                            result, rc, "map concurrent init fail");

        for (int i = 0; i < ALLOC_CACHE_SIZE; i++) {
            this->header->resource_cache.next_index_cache[i] = ALLOC_CACHE_SIZE - i - 1;
            this->header->resource_cache.next_data_cache[i] = ALLOC_CACHE_SIZE - i - 1;
            this->header->resource_cache.next_index_pos = ALLOC_CACHE_SIZE;
            this->header->resource_cache.next_data_pos = ALLOC_CACHE_SIZE;
        }
    } else {
        if (mom_get_alive_pid_count(this->header->resource_cache.pids) == 0) {
            mom_concurrent_unlock(&this->header->concurrent);
            mom_concurrent_rwunlock(&this->header->concurrent);
            for (int i = 0; i < BUCKET_SIZE; i++) {
                mom_concurrent_rwunlock(&this->header->bucket[i].concurrent);
            }
        }
    }

    ASSERT_IF_FAIL_CALL_AND_SET_RESULT (this->header->resource_cache.max_size == max_size,
                                        mom_destroy_shared_map(this, result); mom_concurrent_unlock(
                                                &resource->concurrent),
                                        NULL, ADDRESS,
                                        result, FAIL_INVALID_SIZE,
                                        "map max size is different org_size=[%zu] curr_size=[%zu]",
                                        this->header->resource_cache.max_size, max_size);

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
