/*##############################################################################
   Octopus project octopus.clib
   created by dkchoi
   dgchoi2000@gmail.com
   ----------------------------
   - shared_data.c
   ----------------------------
##############################################################################*/

#include "mom_shared_data.h"

RESULT mom_set_shared_data(DATA this, ADDRESS data, size_t size, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), FAIL_UNDEF, RESULT,
                          result, FAIL_UNDEF, "undefined data");

    ASSERT_AND_SET_RESULT(ASSERT_SIZE_COND(size), FAIL_INVALID_SIZE, RESULT,
                          result, FAIL_INVALID_SIZE, "check size (%zu)", size);

    ASSERT_AND_SET_RESULT(this->size >= size, FAIL_OUT_OF_BOUND, RESULT,
                          result, FAIL_OUT_OF_BOUND, "check size: org_size=%zu, input_size=%zu", this->size, size);

    memcpy(this->data, data, size);

    return size;

}

ADDRESS mom_get_shared_data(DATA this, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), NULL, ADDRESS,
                          result, FAIL_UNDEF, "undefined data");

    return this->data;

}

DATA mom_create_shared_data(size_t size, BOOL is_map, RESULT_DETAIL result) {

    DATA this;

    ASSERT_AND_SET_RESULT(ASSERT_SIZE_COND(size), NULL, ADDRESS,
                          result, FAIL_INVALID_SIZE, "check size (%zu)", size);

    if (is_map == TRUE) {
        this = (DATA_T *) malloc(sizeof(MAP_DATA_T));
    } else {
        this = (DATA_T *) malloc(sizeof(DATA_T));
    }

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), NULL, ADDRESS,
                          result, FAIL_RESOURCE_INSUFFICIENT, "malloc fail");

    this->size = size;
    this->data = malloc(size);
    ASSERT_IF_FAIL_CALL_AND_SET_RESULT(ASSERT_ADDRESS_COND(this->data), free(this), NULL, ADDRESS,
                                       result, FAIL_INVALID_SIZE, "check size: input size=%zu", size);

    //memset(this->data, 0x00, size);

    return this;

}

DATA mom_create_and_set_shared_data(ADDRESS data, size_t size, BOOL is_map, RESULT_DETAIL result) {

    DATA this = mom_create_shared_data(size, is_map, result);
    ASSERT_ADDRESS(this, NULL, ADDRESS);
    ASSERT(mom_set_shared_data(this, data, size, result) >= 0, NULL, ADDRESS);
    return this;

}

RESULT mom_destroy_shared_data(DATA this, RESULT_DETAIL result) {

    ASSERT_AND_SET_RESULT(ASSERT_ADDRESS_COND(this), FAIL_UNDEF, RESULT,
                          result, FAIL_UNDEF, "undefined data");

    if (this->data != NULL) {
        free(this->data);
    }

    free(this);

    return SUCCESS;

}
