//
// Created by cpasjuste on 21/10/18.
//

#ifndef C2D_CTR_CLOCK_H
#define C2D_CTR_CLOCK_H

#include "cross2d/skeleton/sfml/Clock.hpp"

namespace c2d {

    class CTRClock : public Clock {

    public:

        CTRClock();

        Time getCurrentTime() const override;
    };

}


#endif //C2D_CTR_CLOCK_H
