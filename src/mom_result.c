/*##############################################################################
   Octopus project octopus.clib
   created by dkchoi
   dgchoi2000@gmail.com
   ----------------------------
   - result.c
   ----------------------------
##############################################################################*/

#include "mom_result.h"
#include <string.h>

STRING mom_get_result_message(RESULT code) {

    switch (code) {
        case FAIL:
            return "Undefined Internal Error";
        case FAIL_NULL:
            return "Null Point Error";
        case FAIL_LOCK:
            return "Resource Already Locked";
        case FAIL_UNDEF:
            return "Value is undefined";
        default:
            return "Undefined Internal Error";
    }

}

void mom_set_result(RESULT_DETAIL result_detail, RESULT code, STRING message, const char* src_nm, const char* fn_nm, int line) {
    if (result_detail != NULL) {
        result_detail->code = code;
        if (message != NULL) {
            strcpy(result_detail->message, message);
        }
        if (code >= FAIL) {
            result_detail->fail = YES;
        } else {
            result_detail->fail = NO;
        }

        strcpy(result_detail->src, src_nm);
        strcpy(result_detail->fn, fn_nm);
        result_detail->line = line;
    }
}