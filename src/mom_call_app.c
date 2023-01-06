#include "mom_common.h"
#include <dlfcn.h>

RESULT mom_call_app(STRING path, STRING file_name, STRING func_name) {

    //char buf[1024], buf2[1024];
    char *error;
    int (*app)(void *, void *, void *);
    char full_path[256];
    sprintf(full_path, path,file_name);
    void *handle = dlopen(full_path, RTLD_LAZY);
    if (handle == NULL) {
        printf("%s\n", dlerror());
        return FAIL_NULL;
    }
    app = dlsym(handle, func_name);
    if ((error = dlerror()) != NULL) {
        dlclose(handle);
        return 1;
    }

    app("in", "out", "error");

    dlclose(handle);

    return SUCCESS;

}
