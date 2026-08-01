//
// Created by cpasjuste on 04/02/2021.
//

#ifndef CROSS2D_MUTEX_H
#define CROSS2D_MUTEX_H

#include <cstdio>

namespace c2d {

    class Mutex {

    public:
        Mutex() = default;

        virtual ~Mutex() = default;

        virtual bool lock() {
            printf("c2d::Mutex:lock: unimplemented\n");
            return false;
        };

        virtual bool unlock() {
            printf("c2d::Mutex:unlock: unimplemented\n");
            return false;
        };
    };
}

#endif //CROSS2D_MUTEX_H
