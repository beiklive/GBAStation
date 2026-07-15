#pragma once

#include <asm/hwcap.h>

#define AT_HWCAP 16

static inline unsigned long getauxval(unsigned long type) {
    if (type == AT_HWCAP) {
        return HWCAP_FP | HWCAP_ASIMD | HWCAP_AES | HWCAP_SHA1 | HWCAP_SHA2 | HWCAP_CRC32;
    }
    return 0;
}

