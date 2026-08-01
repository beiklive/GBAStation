//
// Created by cpasjuste on 04/02/2021.
//

#include "cross2d/c2d.h"

c2d::CTRThread::CTRThread(Function fn, void *data) : c2d::Thread(fn, data) {
    s32 priority = 0;
    svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
    thread = threadCreate((ThreadFunc) fn, data,
                          CTR_THREAD_STACK_SIZE, priority - 1, -2, false);
}

int c2d::CTRThread::join() {
    Result rc = threadJoin((Thread_tag *) thread, U64_MAX);
    return R_SUCCEEDED(rc);
}

c2d::CTRThread::~CTRThread() {
    threadFree((Thread_tag *) thread);
}
