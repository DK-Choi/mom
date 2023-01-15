/*##############################################################################
   Octopus project octopus.clib
   created by dkchoi
   dgchoi2000@gmail.com
   ----------------------------
   - resource.c
   ----------------------------
##############################################################################*/

#include "mom_resource.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include "mom_map.h"

#define RESOURCE_BUCKET_SIZE 128

static ADDRESS __resource_map__;

static RESOURCE __create_resource__(STRING name) {

    RESOURCE this = NULL;

    if (__resource_map__ == NULL) __resource_map__ = mom_map_init(RESOURCE_BUCKET_SIZE);

    int rc = mom_map_get(__resource_map__, name, (ADDRESS *) &this, NULL, NULL);

    if (rc == SUCCESS) {
        return this;
    } else if (rc == FAIL_NOTFOUND) {
        this = (RESOURCE) malloc(sizeof(RESOURCE_T));
        if (this == NULL) {
            return NULL;
        } else {
            memset(this, 0x00, sizeof(RESOURCE_T));
            mom_map_put(__resource_map__, name, this, sizeof(RESOURCE_T), COLLECTION_TYPE_POINTER);
            strncpy(this->name, name, MAX_NAME_SZ);
            mom_concurrent_init(&this->concurrent, FALSE);
            return this;
        }
    } else {
        return NULL;
    }

}

static RESOURCE __return_resource__(STRING name, RESOURCE this) {

    if (this == NULL) return NULL;

    if (this->addr == NULL) {
        mom_map_remove(__resource_map__, name);
        return NULL;
    } else {
        return this;
    }

}

RESOURCE mom_create_resource_shm(STRING name, CAPACITY capacity) {

    ASSERT_NOT_EMPTY_STRING(name, NULL, ADDRESS);
    ASSERT_CAPACITY(capacity, NULL, ADDRESS);

    RESOURCE this = __create_resource__(name);
    ASSERT_ADDRESS(this, NULL, ADDRESS);

    if (this->type == TYPE_UNDEF) {
        this->type = TYPE_SHM;
        this->capacity = capacity;
        this->fd = shm_open(name, O_CREAT | O_RDWR, 0666);
        if (this->fd < 0) goto END;
        this->capacity = capacity;
        ftruncate(this->fd, this->capacity);
        this->addr = mmap(0, capacity, PROT_WRITE | PROT_READ, MAP_SHARED, this->fd, 0);
        if (this->addr == NULL) goto END;
        close(this->fd);
    }

    END:

    return __return_resource__(name, this);

}

RESOURCE mom_create_resource_file(STRING name, STRING path, CAPACITY capacity) {

    ASSERT_NOT_EMPTY_STRING(name, NULL, ADDRESS);
    ASSERT_NOT_EMPTY_STRING(path, NULL, ADDRESS);
    ASSERT_CAPACITY(capacity, NULL, ADDRESS);

    RESOURCE this = __create_resource__(name);
    ASSERT_ADDRESS(this, NULL, ADDRESS);

    if (this->type == TYPE_UNDEF) {
        this->type = TYPE_FILE;
        this->fd = open(path, O_CREAT | O_RDWR, 0666);
        if (this->fd < 0) goto END;
        this->capacity = capacity;
        ftruncate(this->fd, this->capacity);
        this->addr = mmap(0, capacity, PROT_WRITE | PROT_READ, MAP_SHARED, this->fd, 0);
        if (this->addr == NULL) goto END;
    }

    END:

    return __return_resource__(name, this);

}

RESOURCE create_resource_local(STRING name, CAPACITY capacity) {

    ASSERT_NOT_EMPTY_STRING(name, NULL, ADDRESS);
    ASSERT_CAPACITY(capacity, NULL, ADDRESS);

    RESOURCE this = __create_resource__(name);
    ASSERT_ADDRESS(this, NULL, ADDRESS);

    if (this->type == TYPE_UNDEF) {
        this->type = TYPE_LOCAL;
        this->capacity = capacity;
        this->addr = malloc(capacity);
        if(this->addr == NULL) goto END;
    }

    END:

    return __return_resource__(name, this);

}

RESULT mom_destroy_resource(RESOURCE this) {

    ASSERT_ADDRESS(this, FAIL_UNDEF, RESULT);

    if (this->addr != NULL) {
        switch (this->type) {
            case TYPE_LOCAL:
                free(this->addr);
                break;
            case TYPE_FILE:
                munmap(this->addr, this->capacity);
                close(this->fd);
                break;
            case TYPE_SHM:
                munmap(this->addr, this->capacity);
                shm_unlink(this->name);
                close(this->fd);
                break;
        }
    }

    mom_map_remove(__resource_map__, this->name);

    mom_concurrent_destroy(&this->concurrent);

    free(this);

    return SUCCESS;

}
