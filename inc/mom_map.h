#ifndef __MOM_MAP_H__
#define __MOM_MAP_H__

#ifdef  __cplusplus
extern "C" {
#endif

HANDLE mom_map_init(size_t bucket_sz);

RESULT mom_map_fini(HANDLE this);

RESULT mom_map_put(HANDLE this, STRING key, ADDRESS data, size_t sz, COLLECTION_TP tp);

RESULT mom_map_get(HANDLE this, STRING key, ADDRESS *p_data, size_t *p_sz, COLLECTION_TP *p_tp);

RESULT mom_map_remove(HANDLE this, STRING key);

RESULT mom_map_clear(HANDLE this);

RESULT mom_map_get_keys(HANDLE this);

RESULT mom_map_has_more_keys(HANDLE this);

RESULT mom_map_get_next_key(HANDLE this, STRING *p_key, size_t *p_sz);

#ifdef  __cplusplus
extern }
#endif

#endif
