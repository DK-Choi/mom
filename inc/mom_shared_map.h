#ifndef __MOM_SHARED_MAP_H__
#define __MOM_SHARED_MAP_H__

#include "mom_common.h"
#include "mom_collection.h"
#include "mom_shared_queue.h"

#define BUCKET_SIZE 128

typedef struct {
    char c_use;
    QUEUE_HEADER_T bucket[BUCKET_SIZE];
    CONCURRENT_T concurrent;
} MAP_HEADER_T;

typedef MAP_HEADER_T* MAP_HEADER;

typedef struct {
    MAP_HEADER header;
    COLLECTION collection;
} MAP_T;


typedef MAP_T* MAP;

#ifdef  __cplusplus
extern "C" {
#endif

MAP mom_create_shared_map(RESOURCE resource, size_t max_size, RESULT_DETAIL result_detail);

RESULT mom_destroy_shared_map(MAP this, RESULT_DETAIL result_detail);

RESULT mom_put_shared_map(MAP this, STRING key, ADDRESS data, size_t size, RESULT_DETAIL result_detail);

MAP_DATA mom_get_shared_map(MAP this, STRING key, RESULT_DETAIL result_detail);

long mom_size_shared_map(MAP this, RESULT_DETAIL result_detail);

MAP_DATA mom_remove_shared_map(MAP this, STRING key, RESULT_DETAIL result_detail);

RESULT mom_clear_shared_map(MAP this, RESULT_DETAIL result_detail);

#ifdef  __cplusplus
extern }
#endif

#endif