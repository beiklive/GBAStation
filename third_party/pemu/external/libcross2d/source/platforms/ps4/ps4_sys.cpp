//
// Created by cpasjuste on 06/01/2022.
//

#include <cstdio>
#include <cstdarg>
#include <orbis/libkernel.h>

#include "cross2d/platforms/ps4/ps4_sys.h"

using namespace c2d;

static char c2d_debug_str[0x1000];

void PS4Sys::print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(c2d_debug_str, 0x1000, fmt, args);
    va_end(args);
    sceKernelDebugOutText(0, c2d_debug_str);
}
