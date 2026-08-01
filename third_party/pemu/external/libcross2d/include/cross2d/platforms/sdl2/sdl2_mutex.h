//
// Created by cpasjuste on 04/02/2021.
//

#ifndef CROSS2D_SDL2_MUTEX_H
#define CROSS2D_SDL2_MUTEX_H

#include "cross2d/skeleton/mutex.h"

namespace c2d {

    class SDL2Mutex : public Mutex {

    public:
        SDL2Mutex();

        ~SDL2Mutex() override;

        bool lock() override;

        bool unlock() override;

    private:
        SDL_mutex *mutex = nullptr;

        friend class SDL2Cond;
    };
}

#endif //CROSS2D_SDL2_MUTEX_H
