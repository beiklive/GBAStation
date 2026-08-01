//
// Created by cpasjuste on 11/02/18.
//

#ifndef C2D_POSIX_CLOCK_H
#define C2D_POSIX_CLOCK_H

#include "cross2d/skeleton/sfml/Clock.hpp"

namespace c2d {

    class POSIXClock : public Clock {

    public:

        POSIXClock();

        Time getCurrentTime() const;
    };

}

#endif //C2D_POSIX_CLOCK_H
