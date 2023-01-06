/*##############################################################################
   Octopus project octopus.clib
   created by dkchoi
   dgchoi2000@gmail.com
   ----------------------------
   - clist.c
   ----------------------------
##############################################################################*/


#include <fcntl.h>
#include <stdarg.h>
#include <pthread.h>
#include <math.h>
#include "mom_common.h"
#include "mom_list.h"
#include "mom_map.h"

typedef struct MOM_LIST_NODE_H {
    void *data;
    int sz;
    int tp;
    struct MOM_LIST_NODE_H *prev;
    struct MOM_LIST_NODE_H *next;
} MOM_LIST_NODE_T;

typedef struct MOM_LIST_MAN_H {
    long count;
    MOM_LIST_NODE_T *curr;
    MOM_LIST_NODE_T *head;
    MOM_LIST_NODE_T *tail;
    pthread_rwlock_t lock;
} MOM_LIST_MAN_T;


/*############################################################################*/

static void __swap_node__(MOM_LIST_NODE_T *node1, MOM_LIST_NODE_T *node2) {

    void *wk = NULL;
    int wk_sz = 0;

    wk = node1->data;
    wk_sz = node1->sz;

    node1->data = node2->data;
    node1->sz = node2->sz;

    node2->data = wk;
    node2->sz = wk_sz;

}


//static MOM_LIST_NODE_T *__get_node__(MOM_LIST_MAN_T *man, int idx) {
//
//    ASSERT_ADDRESS(man, NULL, ADDRESS);
//    ASSERT(man->count >= idx, NULL, ADDRESS);
//
//    MOM_LIST_NODE_T *tmp = NULL;
//    long i = 0;
//    for (tmp = man->head; tmp != NULL; tmp = tmp->next) {
//        if (idx == i) break;
//        i++;
//    }
//    return tmp;
//
//}
//
//static MOM_LIST_NODE_T *__get_mid_node__(MOM_LIST_NODE_T *left_node, MOM_LIST_NODE_T *right_node) {
//
//    MOM_LIST_NODE_T *tmp = NULL;
//    long i = 0;
//    long cnt = 0;
//
//    for (tmp = left_node; tmp->next != NULL && tmp != right_node; tmp = tmp->next) {
//        cnt++;
//    }
//
//    cnt = cnt / 2;
//
//    for (tmp = left_node; tmp->next != NULL && tmp != right_node; tmp = tmp->next) {
//        if (i == cnt) break;
//        i++;
//    }
//
//    return tmp;
//
//}

//static MOM_LIST_NODE_T *
//__search__(MOM_LIST_MAN_T *man, MOM_LIST_NODE_T *left_node, MOM_LIST_NODE_T *right_node, void *key,
//           int (*comp)(void *, void *)) {
//
//    MOM_LIST_NODE_T *mid;
//
//    if (man == NULL) return NULL;
//    if (left_node == NULL) return NULL;
//    if (right_node == NULL) return NULL;
//
//    mid = __get_mid_node__(left_node, right_node);
//    if (mid == NULL) return NULL;
//
//    if (comp(key, mid->data) == 0) return mid;
//
//    if (right_node == left_node) return NULL;
//
//    if (comp(key, mid->data) < 0) {
//        return __search__(man, left_node, mid->prev, key, comp);
//    } else {
//        return __search__(man, mid->next, right_node, key, comp);
//    }
//
//    return NULL;
//
//}

static void
__sort__(MOM_LIST_MAN_T *man, MOM_LIST_NODE_T *left_node, MOM_LIST_NODE_T *right_node, int order,
         int (*comp)(void *, void *)) {

    MOM_LIST_NODE_T *key;
    MOM_LIST_NODE_T *_left;
    MOM_LIST_NODE_T *_right;

    if (man == NULL) return;
    if (comp == NULL) return;

    if (left_node == NULL) return;
    if (right_node == NULL) return;
    if (right_node == left_node) return;

    key = left_node;

    _left = left_node->next;
    if (_left == NULL) return;

    _right = right_node;

    while (TRUE) {
        while (order * comp(key->data, _left->data) > 0) {
            _left = _left->next;
            if (_left == NULL) break;
        }

        while (order * comp(key->data, _right->data) < 0) {
            _right = _right->prev;
            if (_right == NULL) return;
        }

        if (_left == NULL) _left = right_node;

        if (key == _right || _left == _right || _left->prev == _right) break;

        if (comp(_left->data, key->data) == 0
            && comp(_right->data, key->data) == 0) {
            _left = _left->next;
            if (_left == NULL) break;
        }

        __swap_node__(_left, _right);
    }

    if (_right == NULL) return;

    __swap_node__(key, _right);

    if (key != _right) __sort__(man, left_node, _right->prev, order, comp);
    if (_right != right_node) __sort__(man, _right->next, right_node, order, comp);

}

