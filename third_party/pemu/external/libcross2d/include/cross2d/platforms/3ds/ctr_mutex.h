//
// Created by cpasjuste on 04/02/2021.
//

#ifndef C2D_CTR_MUTEX_H
#define C2D_CTR_MUTEX_H

#include "cross2d/skeleton/mutex.h"

namespace c2d {

    class CTRMutex : public Mutex {

    public:
        CTRMutex();

        ~CTRMutex() override;

        bool lock() override;

        bool unlock() override;

    private:
        Handle handle;
    };
}

#endif //C2D_CTR_MUTEX_H
