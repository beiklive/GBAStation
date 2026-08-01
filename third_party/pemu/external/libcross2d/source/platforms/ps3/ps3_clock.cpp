//
// Created by cpasjuste on 11/02/18.
//

#include <sys/time.h>
#include "cross2d/c2d.h"

using namespace c2d;

PS3Clock::PS3Clock() : Clock() {
    m_startTime = PS3Clock::getCurrentTime();
}

Time PS3Clock::getCurrentTime() const {
    struct timeval time{};
    gettimeofday(&time, nullptr);
    return microseconds((long) 1000000 * time.tv_sec + time.tv_usec);
}
