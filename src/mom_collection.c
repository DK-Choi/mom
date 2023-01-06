/*##############################################################################
   Octopus project octopus.clib
   created by dkchoi
   dgchoi2000@gmail.com
   ----------------------------
   - collection.c
   ----------------------------
##############################################################################*/

#include "mom_collection.h"
#include "mom_collection_core.c"

#ifndef CHUNK_SIZE
#define CHUNK_SIZE 4096
#endif

COLLECTION mom_create_collection(RESOURCE resource, long max_size, long header_size) {

    COLLECTION this = (COLLECTION) malloc(sizeof(COLLECTION_T));

    ASSERT_ADDRESS(this, NULL, COLLECTION);

    memset(this, 0x00, sizeof(COLLECTION_T));

    this->max_size = max_size;
    this->header_size = header_size;
    this->resource = resource;
    this->header_base = resource->addr;
    this->index_base = this->header_base + header_size;
    this->data_base = this->index_base
                      + this->max_size * sizeof(index_info_t);
    this->max_data_size = resource->capacity / CHUNK_SIZE;

    return this;

}

RESULT mom_destroy_collection(COLLECTION this) {
    ASSERT_ADDRESS(this, SUCCESS, RESULT);
    free(this);
    return SUCCESS;
}
