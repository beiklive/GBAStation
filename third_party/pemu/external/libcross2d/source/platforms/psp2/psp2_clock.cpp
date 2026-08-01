//
// Created by cpasjuste on 11/02/18.
//

#include <sys/time.h>
#include "cross2d/platforms/psp2/psp2_clock.h"

using namespace c2d;

PSP2Clock::PSP2Clock() : Clock() {
    m_startTime = PSP2Clock::getCurrentTime();
}

Time PSP2Clock::getCurrentTime() const {

    struct timeval time{};
    gettimeofday(&time, nullptr);
    return microseconds((long) 1000000 * time.tv_sec + time.tv_usec);
}
