//
// Created by cpasjuste on 04/02/2021.
//

#ifndef C2D_COND_H
#define C2D_COND_H

#include <cstdio>

namespace c2d {

    class Cond {

    public:
        Cond() = default;

        virtual ~Cond() = default;

        virtual int wait(Mutex *mutex) {
            printf("c2d::Cond:wait: unimplemented\n");
            return -1;
        };

        virtual int signal() {
            printf("c2d::Cond:signal: unimplemented\n");
            return -1;
        };
    };
}

#endif //C2D_COND_H
