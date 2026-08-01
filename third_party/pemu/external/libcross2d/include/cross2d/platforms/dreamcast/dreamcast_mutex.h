//
// Created by cpasjuste on 04/02/2021.
//

#ifndef CROSS2D_DC_MUTEX_H
#define CROSS2D_DC_MUTEX_H

#include "cross2d/skeleton/mutex.h"

namespace c2d {

    class DCMutex : public Mutex {

    public:
        DCMutex();

        ~DCMutex() override;

        bool lock() override;

        bool unlock() override;

    private:
        mutex_t mutex{};
    };
}

#endif //CROSS2D_DC_MUTEX_H
