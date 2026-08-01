//
// Created by cpasjuste on 04/02/2021.
//

#ifndef CROSS2D_SDL1_MUTEX_H
#define CROSS2D_SDL1_MUTEX_H

#include "cross2d/skeleton/mutex.h"

namespace c2d {

    class SDL1Mutex : public Mutex {

    public:
        SDL1Mutex();

        ~SDL1Mutex() override;

        bool lock() override;

        bool unlock() override;

    private:
        SDL_mutex *mutex = nullptr;
    };
}

#endif //CROSS2D_SDL1_MUTEX_H
