//
// Created by cpasjuste on 04/02/2021.
//

#include "cross2d/c2d.h"

using namespace c2d;

SDL2Thread::SDL2Thread(Function fn, void *data) : Thread(fn, data) {
    thread = SDL_CreateThread(fn, "SDL2Thread", data);
}

int SDL2Thread::join() {
    int ret = 0;

    if (thread) {
        SDL_WaitThread(thread, &ret);
        thread = nullptr;
    }

    return ret;
}
