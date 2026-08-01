//
// Created by cpasjuste on 08/06/2021.
//

#ifndef C2D_SDL1_H
#define C2D_SDL1_H

#include <SDL/SDL.h>

#include "cross2d/platforms/sdl1/sdl1_renderer.h"
#include "cross2d/platforms/sdl1/sdl1_input.h"
#include "cross2d/platforms/sdl1/sdl1_audio.h"
#include "cross2d/platforms/sdl1/sdl1_thread.h"
#include "cross2d/platforms/sdl1/sdl1_mutex.h"

#define C2DRenderer SDL1Renderer
#define C2DInput SDL1Input
#define C2DAudio SDL1Audio
#define C2DThread SDL1Thread
#define C2DMutex SDL1Mutex

#endif //C2D_SDL1_H
