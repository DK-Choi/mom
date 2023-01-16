#ifndef __MOM_COMMON_H__
#define __MOM_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include "mom_types.h"
#include "mom_result.h"

typedef struct CONCURRENT_H {

    pthread_mutex_t mutex;
    pthread_mutexattr_t mutex_attr;
    pthread_rwlockattr_t rwlock_attr;
    pthread_rwlock_t rwlock;
    pthread_condattr_t cond_attr;
    pthread_cond_t cond;

} CONCURRENT_T;

typedef CONCURRENT_T *CONCURRENT;

#ifdef  __cplusplus
extern "C" {
#endif

RESULT mom_concurrent_init(CONCURRENT concurrent, BOOL shared);

RESULT mom_concurrent_reinit(CONCURRENT concurrent, int flag);

RESULT mom_concurrent_lock(CONCURRENT concurrent, BOOL trymod);

RESULT mom_concurrent_rdlock(CONCURRENT concurrent, BOOL trymod);

RESULT mom_concurrent_rwlock(CONCURRENT concurrent, BOOL trymod);

RESULT mom_concurrent_wait(CONCURRENT concurrent, BOOL include_lock, TIMESTAMP timeout);

RESULT mom_concurrent_unlock(CONCURRENT concurrent);

RESULT mom_concurrent_rwunlock(CONCURRENT concurrent);

RESULT mom_concurrent_signal(CONCURRENT concurrent);

RESULT mom_concurrent_broadcast(CONCURRENT concurrent);

RESULT mom_concurrent_destroy(CONCURRENT concurrent);

int mom_get_hash_idx(STRING key, int mod);

#ifdef  __cplusplus
extern }
#endif


#endif /* __COMMON_H__ */
