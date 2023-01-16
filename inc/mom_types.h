//
// Created by dk.choi on 2023/01/02.
//

#ifndef __MOM_TYPES_H__
#define __MOM_TYPES_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

enum COLLECTION_TYPE {
    COLLECTION_TYPE_UNDEF = 0,
    COLLECTION_TYPE_LIST,
    COLLECTION_TYPE_MAP,
    COLLECTION_TYPE_POINTER
};

enum CONCURRENT_TYPE {
    MUTEX = 1,
    RWLOCK,
    COND,
};

#define ORDER_ASC  1
#define ORDER_DSC  -1
#define MAX_NAME_SZ 255
#define ALLOC_CACHE_SIZE 256

typedef char *STRING;
typedef void *HANDLE;
typedef void *ADDRESS;
typedef long long OFFSET;
typedef unsigned long TIMESTAMP;
typedef unsigned int BOOL;
typedef unsigned long CAPACITY;
typedef int COLLECTION_TP;
typedef int ORDER_TP;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define USE 'U'
#define NOT_USE 'N'

#define YES 'Y'
#define NO 'N'

#define SZ_ERR -1
#define INIT_OFFSET -1L

#ifdef DEBUG
#define PRINT_DEBUG(fmt,...) { \
char tmp_fmt[256];             \
sprintf(tmp_fmt, "[%s][%s][%zu]%s\n",__FILE__, __FUNCTION__, __LINE__,fmt);                               \
printf(tmp_fmt,__VA_ARGS__); \
}
#else
#define PRINT_DEBUG(fmt,...)
#endif

#endif //__MOM_TYPES_H__
