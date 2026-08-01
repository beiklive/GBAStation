//
// Created by cpasjuste on 11/02/18.
//

#ifndef __C2D_PS4_CLOCK_H__
#define __C2D_PS4_CLOCK_H__

#include "cross2d/skeleton/sfml/Clock.hpp"

namespace c2d {

    class PS4Clock : public Clock {

    public:

        PS4Clock();

        Time getCurrentTime() const override;
    };

}

#endif //__C2D_PS4_CLOCK_H__
