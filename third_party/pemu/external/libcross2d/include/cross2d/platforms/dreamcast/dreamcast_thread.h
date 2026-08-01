//
// Created by cpasjuste on 04/02/2021.
//

#ifndef CROSS2D_DC_THREAD_H
#define CROSS2D_DC_THREAD_H

#include "cross2d/skeleton/thread.h"

namespace c2d {

    class DCThread : public Thread {

    public:
        DCThread(Function fn, void *data);

        ~DCThread() override;

        int join() override;

    private:
        kthread_t *thread = nullptr;
    };
}

#endif //CROSS2D_DC_THREAD_H