/*############################################################################*/

HANDLE mom_list_init() {

    MOM_LIST_MAN_T *man = NULL;
    man = (MOM_LIST_MAN_T *) malloc(sizeof(MOM_LIST_MAN_T));
    ASSERT_ADDRESS(man, NULL, ADDRESS);
    memset(man, 0x00, sizeof(MOM_LIST_MAN_T));
    if (pthread_rwlock_init(&man->lock, NULL) != 0) return NULL;
    return man;

}

RESULT mom_list_fini(HANDLE hndl) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);

    mom_list_clear(hndl);

    if (hndl != NULL) {
        free(hndl);
        hndl = NULL;
    }

    return SUCCESS;

}

RESULT mom_list_add(HANDLE hndl, ADDRESS data, size_t sz, COLLECTION_TP tp) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);
    ASSERT_ADDRESS(data, FAIL_NULL, RESULT);
    ASSERT_SIZE(sz, FAIL_INVALID_SIZE, RESULT);
    ASSERT_COLLECTION_TP(tp, FAIL_INVALID_TYPE, RESULT);

    MOM_LIST_MAN_T *man = NULL;
    MOM_LIST_NODE_T *tmp = NULL;

    man = (MOM_LIST_MAN_T *) hndl;

    ASSERT(pthread_rwlock_trywrlock(&man->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

    tmp = man->tail;
    man->tail = (MOM_LIST_NODE_T *) malloc(sizeof(MOM_LIST_NODE_T));
    ASSERT_ADDRESS(man->tail, FAIL_RESOURCE_INSUFFICIENT, RESULT);
    man->tail->data = data;
    man->tail->sz = sz;
    man->tail->tp = tp;
    man->tail->prev = tmp;
    man->tail->next = NULL;

    if (man->head == NULL) {
        man->head = man->tail;
    } else {
        if (tmp != NULL) tmp->next = man->tail;
    }

    if (man->curr == NULL) man->curr = man->head;

    man->count++;

    pthread_rwlock_unlock(&man->lock);

    return SUCCESS;

}

RESULT mom_list_delete(HANDLE hndl, ADDRESS data) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);
    ASSERT_ADDRESS(data, FAIL_NULL, RESULT);

    MOM_LIST_MAN_T *man = NULL;
    MOM_LIST_NODE_T *tmp = NULL;
    int is_curr = 0;

    man = (MOM_LIST_MAN_T *) hndl;

    ASSERT(pthread_rwlock_trywrlock(&man->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

    for (tmp = man->head; tmp != NULL; tmp = tmp->next) {
        if (tmp->data == data) break;
    }

    if (tmp == NULL) {
        pthread_rwlock_unlock(&man->lock);
        return SUCCESS;
    }

    if (tmp->data != NULL) {
        switch (tmp->tp) {
            case COLLECTION_TYPE_LIST:
                mom_list_fini(tmp->data);
                break;
            case COLLECTION_TYPE_MAP:
                mom_map_fini(tmp->data);
                break;
            default:
                if (tmp->tp == COLLECTION_TYPE_POINTER) {
                    if (tmp->data != NULL) {
                        free(tmp->data);
                        tmp->data = NULL;
                    }
                }
                break;
        }
    }

    if (man->curr == tmp) is_curr = 1;

    if (tmp == man->head) {
        man->head = tmp->next;
        if (tmp->next != NULL) tmp->next->prev = NULL;
        if (is_curr) man->curr = man->head;
    } else if (tmp == man->tail) {
        man->tail = tmp->prev;
        if (tmp->prev != NULL) tmp->prev->next = NULL;
        if (is_curr) man->curr = man->tail;
    } else {
        tmp->prev->next = tmp->next;
        tmp->next->prev = tmp->prev;
        if (is_curr) man->curr = tmp->prev;
    }

    free(tmp);
    tmp = NULL;

    man->count--;

    pthread_rwlock_unlock(&man->lock);

    return SUCCESS;

}

RESULT mom_list_exist_by_value(HANDLE hndl, ADDRESS data) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);
    ASSERT_ADDRESS(data, FAIL_NULL, RESULT);

    MOM_LIST_MAN_T *man = NULL;
    MOM_LIST_NODE_T *tmp = NULL;

    man = (MOM_LIST_MAN_T *) hndl;

    ASSERT(pthread_rwlock_tryrdlock(&man->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

    for (tmp = man->head; tmp != NULL; tmp = tmp->next) {
        if (memcmp(tmp->data, data, tmp->sz) == 0) {
            pthread_rwlock_unlock(&man->lock);
            return EXIST;
        }
    }

    pthread_rwlock_unlock(&man->lock);

    return NOT_EXIST;

}

RESULT mom_list_delete_by_value(HANDLE hndl, ADDRESS data) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);
    ASSERT_ADDRESS(data, FAIL_NULL, RESULT);

    MOM_LIST_MAN_T *man = NULL;
    MOM_LIST_NODE_T *tmp = NULL;
    int is_curr = 0;

    man = (MOM_LIST_MAN_T *) hndl;

    ASSERT(pthread_rwlock_tryrdlock(&man->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

    for (tmp = man->head; tmp != NULL; tmp = tmp->next) {
        if (memcmp(tmp->data, data, tmp->sz) == 0) break;
    }

    if (tmp == NULL) {
        pthread_rwlock_unlock(&man->lock);
        return SUCCESS;
    }

    if (tmp->data != NULL) {
        switch (tmp->tp) {
            case COLLECTION_TYPE_LIST:
                mom_list_fini(tmp->data);
                break;
            case COLLECTION_TYPE_MAP:
                mom_map_fini(tmp->data);
                break;
            default:
                if (tmp->tp == COLLECTION_TYPE_POINTER) {
                    if (tmp->data != NULL) {
                        free(tmp->data);
                        tmp->data = NULL;
                    }
                }
                break;
        }
    }

    if (man->curr == tmp) is_curr = 1;

    if (tmp == man->head) {
        man->head = tmp->next;
        if (tmp->next != NULL) tmp->next->prev = NULL;
        if (is_curr) man->curr = man->head;
    } else if (tmp == man->tail) {
        man->tail = tmp->prev;
        if (tmp->prev != NULL) tmp->prev->next = NULL;
        if (is_curr) man->curr = man->tail;
    } else {
        if (tmp->prev != NULL) tmp->prev->next = tmp->next;
        if (tmp->next != NULL) tmp->next->prev = tmp->prev;
        if (is_curr) man->curr = tmp->prev;
    }

    free(tmp);
    tmp = NULL;

    man->count--;

    pthread_rwlock_unlock(&man->lock);

    return SUCCESS;

}

RESULT mom_list_delete_by_id(HANDLE hndl, int idx) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);
    ASSERT_INDEX(idx, FAIL_INVALID_INDEX, RESULT);

    MOM_LIST_MAN_T *man = NULL;
    MOM_LIST_NODE_T *tmp = NULL;
    long i = 0;
    int is_curr = 0;

    man = (MOM_LIST_MAN_T *) hndl;

    ASSERT(man->count >= idx, FAIL_OUT_OF_INDEX, RESULT);
    ASSERT(pthread_rwlock_trywrlock(&man->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

    for (tmp = man->head; tmp != NULL; tmp = tmp->next) {
        if (idx == i) break;
        i++;
    }

    if (tmp == NULL) {
        pthread_rwlock_unlock(&man->lock);
        return SUCCESS;
    }

    if (tmp->data != NULL) {
        switch (tmp->tp) {
            case COLLECTION_TYPE_LIST:
                mom_list_fini(tmp->data);
                break;
            case COLLECTION_TYPE_MAP:
                mom_map_fini(tmp->data);
                break;
            default:
                if (tmp->tp == COLLECTION_TYPE_POINTER) {
                    if (tmp->data != NULL) {
                        free(tmp->data);
                        tmp->data = NULL;
                    }
                }
                break;
        }
    }

    if (man->curr == tmp) is_curr = 1;

    if (tmp == man->head) {
        man->head = tmp->next;
        if (tmp->next != NULL) tmp->next->prev = NULL;
        if (is_curr) man->curr = man->head;
    } else if (tmp == man->tail) {
        man->tail = tmp->prev;
        if (tmp->prev != NULL) tmp->prev->next = NULL;
        if (is_curr) man->curr = man->tail;
    } else {
        if (tmp->prev != NULL) tmp->prev->next = tmp->next;
        if (tmp->next != NULL) tmp->next->prev = tmp->prev;
        if (is_curr) man->curr = tmp->prev;
    }

    if (tmp != NULL) {
        free(tmp);
        tmp = NULL;
    }

    man->count--;

    pthread_rwlock_unlock(&man->lock);

    return SUCCESS;

}

RESULT mom_list_clear(HANDLE hndl) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);

    MOM_LIST_MAN_T *man = (MOM_LIST_MAN_T *) hndl;
    int rc;

    while (man->count > 0) {
        if ((rc = mom_list_delete_by_id(man, 0)) != 0) return rc;
    }

    return SUCCESS;

}

