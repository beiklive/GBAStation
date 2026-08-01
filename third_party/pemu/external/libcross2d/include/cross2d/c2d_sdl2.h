//
// Created by cpasjuste on 08/06/2021.
//

#ifndef C2D_SDL2_H
#define C2D_SDL2_H

#include <SDL2/SDL.h>

#include "cross2d/platforms/sdl2/sdl2_renderer.h"
#include "cross2d/platforms/sdl2/sdl2_input.h"
#include "cross2d/platforms/sdl2/sdl2_audio.h"
#include "cross2d/platforms/sdl2/sdl2_thread.h"
#include "cross2d/platforms/sdl2/sdl2_mutex.h"
#include "cross2d/platforms/sdl2/sdl2_cond.h"
#include "cross2d/platforms/sdl2/sdl2_device.h"

#define C2DRenderer SDL2Renderer
#define C2DInput SDL2Input
#define C2DAudio SDL2Audio
#define C2DThread SDL2Thread
#define C2DMutex SDL2Mutex
#define C2DCond SDL2Cond
#undef C2DDevice
#define C2DDevice SDL2Device

#endif //C2D_SDL2_H
