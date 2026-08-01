//
// Created by cpasjuste on 11/02/18.
//

#ifndef CROSS2D_PS3_CLOCK_H
#define CROSS2D_PS3_CLOCK_H

#include "cross2d/skeleton/sfml/Clock.hpp"

namespace c2d {
    class PS3Clock : public Clock {
    public:
        PS3Clock();

        Time getCurrentTime() const override;
    };
}

#endif //CROSS2D_PS3_CLOCK_H
