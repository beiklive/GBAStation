//
// Created by cpasjuste on 12/12/16.
//

#ifndef _SDL2_AUDIO_H_
#define _SDL2_AUDIO_H_

#include "cross2d/skeleton/audio.h"

namespace c2d {

    class CTRAudio : public Audio {

    public:

        explicit CTRAudio(int rate = 48000, int samples = 2048, C2DAudioCallback cb = nullptr);

        ~CTRAudio() override;

        void pause(int pause) override;

        void reset() override;

    private:
        c2d::Thread *thread = nullptr;
        bool running = true;
    };
}

#endif //_SDL2_AUDIO_H_
