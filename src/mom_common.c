/*##############################################################################
   Octopus project octopus.clib
   created by dkchoi
   dgchoi2000@gmail.com
   ----------------------------
   - common.c
   ----------------------------
##############################################################################*/

#include "mom_common.h"
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#define RETRY_WAIT_TIME 10000L //0.01sec
#define ONE_SEC_TO_NANO_SEC 1000000000L
#define ONE_MILLI_SEC 1000
/*############################################################################*/

enum {
    NEED_TO_INIT = 1,
    NEED_TO_RETRY,
    TIMED_OUT,
    INVALID
};

static int __get_error_no__(int rc) {

    switch (rc) {
        case EINVAL:
            return NEED_TO_INIT;
        case EBUSY:
            return NEED_TO_RETRY;
        case ETIMEDOUT:
            return TIMED_OUT;
        default:
            return INVALID;
    }

}


/*############################################################################*/
RESULT mom_concurrent_init(CONCURRENT concurrent, BOOL shared) {

    ASSERT_ADDRESS(concurrent, FAIL_NULL, RESULT);

    pthread_mutexattr_init(&concurrent->mutex_attr);
    pthread_rwlockattr_init(&concurrent->rwlock_attr);
    pthread_condattr_init(&concurrent->cond_attr);
    if (shared == TRUE) {
        if (pthread_mutexattr_setpshared(&concurrent->mutex_attr, PTHREAD_PROCESS_SHARED) != SUCCESS) {
            return FAIL_UNSUPPORTED;
        }
        if (pthread_rwlockattr_setpshared(&concurrent->rwlock_attr, PTHREAD_PROCESS_SHARED) != SUCCESS) {
            return FAIL_UNSUPPORTED;
        }
        if (pthread_condattr_setpshared(&concurrent->cond_attr, PTHREAD_PROCESS_SHARED) != SUCCESS) {
            return FAIL_UNSUPPORTED;
        }
    }

    pthread_mutex_init(&concurrent->mutex, &concurrent->mutex_attr);
    pthread_rwlock_init(&concurrent->rwlock, &concurrent->rwlock_attr);
    pthread_cond_init(&concurrent->cond, &concurrent->cond_attr);

    return SUCCESS;

}

RESULT mom_concurrent_reinit(CONCURRENT concurrent, int flag) {

    ASSERT_ADDRESS(concurrent, FAIL_NULL, RESULT);

    switch (flag) {
        case MUTEX:
            pthread_mutex_init(&concurrent->mutex, &concurrent->mutex_attr);
            break;
        case RWLOCK:
            pthread_rwlock_init(&concurrent->rwlock, &concurrent->rwlock_attr);
            break;
        case COND:
            pthread_cond_init(&concurrent->cond, &concurrent->cond_attr);
            break;
        default:
            return FAIL_UNDEF;
    }

    return SUCCESS;

}

RESULT mom_concurrent_lock(CONCURRENT concurrent, BOOL trymod) {

    ASSERT_ADDRESS(concurrent, FAIL_NULL, RESULT);

    int rc;
    for (int i = 0; i < 10; i++) {
        if (trymod == TRUE) {
            rc = pthread_mutex_trylock(&concurrent->mutex);
        } else {
            rc = pthread_mutex_lock(&concurrent->mutex);
        }
        if (rc != SUCCESS) {
            switch (__get_error_no__(rc)) {
                case NEED_TO_INIT:
                    if ((rc = mom_concurrent_reinit(concurrent, MUTEX)) != SUCCESS) {
                        return rc;
                    }
                    break;
                case NEED_TO_RETRY:
                    usleep(RETRY_WAIT_TIME);
                    break;
                default:
                    return FAIL;
            }
        } else {
            break;
        }
    }

    return SUCCESS;

}

RESULT mom_concurrent_rdlock(CONCURRENT concurrent, BOOL trymod) {

    ASSERT_ADDRESS(concurrent, FAIL_NULL, RESULT);

    int rc;

    for (int i = 0; i < 10; i++) {
        if (trymod == TRUE) {
            rc = pthread_rwlock_tryrdlock(&concurrent->rwlock);
        } else {
            rc = pthread_rwlock_rdlock(&concurrent->rwlock);
        }
        if (rc != SUCCESS) {
            switch (__get_error_no__(rc)) {
                case NEED_TO_INIT:
                    if ((rc = mom_concurrent_reinit(concurrent, RWLOCK)) != SUCCESS) {
                        return rc;
                    }
                    break;
                case NEED_TO_RETRY:
                    usleep(RETRY_WAIT_TIME);
                    break;
                default:
                    return FAIL;
            }
        } else {
            break;
        }
    }

    return SUCCESS;

}

