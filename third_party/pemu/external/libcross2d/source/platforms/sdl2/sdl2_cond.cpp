//
// Created by cpasjuste on 04/02/2021.
//

#include "cross2d/c2d.h"

using namespace c2d;

SDL2Cond::SDL2Cond() {
    cond = SDL_CreateCond();
}

SDL2Cond::~SDL2Cond() {
    SDL_DestroyCond(cond);
}

int SDL2Cond::wait(Mutex *mutex) {
    return SDL_CondWait(cond, ((SDL2Mutex *) mutex)->mutex);
}

int SDL2Cond::signal() {
    return SDL_CondSignal(cond);
}
