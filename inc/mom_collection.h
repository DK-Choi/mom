#ifndef __MOM_COLLECTION_H__
#define __MOM_COLLECTION_H__

#include "mom_common.h"
#include "mom_resource.h"
#include "mom_shared_data.h"

typedef struct COLLECTION_H{
	long  header_size;
	long  max_size;
	long  max_data_size;
	ADDRESS header_base;
    ADDRESS index_base;
    ADDRESS data_base;
	RESOURCE resource;
} COLLECTION_T;

typedef COLLECTION_T* COLLECTION;

#ifdef  __cplusplus
extern "C" {
#endif

COLLECTION mom_create_collection(RESOURCE resource
								, long max_size
								, long header_size);
								
RESULT mom_destroy_collection(COLLECTION this);

#ifdef  __cplusplus
extern }
#endif

#endif /* __COLLECTION_H__ */
