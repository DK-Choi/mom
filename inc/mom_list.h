#ifndef __MOM_LIST_H__
#define __MOM_LIST_H__

#ifdef  __cplusplus
extern "C" {
#endif

HANDLE mom_list_init(void);

RESULT mom_list_fini(HANDLE this);

RESULT mom_list_add(HANDLE this, ADDRESS data, size_t sz, COLLECTION_TP tp);

RESULT mom_list_delete(HANDLE this, ADDRESS data);

RESULT mom_list_delete_by_id(HANDLE this, int idx);

RESULT mom_list_clear(HANDLE this);

RESULT mom_list_get_by_idx(HANDLE this, int idx, ADDRESS *p_data, size_t *p_sz, COLLECTION_TP *p_tp);

RESULT mom_list_put_by_idx(HANDLE this, int idx, ADDRESS data, size_t sz, COLLECTION_TP tp);

RESULT mom_list_get_next(HANDLE this, ADDRESS *p_data, size_t *p_sz, COLLECTION_TP *p_tp);

RESULT mom_list_has_more(HANDLE this);

RESULT mom_list_rewind(HANDLE this);

long mom_list_get_size(HANDLE this);

RESULT mom_list_swap(HANDLE this, int idx1, int idx2);

RESULT mom_list_sort(HANDLE this, ORDER_TP order, int (*comp)(ADDRESS, ADDRESS));

RESULT mom_list_search(HANDLE this, ADDRESS key, int (*comp)(ADDRESS, ADDRESS), ADDRESS *p_data, size_t *p_sz);

#ifdef  __cplusplus
extern }
#endif

#endif