RESULT mom_concurrent_rwlock(CONCURRENT concurrent, BOOL trymod) {

    ASSERT_ADDRESS(concurrent, FAIL_NULL, RESULT);

    int rc;
    for (int i = 0; i < 10; i++) {
        if (trymod == TRUE) {
            rc = pthread_rwlock_trywrlock(&concurrent->rwlock);
        } else {
            rc = pthread_rwlock_wrlock(&concurrent->rwlock);
        }
        int rc2;
        if (rc != SUCCESS) {
            switch (__get_error_no__(rc)) {
                case NEED_TO_INIT:
                    if ((rc2 = mom_concurrent_reinit(concurrent, RWLOCK)) != SUCCESS) {
                        return rc2;
                    }
                    break;
                case NEED_TO_RETRY:
                    usleep(RETRY_WAIT_TIME);
                    break;
                default:
                    return FAIL;
            }
        } else {
            break;
        }
    }
    return rc;
}

RESULT mom_concurrent_wait(CONCURRENT concurrent, BOOL include_lock, TIMESTAMP timeout) {

    ASSERT_ADDRESS(concurrent, FAIL_NULL, RESULT);

    int rc;

    if (include_lock == TRUE) {
        if ((rc = mom_concurrent_lock(concurrent, FALSE)) != SUCCESS) {
            return rc;
        }
    }

    for (int i = 0; i < 10; i++) {

        if (timeout > 0) {
            struct timeval now;
            struct timespec ts;

            gettimeofday(&now, NULL);

            long nano_sec = (now.tv_usec + timeout * ONE_MILLI_SEC) * ONE_MILLI_SEC;
            int fragment_sec = nano_sec / ONE_SEC_TO_NANO_SEC;
            nano_sec = nano_sec % ONE_SEC_TO_NANO_SEC;

            ts.tv_sec = now.tv_sec + fragment_sec;
            ts.tv_nsec = nano_sec;

            rc = pthread_cond_timedwait(&concurrent->cond, &concurrent->mutex, &ts);
        } else {
            rc = pthread_cond_wait(&concurrent->cond, &concurrent->mutex);
        }

        if (rc != SUCCESS) {
            int rc2;
            switch (__get_error_no__(rc)) {
                case NEED_TO_INIT:
                    if ((rc2 = mom_concurrent_reinit(concurrent, COND)) != SUCCESS) {
                        if (include_lock == TRUE) {
                            mom_concurrent_unlock(concurrent);
                        }
                        return rc2;
                    }
                    break;
                case NEED_TO_RETRY:
                    usleep(RETRY_WAIT_TIME);
                    break;
                default:
                    if (include_lock == TRUE) {
                        mom_concurrent_unlock(concurrent);
                    }
                    return FAIL;
            }
        } else {
            break;
        }
    }

    if (include_lock == TRUE) {
        mom_concurrent_unlock(concurrent);
    }

    return SUCCESS;

}

RESULT mom_concurrent_unlock(CONCURRENT concurrent) {
    ASSERT_ADDRESS(concurrent, FAIL_NULL, RESULT);
    pthread_mutex_unlock(&concurrent->mutex);
    return SUCCESS;

}

RESULT mom_concurrent_rwunlock(CONCURRENT concurrent) {

    ASSERT_ADDRESS(concurrent, FAIL_NULL, RESULT);
    pthread_rwlock_unlock(&concurrent->rwlock);
    return SUCCESS;

}

RESULT mom_concurrent_signal(CONCURRENT concurrent) {

    ASSERT_ADDRESS(concurrent, FAIL_NULL, RESULT);
    pthread_cond_signal(&concurrent->cond);
    return SUCCESS;

}

RESULT mom_concurrent_broadcast(CONCURRENT concurrent) {

    ASSERT_ADDRESS(concurrent, FAIL_NULL, RESULT);
    pthread_cond_broadcast(&concurrent->cond);
    return SUCCESS;

}

RESULT mom_concurrent_destroy(CONCURRENT concurrent) {

    ASSERT_ADDRESS(concurrent, FAIL_NULL, RESULT);

    pthread_mutex_destroy(&concurrent->mutex);
    pthread_rwlock_destroy(&concurrent->rwlock);
    pthread_cond_destroy(&concurrent->cond);

    pthread_mutexattr_destroy(&concurrent->mutex_attr);
    pthread_rwlockattr_destroy(&concurrent->rwlock_attr);
    pthread_condattr_destroy(&concurrent->cond_attr);

    return SUCCESS;

}

int mom_get_alive_pid_count(pid_t pids[]) {
    int cnt = 0;
    for (int i = 0; i < MAX_PID_SZ; i++) {
        if (pids[i] > 0) {
            if (kill(pids[i], 0) != 0) {
                pids[i] = 0;
            } else {
                cnt++;
            }
        }
    }
    return cnt;
}

RESULT mom_set_alive_pid(pid_t pids[]){

    for (int i = 0; i < MAX_PID_SZ; i++) {
        if (pids[i] <= 0) {
            pids[i] = getpid();
            return SUCCESS;
        }
    }

    return FAIL_OVER_MAXIMUM;
}


int mom_get_hash_idx(STRING key, int mod) {

    unsigned long hash = 5381;

    for (; *key; ++key) {
        hash ^= *key;
        hash *= 0x5bd1e995;
        hash ^= hash >> 15;
    }

    return hash % mod;
}