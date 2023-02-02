/*##############################################################################
   Octopus project octopus.clib
   created by dkchoi
   dgchoi2000@gmail.com
   ----------------------------
   - cmap.c
   ----------------------------
##############################################################################*/


#include <fcntl.h>
#include <stdarg.h>
#include <pthread.h>
#include "mom_common.h"
#include "mom_list.h"
#include "mom_map.h"

#define LEN_MAP_DATA_KEY 128

typedef struct CBUCKET_H {
    void *slot_hndl;
    pthread_rwlock_t lock;
} CBUCKET_T;

typedef struct CMAP_MAN_H {
    void *key_hndl;
    int bucket_cnt;
    CBUCKET_T *bucket;
} CMAP_MAN_T;

typedef struct CMAP_DATA_H {
    char key[LEN_MAP_DATA_KEY + 1];
    void *data;
    int sz;
    int tp;
} CMAP_DATA_T;

/*############################################################################*/

static long __hash_code__(char *key, int cnt) {

    unsigned long hash = 0;

    while (*key != '\0') {
        hash = ((hash << 4) + (int) (*key)) % cnt;
        key++;
    }

    return hash % cnt;

}

static int __comp_key__(void *k1, void *k2) {

    return strcmp(((CMAP_DATA_T *) k1)->key, ((CMAP_DATA_T *) k2)->key);

}


static int __fini__(void *hndl) {

    CMAP_MAN_T *man = NULL;
    CMAP_DATA_T *map_data = NULL;
    CBUCKET_T *bucket = NULL;
    int i;
    size_t sz;
    int tp;
    int rc;

    man = (CMAP_MAN_T *) hndl;

    for (i = 0; i < man->bucket_cnt; i++) {

        bucket = (CBUCKET_T *) (man->bucket + i);
        if (bucket == NULL || bucket->slot_hndl == NULL) continue;

        ASSERT(pthread_rwlock_trywrlock(&bucket->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

        mom_list_rewind(bucket->slot_hndl);
        while (mom_list_has_more(bucket->slot_hndl) == TRUE) {

            mom_list_get_next(bucket->slot_hndl, (void **) &map_data, &sz, &tp);
            if (map_data == NULL) continue;
            if (map_data->data != NULL) {
                if (map_data->tp == COLLECTION_TYPE_LIST) {
                    mom_list_fini(map_data->data);
                } else if (map_data->tp == COLLECTION_TYPE_MAP) {
                    mom_map_fini(map_data->data);
                } else {
                    if (map_data->tp == COLLECTION_TYPE_POINTER) {
                        if (map_data->data != NULL) {
                            free(map_data->data);
                            map_data->data = NULL;
                        }
                    }
                }
            }
        }

        if ((rc = mom_list_fini(bucket->slot_hndl)) != SUCCESS) {
            pthread_rwlock_unlock(&bucket->lock);
            return rc;
        }

        pthread_rwlock_unlock(&bucket->lock);
    }

    return SUCCESS;

}

/*############################################################################*/

HANDLE mom_map_init(size_t bucket_sz) {

    ASSERT_SIZE(bucket_sz, NULL, ADDRESS);

    CMAP_MAN_T *man = (CMAP_MAN_T *) malloc(sizeof(CMAP_MAN_T));

    ASSERT_ADDRESS(man, NULL, ADDRESS);

    memset(man, 0x00, sizeof(CMAP_MAN_T));

    man->bucket_cnt = bucket_sz;
    man->bucket = (CBUCKET_T *) malloc(sizeof(CBUCKET_T) * man->bucket_cnt);
    memset(man->bucket, 0x00, sizeof(CBUCKET_T) * man->bucket_cnt);

    return man;

}

RESULT mom_map_fini(HANDLE hndl) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);

    CMAP_MAN_T *man = (CMAP_MAN_T *) hndl;

    mom_list_fini(man->key_hndl);

    __fini__(hndl);

    if (man->bucket != NULL) {
        free(man->bucket);
        man->bucket = NULL;
    }

    if (hndl != NULL) {
        free(hndl);
        hndl = NULL;
    }

    return SUCCESS;

}

