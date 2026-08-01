//
// Created by cpasjuste on 11/02/18.
//

#include <sys/time.h>
#include "cross2d/platforms/ps4/ps4_clock.h"

using namespace c2d;

PS4Clock::PS4Clock() : Clock() {
    m_startTime = PS4Clock::getCurrentTime();
}

Time PS4Clock::getCurrentTime() const {
    struct timeval time{};
    gettimeofday(&time, nullptr);
    return microseconds((long) 1000000 * time.tv_sec + time.tv_usec);
}
