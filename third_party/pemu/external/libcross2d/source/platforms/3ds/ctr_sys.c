//
// Created by cpasjuste on 22/09/22.
//

#include <3ds.h>
#include "ctr_sys.h"

// minimize linear heap as possible to get a bigger (non-linear) heap size
__attribute__((weak)) u32 __ctru_linear_heap_size = 0x1000000;

u32 ctr_get_heap_space() {
    s64 mem_used;
    //u32 app_memory = *((u32 *) 0x1FF80040);
    svcGetSystemInfo(&mem_used, 0, 1);
    return (u32) mem_used;
}
