//
// Created by cpasjuste on 21/10/18.
//

#include <3ds.h>

#include "cross2d/platforms/3ds/ctr_clock.h"

using namespace c2d;

#define TICKS_PER_MSEC (268123.480)

CTRClock::CTRClock() : Clock() {

    m_startTime = getCurrentTime();
}

Time CTRClock::getCurrentTime() const {

    return milliseconds((int) (svcGetSystemTick() / TICKS_PER_MSEC));
}
