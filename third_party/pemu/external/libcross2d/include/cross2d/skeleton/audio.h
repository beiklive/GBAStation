//
// Created by cpasjuste on 12/12/16.
//

#ifndef C2D_AUDIO_H
#define C2D_AUDIO_H

#include <cstring>
#include "cross2d/skeleton/mutex.h"
#include "audio_buffer.h"

namespace c2d {

    class Audio {

    public:

        enum SyncMode {
            None = 0,
            LowLatency = 1,
            Safe = 2
        };

        typedef void (*C2DAudioCallback)(void *data, unsigned char *stream, int len);

        Audio(int rate = 48000, int samples = 1024, C2DAudioCallback cb = nullptr);

        virtual ~Audio();

        virtual void play(const void *data, int samples, SyncMode syncMode = None);

        virtual void pause(int pause);

        virtual void reset();

        virtual bool isPaused() {
            return paused;
        }

        bool lock();

        bool unlock();

        bool isAvailable();

        int getChannels();

        int getSampleRate();

        int getSamples();

        int getSamplesSize();

        SampleBuffer *getSampleBuffer();

        int getSampleBufferQueued();

        int getSampleBufferCapacity();

        int getSampleBufferAvailable();

        static int toSamples(int rate, float fps);

    protected:

        int m_sample_rate = 48000;
        int channels = 2;
        SampleBuffer *m_buffer = nullptr;
        int m_samples = 0;
        int m_samples_size = 0;
        bool paused = false;
        bool available = false;
        C2DAudioCallback callback = nullptr;

    private:
        c2d::Mutex *mutex = nullptr;
    };
}

#endif //C2D_AUDIO_H
