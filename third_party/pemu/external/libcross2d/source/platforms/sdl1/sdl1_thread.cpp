//
// Created by cpasjuste on 04/02/2021.
//

#include "cross2d/c2d.h"

using namespace c2d;

SDL1Thread::SDL1Thread(Function fn, void *data) : Thread(fn, data) {
    thread = SDL_CreateThread(fn, data);
}

int SDL1Thread::join() {
    int ret = -1;

    if (thread) {
        SDL_WaitThread(thread, &ret);
        thread = nullptr;
    }

    return ret;
}