RESULT mom_list_get_by_idx(HANDLE hndl, int idx, ADDRESS *p_data, size_t *p_sz, COLLECTION_TP *p_tp) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);

    MOM_LIST_MAN_T *man = NULL;
    MOM_LIST_NODE_T *tmp = NULL;
    long i = 0;

    man = (MOM_LIST_MAN_T *) hndl;

    ASSERT(man->count >= idx, FAIL_OUT_OF_INDEX, RESULT);
    ASSERT(pthread_rwlock_tryrdlock(&man->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

    for (tmp = man->head; tmp != NULL; tmp = tmp->next) {
        if (idx == i) break;
        i++;
    }

    if (p_data != NULL) *p_data = tmp->data;
    if (p_sz != NULL) *p_sz = tmp->sz;
    if (p_tp != NULL) *p_tp = tmp->tp;

    pthread_rwlock_unlock(&man->lock);

    return SUCCESS;

}

RESULT mom_list_put_by_idx(HANDLE hndl, int idx, ADDRESS data, size_t sz, COLLECTION_TP tp) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);
    ASSERT_ADDRESS(data, FAIL_NULL, RESULT);
    ASSERT_INDEX(idx, FAIL_INVALID_INDEX, RESULT);
    ASSERT_SIZE(sz, FAIL_INVALID_SIZE, RESULT);
    ASSERT_COLLECTION_TP(tp, FAIL_INVALID_TYPE, RESULT);

    MOM_LIST_MAN_T *man = NULL;
    MOM_LIST_NODE_T *tmp = NULL;
    long i = 0;

    man = (MOM_LIST_MAN_T *) hndl;

    ASSERT(man->count >= idx, FAIL_OUT_OF_INDEX, RESULT);
    ASSERT(pthread_rwlock_trywrlock(&man->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

    for (tmp = man->head; tmp != NULL; tmp = tmp->next) {
        if (idx == i) break;
        i++;
    }

    if (tmp == NULL) {
        pthread_rwlock_unlock(&man->lock);
        return FAIL_OUT_OF_INDEX;
    }

    if (data != tmp->data) {

        if (tmp->data != NULL) {
            switch (tmp->tp) {
                case COLLECTION_TYPE_LIST:
                    mom_list_clear(tmp->data);
                    break;
                case COLLECTION_TYPE_MAP:
                    mom_map_clear(tmp->data);
                    break;
                default:
                    if (tmp->tp == COLLECTION_TYPE_POINTER) {
                        if (tmp->data != NULL) {
                            free(tmp->data);
                            tmp->data = NULL;
                        }
                    }
                    break;
            }
        }
        tmp->data = data;
        tmp->sz = sz;
        tmp->tp = tp;
    }

    pthread_rwlock_unlock(&man->lock);

    return SUCCESS;

}

RESULT mom_list_get_next(HANDLE hndl, ADDRESS *p_data, size_t *p_sz, COLLECTION_TP *p_tp) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);

    MOM_LIST_MAN_T *man = (MOM_LIST_MAN_T *) hndl;

    ASSERT(pthread_rwlock_tryrdlock(&man->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

    if (man->curr == NULL) man->curr = man->head;
    if (p_data != NULL) *p_data = man->curr->data;
    if (p_sz != NULL) *p_sz = man->curr->sz;
    if (p_tp != NULL) *p_tp = man->curr->tp;

    man->curr = man->curr->next;

    pthread_rwlock_unlock(&man->lock);

    return SUCCESS;

}

RESULT mom_list_has_more(HANDLE hndl) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);

    MOM_LIST_MAN_T *man = (MOM_LIST_MAN_T *) hndl;

    ASSERT(pthread_rwlock_tryrdlock(&man->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

    if (man->curr != NULL) {
        pthread_rwlock_unlock(&man->lock);
        return EXIST;
    }

    pthread_rwlock_unlock(&man->lock);

    return NOT_EXIST;

}

RESULT mom_list_rewind(HANDLE hndl) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);

    MOM_LIST_MAN_T *man = (MOM_LIST_MAN_T *) hndl;

    ASSERT(pthread_rwlock_tryrdlock(&man->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

    man->curr = man->head;
    pthread_rwlock_unlock(&man->lock);

    return SUCCESS;

}

long mom_list_get_size(HANDLE hndl) {

    ASSERT_ADDRESS(hndl, SZ_ERR, long);

    MOM_LIST_MAN_T *man = (MOM_LIST_MAN_T *) hndl;

    return man->count;

}

RESULT mom_list_swap(HANDLE hndl, int idx1, int idx2) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);
    ASSERT_INDEX(idx1, FAIL_INVALID_INDEX, RESULT);
    ASSERT_INDEX(idx2, FAIL_INVALID_INDEX, RESULT);

    MOM_LIST_MAN_T *man = NULL;
    MOM_LIST_NODE_T *tmp = NULL;
    MOM_LIST_NODE_T *tmp2 = NULL;
    void *wk = NULL;
    int wk_sz = 0;
    long i = 0;

    man = (MOM_LIST_MAN_T *) hndl;

    ASSERT(man->count >= idx1, FAIL_OUT_OF_INDEX, RESULT);
    ASSERT(man->count >= idx2, FAIL_OUT_OF_INDEX, RESULT);
    ASSERT(pthread_rwlock_trywrlock(&man->lock) == EXIT_SUCCESS, FAIL_LOCK, RESULT);

    for (tmp = man->head; tmp != NULL; tmp = tmp->next) {
        if (idx1 == i) break;
        i++;
    }

    i = 0;
    for (tmp2 = man->head; tmp2 != NULL; tmp2 = tmp2->next) {
        if (idx2 == i) break;
        i++;
    }

    wk = tmp->data;
    wk_sz = tmp->sz;

    tmp->data = tmp2->data;
    tmp->sz = tmp2->sz;

    tmp2->data = wk;
    tmp2->sz = wk_sz;

    pthread_rwlock_unlock(&man->lock);

    return SUCCESS;

}

