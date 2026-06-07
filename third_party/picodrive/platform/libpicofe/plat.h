#pragma once

// Color packing macros for different pixel formats
#ifdef USE_BGR555
#define PXMAKE(r,g,b) (((b)>>3)|(((g)>>3)<<5)|(((r)>>3)<<10))
#elif defined(USE_BGR565)
#define PXMAKE(r,g,b) (((b)>>3)|(((g)>>2)<<5)|(((r)>>3)<<11))
#elif defined(USE_16BPP)
#define PXMAKE(r,g,b) (((r)>>3)|(((g)>>3)<<5)|(((b)>>3)<<10))
#else
#define PXMAKE(r,g,b) (((r))|((g)<<8)|((b)<<16)|(0xff<<24))
#endif

// Platform-specific definitions needed by platform code
#define PICO_INTERNAL
#define PICO_CD

#ifdef __GNUC__
#define unlikely(x) __builtin_expect((x), 0)
#define likely(x)   __builtin_expect((x), 1)
#else
#define unlikely(x) (x)
#define likely(x)   (x)
#endif

#include <stdint.h>
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef  int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
