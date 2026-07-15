#pragma once

#include <stddef.h>

struct iovec {
    void* iov_base;
    size_t iov_len;
};
