//
// Created by cpasjuste on 04/02/2021.
//

#include "cross2d/c2d.h"

using namespace c2d;

SDL2Mutex::SDL2Mutex() : Mutex() {
    mutex = SDL_CreateMutex();
}

bool SDL2Mutex::lock() {

    if (!mutex) {
        printf("c2d::SDL2Mutex::lock: mutex is null\n");
        return false;
    }

    if (SDL_LockMutex(mutex) == 0) {
        return true;
    }

    return false;
}

bool SDL2Mutex::unlock() {

    if (!mutex) {
        printf("c2d::SDL2Mutex::unlock: mutex is null\n");
        return false;
    }

    if (SDL_UnlockMutex(mutex) == 0) {
        return true;
    }

    return false;
}

SDL2Mutex::~SDL2Mutex() {
    if (mutex) {
        SDL_DestroyMutex(mutex);
    }
}
