#ifndef __MOM_RESULT_H__
#define __MOM_RESULT_H__

#include "mom_types.h"
#include "mom_assert.h"

enum RESULT_CODE {
    SUCCESS = 0,
    EMPTY = 1,
    EXIST = 2,
    NOT_EXIST = 3,
    FAIL = 101,
    FAIL_NULL,
    FAIL_LOCK,
    FAIL_NOTFOUND,
    FAIL_NODATA,
    FAIL_UNDEF,
    FAIL_OUT_OF_INDEX,
    FAIL_OUT_OF_BOUND,
    FAIL_ASSIGN,
    FAIL_UNSUPPORTED,
    FAIL_INVALID_SIZE,
    FAIL_OVER_MAXIMUM,
    FAIL_RESOURCE_INSUFFICIENT,
    FAIL_INVALID_TYPE,
    FAIL_INVALID_INDEX,
    FAIL_SYS,

};

#define MAX_MESSAGE_SIZE 256

typedef struct {
    char fail;
    int code;
    char message[MAX_MESSAGE_SIZE + 1];
    int line;
    char src[256];
    char fn[256];
} RESULT_DETAIL_T;

typedef RESULT_DETAIL_T *RESULT_DETAIL;

typedef int RESULT;

#define SET_RESULT(target, code, message) mom_set_result(target, code, message, __FILE__, __FUNCTION__, __LINE__)
#define ASSERT_AND_SET_RESULT(cond, rtn, tp, target, code, ...) if(!(cond)) { \
char tmp[256];                                                                      \
sprintf(tmp, __VA_ARGS__);                                                 \
mom_set_result(target, code, tmp, __FILE__, __FUNCTION__, __LINE__);                \
return (tp)rtn;                                                                     \
}

#define ASSERT_IF_FAIL_CALL_AND_SET_RESULT(cond, call, rtn, tp, target, code, ...) if(!(cond)) { \
call;                                                                                                  \
char tmp[256];                                                                                         \
sprintf(tmp, __VA_ARGS__);                                                                    \
mom_set_result(target, code, tmp, __FILE__, __FUNCTION__, __LINE__);                                   \
return (tp)rtn;                                                                                        \
}

#ifdef  __cplusplus
extern "C" {
#endif

void mom_set_result(RESULT_DETAIL target, RESULT code, STRING message, const char* src_nm, const char* fn_nm, int line);

STRING mom_get_result_message(RESULT code);

#ifdef  __cplusplus
extern }
#endif

#endif /* __RESOURCE_H__ */
