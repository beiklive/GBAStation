//
// Created by cpasjuste on 04/02/2021.
//

#include "cross2d/c2d.h"

using namespace c2d;

SDL1Mutex::SDL1Mutex() : Mutex() {
    mutex = SDL_CreateMutex();
}

bool SDL1Mutex::lock() {

    if (!mutex) {
        printf("c2d::SDL2Mutex::lock: mutex is null\n");
        return false;
    }

    if (SDL_LockMutex(mutex) == 0) {
        return true;
    }

    return false;
}

bool SDL1Mutex::unlock() {

    if (!mutex) {
        printf("c2d::SDL2Mutex::unlock: mutex is null\n");
        return false;
    }

    if (SDL_UnlockMutex(mutex) == 0) {
        return true;
    }

    return false;
}

SDL1Mutex::~SDL1Mutex() {
    if (mutex) {
        SDL_DestroyMutex(mutex);
    }
}
