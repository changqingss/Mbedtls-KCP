#include "common_dlfcn.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *COMM_dlsym(void *handle, const char *symbol) {
  if (handle == NULL) {
    return NULL;
  }

  if (symbol == NULL) {
    return NULL;
  }

  return dlsym(handle, symbol);
}

char *COMM_dlerror(void) { return dlerror(); }

void *COMM_dlopen(const char *sofile, int flag) {
  if (sofile == NULL) {
    return NULL;
  }

  void *handle = dlopen(sofile, flag);
  if (!handle) {
    fprintf(stderr, "dlopen error: %s\n", dlerror());
    return NULL;
  }

  return handle;
}

int COMM_dlclose(void *handle) {
  if (handle == NULL) {
    return -1;
  }

  dlclose(handle);

  return -1;
}
