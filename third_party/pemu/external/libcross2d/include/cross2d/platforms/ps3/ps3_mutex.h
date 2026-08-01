//
// Created by cpasjuste on 04/02/2021.
//

#ifndef CROSS2D_PS3_MUTEX_H
#define CROSS2D_PS3_MUTEX_H

#include "cross2d/skeleton/mutex.h"
#include "sys/mutex.h"

namespace c2d {

    class PS3Mutex : public Mutex {

    public:
        PS3Mutex();

        ~PS3Mutex() override;

        bool lock() override;

        bool unlock() override;

    private:
        sys_mutex_t mutex;
        bool available = false;
    };
}

#endif //CROSS2D_PS3_MUTEX_H