RESULT mom_list_sort(HANDLE hndl, ORDER_TP order, int (*comp)(ADDRESS, ADDRESS)) {

    ASSERT_ADDRESS(hndl, FAIL_UNDEF, RESULT);
    ASSERT_ADDRESS(comp, FAIL_UNDEF, RESULT);
    ASSERT_ORDER(order, FAIL_INVALID_TYPE, RESULT);

    MOM_LIST_MAN_T *man = (MOM_LIST_MAN_T *) hndl;

    __sort__(hndl, man->head, man->tail, order, comp);

    return SUCCESS;

}

RESULT mom_list_search(HANDLE hndl, ADDRESS key, int (*comp)(ADDRESS, ADDRESS), ADDRESS *p_data, size_t *p_sz) {

    ASSERT_ADDRESS(hndl , FAIL_UNDEF, RESULT);
    ASSERT_ADDRESS(key , FAIL_NULL, RESULT);
    ASSERT_ADDRESS(p_data , FAIL_NULL, RESULT);
    ASSERT_ADDRESS(p_sz , FAIL_NULL, RESULT);

    MOM_LIST_MAN_T *man = NULL;
    MOM_LIST_NODE_T *tmp = NULL;

    man = (MOM_LIST_MAN_T *) hndl;

    *p_data = NULL;
    *p_sz = 0;

    for (tmp = man->head; tmp != NULL; tmp = tmp->next) {
        if (comp(key, tmp->data) == 0) {
            *p_data = tmp->data;
            *p_sz = tmp->sz;
            break;
        }
    }

    return SUCCESS;

}