RESULT mom_map_put(HANDLE hndl, STRING key, ADDRESS data, size_t sz, COLLECTION_TP tp) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);
    ASSERT_ADDRESS(data, FAIL_NULL, RESULT);
    ASSERT_NOT_EMPTY_STRING(key, FAIL_NULL, RESULT);
    ASSERT_SIZE(sz, FAIL_INVALID_SIZE, RESULT);
    ASSERT_COLLECTION_TP(tp, FAIL_INVALID_TYPE, RESULT);

    CMAP_MAN_T *man = NULL;
    CMAP_DATA_T *map_data = NULL;
    long hash;
    size_t map_sz;
    CBUCKET_T *bucket = NULL;
    int rc;

    man = (CMAP_MAN_T *) hndl;
    hash = __hash_code__(key, man->bucket_cnt);

    bucket = (CBUCKET_T *) (man->bucket + hash);

    if (bucket->slot_hndl == NULL) {
        ASSERT(pthread_rwlock_init(&bucket->lock, NULL) == EXIT_SUCCESS, FAIL_LOCK, RESULT);
        bucket->slot_hndl = mom_list_init();
        ASSERT_ADDRESS(bucket->slot_hndl, FAIL_NULL, RESULT);
    }

    ASSERT (pthread_rwlock_trywrlock(&bucket->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

    if ((rc = mom_list_search(bucket->slot_hndl, (void *) key, __comp_key__, (void **) &map_data, &map_sz)) ==
        SUCCESS) {

        if (map_data == NULL) {
            map_data = (CMAP_DATA_T *) malloc(sizeof(CMAP_DATA_T));
            if (map_data == NULL) {
                pthread_rwlock_unlock(&bucket->lock);
                return FAIL_ASSIGN;
            }
            memset(map_data, 0x00, sizeof(CMAP_DATA_T));
            if ((rc = mom_list_add(bucket->slot_hndl, map_data, sizeof(CMAP_DATA_T), COLLECTION_TYPE_POINTER)) !=
                SUCCESS) {

                pthread_rwlock_unlock(&bucket->lock);
                return rc;
            }

            strncpy(map_data->key, key, LEN_MAP_DATA_KEY);
        }

        map_data->data = data;
        map_data->sz = sz;
        map_data->tp = tp;

        pthread_rwlock_unlock(&bucket->lock);
        return SUCCESS;

    } else {
        pthread_rwlock_unlock(&bucket->lock);
        return rc;
    }
}

RESULT mom_map_get(HANDLE hndl, STRING key, ADDRESS *p_data, size_t *p_sz, COLLECTION_TP *p_tp) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);
    ASSERT_NOT_EMPTY_STRING(key, FAIL_NULL, RESULT);

    CMAP_MAN_T *man = NULL;
    CMAP_DATA_T *map_data = NULL;
    long hash;
    size_t map_sz;
    CBUCKET_T *bucket = NULL;

    man = (CMAP_MAN_T *) hndl;
    hash = __hash_code__(key, man->bucket_cnt);

    bucket = (CBUCKET_T *) (man->bucket + hash);

    if (bucket->slot_hndl == NULL) {
        ASSERT(pthread_rwlock_init(&bucket->lock, NULL) == EXIT_SUCCESS, FAIL_LOCK, RESULT);
        bucket->slot_hndl = mom_list_init();
        ASSERT_ADDRESS(bucket->slot_hndl, FAIL_NULL, RESULT);
        *p_data = NULL;
        if (p_sz != NULL) *p_sz = 0;
        return FAIL_NOTFOUND;
    }

    ASSERT(pthread_rwlock_tryrdlock(&bucket->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

    if (mom_list_search(bucket->slot_hndl, key, __comp_key__, (void **) &map_data, &map_sz) == SUCCESS) {
        if (map_data == NULL) {
            *p_data = NULL;
            if (p_sz != NULL) *p_sz = 0;
            pthread_rwlock_unlock(&bucket->lock);
            return FAIL_NOTFOUND;
        }

        if (p_data != NULL) *p_data = map_data->data;
        if (p_sz != NULL) *p_sz = map_data->sz;
        if (p_tp != NULL) *p_tp = map_data->tp;

        pthread_rwlock_unlock(&bucket->lock);
        return SUCCESS;
    }

    pthread_rwlock_unlock(&bucket->lock);
    return FAIL_NOTFOUND;
}

RESULT mom_map_remove(HANDLE hndl, STRING key) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);
    ASSERT_NOT_EMPTY_STRING(key, FAIL_NULL, RESULT);

    CMAP_MAN_T *man = NULL;
    CMAP_DATA_T *map_data = NULL;
    long hash;
    size_t map_sz;
    CBUCKET_T *bucket = NULL;

    man = (CMAP_MAN_T *) hndl;
    hash = __hash_code__(key, man->bucket_cnt);

    bucket = (CBUCKET_T *) (man->bucket + hash);

    if (bucket->slot_hndl == NULL) {
        return SUCCESS;
    }

    ASSERT(pthread_rwlock_trywrlock(&bucket->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

    if (mom_list_search(bucket->slot_hndl, key, __comp_key__, (void **) &map_data, &map_sz) == SUCCESS) {
        if (map_data == NULL) {
            pthread_rwlock_unlock(&bucket->lock);
            return FAIL_NOTFOUND;
        }

        if (map_data->data != NULL) {
            if (map_data->tp == COLLECTION_TYPE_LIST) {
                mom_list_fini(map_data->data);
            } else if (map_data->tp == COLLECTION_TYPE_MAP) {
                mom_map_fini(map_data->data);
            } else {
                if (map_data->tp == COLLECTION_TYPE_POINTER) {
                    free(map_data->data);
                    map_data->data = NULL;
                }
            }
        }

        mom_list_delete(bucket->slot_hndl, map_data);
        pthread_rwlock_unlock(&bucket->lock);
        return SUCCESS;

    }

    pthread_rwlock_unlock(&bucket->lock);
    return FAIL_NOTFOUND;

}

RESULT mom_map_clear(HANDLE hndl) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);

    CMAP_MAN_T *man = NULL;
    CMAP_DATA_T *map_data = NULL;
    CBUCKET_T *bucket = NULL;
    int i, tp, rc;
    size_t sz;

    man = (CMAP_MAN_T *) hndl;

    for (i = 0; i < man->bucket_cnt; i++) {

        bucket = (CBUCKET_T *) (man->bucket + i);
        if (bucket == NULL || bucket->slot_hndl == NULL) continue;

        ASSERT(pthread_rwlock_trywrlock(&bucket->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

        mom_list_rewind(bucket->slot_hndl);
        while (mom_list_has_more(bucket->slot_hndl) == SUCCESS) {

            mom_list_get_next(bucket->slot_hndl, (void **) &map_data, &sz, &tp);
            if (map_data == NULL) continue;
            if (map_data->data != NULL) {
                if (map_data->tp == COLLECTION_TYPE_LIST) {
                    mom_list_fini(map_data->data);
                } else if (map_data->tp == COLLECTION_TYPE_MAP) {
                    mom_map_fini(map_data->data);
                } else {
                    if (map_data->tp == COLLECTION_TYPE_POINTER) {
                        if (map_data->data != NULL) {
                            free(map_data->data);
                            map_data->data = NULL;
                        }
                    }
                }
            }
        }

        if ((rc = mom_list_clear(bucket->slot_hndl)) != SUCCESS) {
            pthread_rwlock_unlock(&bucket->lock);
            return rc;
        }

        pthread_rwlock_unlock(&bucket->lock);
    }

    return SUCCESS;

}

RESULT mom_map_get_keys(HANDLE hndl) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);

    CMAP_MAN_T *man = NULL;
    CMAP_DATA_T *map_data = NULL;
    CBUCKET_T *bucket = NULL;
    int i;
    size_t sz;
    int tp;

    man = (CMAP_MAN_T *) hndl;

    if (man->key_hndl == NULL) {
        man->key_hndl = mom_list_init();
    } else {
        mom_list_fini(man->key_hndl);
    }

    for (i = 0; i < man->bucket_cnt; i++) {
        bucket = (CBUCKET_T *) (man->bucket + i);
        if (bucket == NULL) continue;

        ASSERT(pthread_rwlock_trywrlock(&bucket->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

        mom_list_rewind(bucket->slot_hndl);
        while (mom_list_has_more(bucket->slot_hndl) == EXIST) {

            if (mom_list_get_next(bucket->slot_hndl, (void **) &map_data, &sz, &tp) != 0) break;

            mom_list_add(man->key_hndl, strdup(map_data->key), strlen(map_data->key), COLLECTION_TYPE_POINTER);
        }

        pthread_rwlock_unlock(&bucket->lock);
    }

    return SUCCESS;
}


RESULT mom_map_has_more_keys(HANDLE hndl) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);

    CMAP_MAN_T *man = (CMAP_MAN_T *) hndl;

    ASSERT_ADDRESS(man->key_hndl, NOT_EXIST, RESULT);

    return mom_list_has_more(man->key_hndl);

}

RESULT mom_map_get_next_key(HANDLE hndl, STRING *p_key, size_t *p_sz) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);
    ASSERT_ADDRESS(p_key, FAIL_NULL, RESULT);

    COLLECTION_TP tp;
    CMAP_MAN_T *man = (CMAP_MAN_T *) hndl;

    ASSERT_ADDRESS(man->key_hndl, FAIL_NULL, RESULT);

    return mom_list_get_next(man->key_hndl, (ADDRESS *) p_key, p_sz, &tp);

}
