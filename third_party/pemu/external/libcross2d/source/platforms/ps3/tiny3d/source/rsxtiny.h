#pragma once

#ifdef OLD_TINY3D
    #include <psl1ght/types.h>
    #include "rsx/gcm.h"
#else
    #include <rsx/gcm_sys.h>
#endif

#include <sysutil/video.h>


typedef struct{
	u32 begin;
	u32 end;
	u32 current;
	u32 callback;
} tiny_gcmContextData;

extern int tinyerror_incallback; 
extern int tinycallback_flag;

void tiny3d_alarm(int leds);

/* rsxtiny_FlushBuffer:
 * Flushes the RSX Command buffer.
 */
void rsxtiny_FlushBuffer(tiny_gcmContextData *context);

/* rsxtiny_Init:
 * Maps the memory at ioAddress into the RSX's memory space so the userspace thread
 * and the RSX can comunicate.
 * This shared memory must be 1mb aligned, and at least 1mb long.
 * Also Initilizes a RSX command buffer of cmdSize inside the shared memory. 
 * 
 * Returns a context structure that controls the current status of thecommand buffer.
 */
tiny_gcmContextData *rsxtiny_Init(const u32 cmdSize, const u32 ioSize, const void* ioAddress);

s32 rsxtiny_AddressToOffset(void *ptr, u32 *offset);

// RSX memory allocators
void *rsxtiny_MemAlign(s32 alignment, s32 size);
void *rsxtiny_Mem(s32 size);

