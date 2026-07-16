#pragma once

#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_GLOBAL 0x100

static inline void* dlopen(const char*, int) {
    return nullptr;
}

static inline int dlclose(void*) {
    return 0;
}

static inline void* dlsym(void*, const char*) {
    return nullptr;
}

static inline const char* dlerror(void) {
    return "Dynamic libraries are unavailable on Nintendo Switch";
}
