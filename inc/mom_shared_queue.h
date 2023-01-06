#ifndef __MOM_SHARED_QUEUE_H__
#define __MOM_SHARED_QUEUE_H__

#include "mom_common.h"
#include "mom_collection.h"
#include "mom_shared_data.h"

typedef struct {
    char c_use;
    OFFSET start;
    OFFSET last;
    long cnt;
    CONCURRENT_T concurrent;
} QUEUE_HEADER_T;

typedef QUEUE_HEADER_T* QUEUE_HEADER;

typedef struct {
    QUEUE_HEADER header;
    COLLECTION collection;
} QUEUE_T;

typedef QUEUE_T *QUEUE;

#ifdef  __cplusplus
extern "C" {
#endif

QUEUE mom_create_shared_queue(RESOURCE resource, size_t max_size, RESULT_DETAIL result_detail);

RESULT mom_destroy_shared_queue(QUEUE this, RESULT_DETAIL result_detail);

long mom_push_shared_queue(QUEUE this, ADDRESS data, size_t size, RESULT_DETAIL result_detail);

long mom_add_shared_queue(QUEUE this, ADDRESS data, size_t size, RESULT_DETAIL result_detail);

DATA mom_poll_shared_queue(QUEUE this, TIMESTAMP timeout, RESULT_DETAIL result_detail);

DATA mom_get_shared_queue(QUEUE this, int idx, RESULT_DETAIL result_detail);

long mom_size_shared_queue(QUEUE this, RESULT_DETAIL result_detail);

DATA mom_remove_shared_queue(QUEUE this, int idx, RESULT_DETAIL result_detail);

RESULT mom_clear_shared_queue(QUEUE this, RESULT_DETAIL result_detail);

#ifdef  __cplusplus
extern }
#endif

#endif
