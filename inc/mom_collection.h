#ifndef __MOM_COLLECTION_H__
#define __MOM_COLLECTION_H__

#include "mom_common.h"
#include "mom_resource.h"
#include "mom_shared_data.h"

typedef struct COLLECTION_H{
    size_t  header_size;
    size_t  max_size;
    size_t  max_data_size;
	ADDRESS header_base;
    ADDRESS index_base;
    ADDRESS data_base;
	RESOURCE resource;
    size_t* p_index_cache;
    size_t* p_data_cache;
    int* p_index_pos;
    int* p_data_pos;
} COLLECTION_T;

typedef COLLECTION_T* COLLECTION;

#ifdef  __cplusplus
extern "C" {
#endif

COLLECTION mom_create_collection(RESOURCE resource
								, size_t max_size
								, size_t header_size);
								
RESULT mom_destroy_collection(COLLECTION this);

#ifdef  __cplusplus
extern }
#endif

#endif /* __COLLECTION_H__ */
