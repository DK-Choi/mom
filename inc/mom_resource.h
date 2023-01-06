#ifndef __MOM_RESOURCE_H__
#define __MOM_RESOURCE_H__

#include "mom_common.h"

#define TYPE_UNDEF 0
#define TYPE_SHM   1
#define TYPE_FILE  2
#define TYPE_LOCAL 3

typedef struct {
    int type;
    int fd;
    int collection_type;
    long capacity;
    char name[MAX_NAME_SZ + 1];
    ADDRESS addr;
    CONCURRENT_T concurrent;
} RESOURCE_T;

typedef RESOURCE_T* RESOURCE;


#ifdef  __cplusplus
extern "C" {
#endif

RESOURCE mom_create_resource_shm(STRING name, CAPACITY capacity);

RESOURCE mom_create_resource_file(STRING name, STRING path, CAPACITY capacity);

RESOURCE mom_create_resource_local(STRING name, CAPACITY capacity);

RESULT mom_destroy_resource(RESOURCE this);

#ifdef  __cplusplus
extern }
#endif

#endif /* __RESOURCE_H__ */
