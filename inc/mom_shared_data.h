#ifndef __MOM_SHARED_DATA_H__
#define __MOM_SHARED_DATA_H__

#include "mom_common.h"

typedef struct {
    size_t size;
    ADDRESS data;
} DATA_T;

typedef struct {
    size_t size;
    ADDRESS data;
    char key[MAX_NAME_SZ + 1];
} MAP_DATA_T;

typedef DATA_T *DATA;
typedef MAP_DATA_T *MAP_DATA;

#ifdef  __cplusplus
extern "C" {
#endif

DATA mom_create_shared_data(size_t size, BOOL is_map, RESULT_DETAIL result_detail);

DATA mom_create_and_set_shared_data(ADDRESS data, size_t size, BOOL is_map, RESULT_DETAIL result_detail);

RESULT mom_set_shared_data(DATA this, ADDRESS data, size_t size, RESULT_DETAIL result_detail);

ADDRESS mom_get_shared_data(DATA this, RESULT_DETAIL result_detail);

RESULT mom_destroy_shared_data(DATA this, RESULT_DETAIL result_detail);

#ifdef  __cplusplus
extern }
#endif

#endif /* __SHARED_DATA_H__ */
