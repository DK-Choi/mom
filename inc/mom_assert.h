//
// Created by dk.choi on 2023/01/02.
//

#ifndef __MOM_ASSERT__
#define __MOM_ASSERT__

#include "mom_types.h"

#define MAX_CAPACITY 0x100000000
#define MIN_CAPACITY 0x1000

#define MAX_SIZE 0x400000
#define MIN_SIZE 0x1

#define MAX_CHUNK 0x1000
#define MIN_CHUNK 0x80

#define MAX_IDX __INT_MAX__
#define MIN_IDX 0

#define ASSERT(cond, rtn, tp) if(!(cond)) return (tp)rtn

#define ASSERT_ADDRESS_COND(value)          ((value) != NULL)
#define ASSERT_CAPACITY_COND(value)         ((value) >= MIN_CAPACITY && (value) <= MAX_CAPACITY)
#define ASSERT_SIZE_COND(value)             ((value) >= MIN_SIZE && (value) <= MAX_SIZE)
#define ASSERT_CHUNK_COND(value)            ((value) >= MIN_CHUNK && (value) <= MAX_CHUNK)
#define ASSERT_NOT_EMPTY_STRING_COND(value) ((value) != NULL && strlen(value) > 0)
#define ASSERT_INDEX_COND(value)            ((value) >= MIN_IDX && (value) <= MAX_IDX)
#define ASSERT_OFFSET_COND(value)           ((value) >= 0)
#define ASSERT_COLLECTION_TP_COND(value)    ((value) >= COLLECTION_TYPE_LIST && (value) <= COLLECTION_TYPE_POINTER)
#define ASSERT_BOOL_COND(value)             ((value) == TRUE)
#define ASSERT_ORDER_COND(value)            ((value) == ORDER_ASC || (value) == ORDER_DSC)

#define ASSERT_ADDRESS(value, rtn, tp)          if(!ASSERT_ADDRESS_COND(value)) return (tp)rtn
#define ASSERT_CAPACITY(value, rtn, tp)         if(!ASSERT_CAPACITY_COND(value)) return (tp)rtn
#define ASSERT_SIZE(value, rtn, tp)             if(!ASSERT_SIZE_COND(value)) return (tp)rtn
#define ASSERT_NOT_EMPTY_STRING(value, rtn, tp) if(!ASSERT_NOT_EMPTY_STRING_COND(value))  return (tp)rtn
#define ASSERT_INDEX(value, rtn, tp)            if(!ASSERT_INDEX_COND(value))  return (tp)rtn
#define ASSERT_OFFSET(value, rtn, tp)           if(!ASSERT_OFFSET_COND(value))  return (tp)rtn
#define ASSERT_COLLECTION_TP(value, rtn, tp)    if(!ASSERT_COLLECTION_TP_COND(value))  return (tp)rtn
#define ASSERT_BOOL(value, rtn, tp)             if(!ASSERT_BOOL_COND(value))  return (tp)rtn
#define ASSERT_ORDER(value, rtn, tp)            if(!ASSERT_ORDER_COND(value))  return (tp)rtn



#endif //__MOM_ASSERT__
