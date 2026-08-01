//
// Created by cpasjuste on 04/02/2021.
//

#include "cross2d/c2d.h"

using namespace c2d;

DCThread::DCThread(Function fn, void *data) : Thread(fn, data) {
    thread = thd_create(0, reinterpret_cast<void *(*)(void *)>(fn), data);
}

int DCThread::join() {
    int ret = -1;

    if (thread) {
        ret = thd_join(thread, nullptr);
        thread = nullptr;
    }

    return ret;
}

DCThread::~DCThread() {
    if (thread) {
        thd_destroy(thread);
    }
}
