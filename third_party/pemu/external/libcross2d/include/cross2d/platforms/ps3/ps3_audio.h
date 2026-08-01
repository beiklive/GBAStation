//
// Created by cpasjuste on 12/12/16.
//

#ifndef _PS3_AUDIO_H_
#define _PS3_AUDIO_H_

#include "cross2d/skeleton/audio.h"

namespace c2d {

    class PS3Audio : Audio {

    public:

        PS3Audio(int freq, int fps = 60);

        virtual ~PS3Audio();

        virtual void Play();

        virtual void Pause(int pause);

    };
}

#endif //_SDL2_AUDIO_H_
