//
// Created by cpasjuste on 04/02/2021.
//

#ifndef C2D_CTR_THREAD_H
#define C2D_CTR_THREAD_H

#define CTR_THREAD_STACK_SIZE 0x10000

namespace c2d {

    class CTRThread : public Thread {

    public:
        CTRThread(Function fn, void *data);

        ~CTRThread() override;

        int join() override;

    private:
        void *thread = nullptr;
    };
}

#endif //C2D_CTR_THREAD_H
