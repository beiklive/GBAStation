#include <cerrno>
#include <csignal>
#include <cstring>
#include <reent.h>
#include <sys/types.h>
#include <unistd.h>

#include "three_ds_stub/ThreeDSSwitch.hpp"

extern "C" long sysconf(int name) {
#ifdef _SC_LEVEL1_DCACHE_LINESIZE
    if (name == _SC_LEVEL1_DCACHE_LINESIZE) {
        return 64;
    }
#endif
    if (name == _SC_PAGESIZE) {
        return 0x1000;
    }
    if (name == _SC_CLK_TCK) {
        return 1'000'000;
    }
    errno = EINVAL;
    return -1;
}

extern "C" int sigprocmask(int, const sigset_t*, sigset_t* old_set) {
    if (old_set) {
        std::memset(old_set, 0, sizeof(*old_set));
    }
    return 0;
}

extern "C" int _getentropy_r(struct _reent*, void* buffer, std::size_t size) {
    randomGet(buffer, size);
    return 0;
}

extern "C" uid_t getuid(void) {
    return 0;
}
