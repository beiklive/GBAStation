#pragma once

#include "rsxtiny.h"

//extern u32 *buffer[2];
extern tiny_gcmContextData *context; 

#ifdef OLD_TINY3D
extern VideoResolution Video_Resolution;
#else
extern videoResolution Video_Resolution;
#endif

// Initilize the screen and rsx
void init_screen(int command_buffer, int z_method);

// Block the PPU thread untill the previous flip operation has finished.
void waitFlip();

void flip(s32 buffer);

// Needs to be called each frame to map the buffers and setup render target
void setupRenderTarget(u32 currentBuffer);

// to render in one surface
void setupRenderTarget2(u32 rsx_offset, u32 pitch, u32 width, u32 height, u32 format);
