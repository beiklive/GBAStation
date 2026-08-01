//
// Created by cpasjuste on 12/12/16.
//

#ifndef _DC_AUDIO_H_
#define _DC_AUDIO_H_

#include "cross2d/skeleton/audio.h"

namespace c2d {

    class DCAudio : public Audio {

    public:
        explicit DCAudio(int freq = 48000, int samples = 2048, C2DAudioCallback cb = nullptr);

        ~DCAudio() override;

        void pause(int pause) override;

        void reset() override;

    private:

        c2d::Thread *thread = nullptr;
        bool running = true;
    };
}

#endif //_DC_AUDIO_H_
