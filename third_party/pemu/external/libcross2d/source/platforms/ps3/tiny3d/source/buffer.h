#pragma once

#include "rsxtiny.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef assert
	#define assert(x) if(!(x)) return
#endif


#define INTERNAL_COMMAND_LENGTH(context, len) \
    if(context->current + len*4 > context->end) {\
        if(tinycallback_flag==1) tiny3d_alarm(1); \
        if(tinycallback_flag==2) tiny3d_alarm(2); \
        if(tiny_rsxContextCallback(context, len) != 0) {\
            tinyerror_incallback = 1; tiny3d_alarm(0);return; \
        }\
    }

typedef union {
	float	 f;
	uint32_t u;
} rsxtiny_ieee32;

// Internal function that you shouldn't use unless you are directly manulipting the buffer.
s32 __attribute__((noinline)) tiny_rsxContextCallback(tiny_gcmContextData *context,u32 count);
void commandBufferPut(tiny_gcmContextData *context, uint32_t value);

// Inline macros for writing command packets to the rsx buffer.
void inline extern internal_commandBufferPutCmd1(tiny_gcmContextData* context, uint32_t command, uint32_t v1) {
	uint32_t* buffer = (uint32_t *)(uint64_t) context->current;
	*buffer++ = command | 1 << 18;
	*buffer++ = v1;
	context->current = (uint32_t)(uint64_t) buffer;
}

void inline extern internal_commandBufferPutCmd2(tiny_gcmContextData* context, uint32_t command, uint32_t v1, uint32_t v2) {
	uint32_t* buffer = (uint32_t *)(uint64_t) context->current;
	*buffer++ = command | 2 << 18;
	*buffer++ = v1;
	*buffer++ = v2;
	context->current = (uint32_t)(uint64_t) buffer;
}

void inline extern internal_commandBufferPutCmd3(tiny_gcmContextData* context, uint32_t command, uint32_t v1, uint32_t v2, uint32_t v3) {
	uint32_t* buffer = (uint32_t *)(uint64_t) context->current;
	*buffer++ = command | 3 << 18;
	*buffer++ = v1;
	*buffer++ = v2;
	*buffer++ = v3;
	context->current = (uint32_t)(uint64_t) buffer;
}

void inline extern internal_commandBufferPutCmd4(tiny_gcmContextData* context, uint32_t command, uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4) {
	uint32_t* buffer = (uint32_t *)(uint64_t) context->current;
	*buffer++ = command | 4 << 18;
	*buffer++ = v1;
	*buffer++ = v2;
	*buffer++ = v3;
	*buffer++ = v4;
	context->current = (uint32_t)(uint64_t) buffer;
}

void inline extern internal_commandBufferPutCmd5(tiny_gcmContextData* context, uint32_t command, uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4, uint32_t v5) {
	uint32_t* buffer = (uint32_t *)(uint64_t) context->current;
	*buffer++ = command | 5 << 18;
	*buffer++ = v1;
	*buffer++ = v2;
	*buffer++ = v3;
	*buffer++ = v4;
	*buffer++ = v5;
	context->current = (uint32_t)(uint64_t) buffer;
}

void inline extern internal_commandBufferPutCmd6(tiny_gcmContextData* context, uint32_t command, uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4, uint32_t v5, uint32_t v6) {
	uint32_t* buffer = (uint32_t *)(uint64_t) context->current;
	*buffer++ = command | 6 << 18;
	*buffer++ = v1;
	*buffer++ = v2;
	*buffer++ = v3;
	*buffer++ = v4;
	*buffer++ = v5;
	*buffer++ = v6;
	context->current = (uint32_t)(uint64_t) buffer;
}

void inline extern internal_commandBufferPutCmd7(tiny_gcmContextData* context, uint32_t command, uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4, uint32_t v5, uint32_t v6, uint32_t v7) {
	uint32_t* buffer = (uint32_t *)(uint64_t) context->current;
	*buffer++ = command | 7 << 18;
	*buffer++ = v1;
	*buffer++ = v2;
	*buffer++ = v3;
	*buffer++ = v4;
	*buffer++ = v5;
	*buffer++ = v6;
	*buffer++ = v7;
	context->current = (uint32_t)(uint64_t) buffer;
}

void inline extern internal_commandBufferPutCmd8(tiny_gcmContextData* context, uint32_t command, uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4, uint32_t v5, uint32_t v6, uint32_t v7, uint32_t v8) {
	uint32_t* buffer = (uint32_t *)(uint64_t) context->current;
	*buffer++ = command | 8 << 18;
	*buffer++ = v1;
	*buffer++ = v2;
	*buffer++ = v3;
	*buffer++ = v4;
	*buffer++ = v5;
	*buffer++ = v6;
	*buffer++ = v7;
	*buffer++ = v8;
	context->current = (uint32_t)(uint64_t) buffer;
}

#ifdef __cplusplus
}
#endif
