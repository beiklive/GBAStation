#pragma once

#include <mutex>

namespace beiklive::audio
{
    // libnx audout has one completion queue for the process.  Every producer
    // must keep append/wait paired, otherwise another thread can receive its
    // completion and free a DMA buffer too early.
    inline std::mutex switchAudioOutMutex;
}
