//
// Created by cpasjuste on 15/09/18.
//

#include <unistd.h>
#include "cross2d/c2d.h"

#if !defined(NDEBUG) && !defined(__DEBUG_SVC__)
static int sock = -1;
void deinitNxLink();
#endif

void initNxLink() {
#ifndef NDEBUG
#ifdef __DEBUG_SVC__
    consoleDebugInit(debugDevice_SVC);
    stdout = stderr;
#else
    socketInitializeDefault();
    sock = nxlinkStdio();
    if (sock < 0) {
        socketExit();
        return;
    }

    std::atexit(deinitNxLink);
#endif
#endif
}

void deinitNxLink() {
#ifndef NDEBUG
#ifndef __DEBUG_SVC__
    if (sock >= 0) {
        close(sock);
        socketExit();
        sock = -1;
    }
#endif
#endif
}
