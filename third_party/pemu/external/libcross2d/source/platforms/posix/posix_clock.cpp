//
// Created by cpasjuste on 11/02/18.
//

#include <ctime>
#include "cross2d/platforms/posix/posix_clock.h"

using namespace c2d;

POSIXClock::POSIXClock() : Clock() {
    m_startTime = POSIXClock::getCurrentTime();
}

Time POSIXClock::getCurrentTime() const {
    timespec time = {};
    clock_gettime(CLOCK_MONOTONIC, &time);
    return microseconds((long) time.tv_sec * 1000000 + time.tv_nsec / 1000);
}
