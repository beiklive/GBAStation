//
// Created by cpasjuste on 04/02/2021.
//

#ifndef CROSS2D_SDL2_THREAD_H
#define CROSS2D_SDL2_THREAD_H

#include "cross2d/skeleton/thread.h"

namespace c2d {

    class SDL2Thread : public Thread {

    public:
        SDL2Thread(Function fn, void *data);

        int join() override;

    private:
        SDL_Thread *thread = nullptr;
    };
}

#endif //CROSS2D_SDL2_THREAD_H
