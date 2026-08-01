//
// Created by cpasjuste on 12/12/16.
//

#include <cstdio>
#include <malloc.h>
#include "cross2d/c2d.h"

using namespace c2d;

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
static LightEvent s_event;
static ndspWaveBuf s_waveBufs[3];
static int16_t *s_audioBuffer = nullptr;
volatile bool s_quit = false;

// NDSP audio frame callback
// This signals the audioThread to decode more things
// once NDSP has played a sound frame, meaning that there should be
// one or more available waveBufs to fill with more data.
static void audioCallback(void *const nul_) {
    (void) nul_;  // Unused
    if (s_quit) { // Quit flag
        return;
    }
    LightEvent_Signal(&s_event);
}

// Audio thread
// This handles calling the decoder function to fill NDSP buffers as necessary
static int audioThread(void *data) {
    auto *audio = (CTRAudio *) data;

    while (!s_quit && audio->isAvailable()) {  // Whilst the quit flag is unset,
        // search our waveBufs and fill any that aren't currently
        // queued for playback (i.e, those that are 'done')
        for (size_t i = 0; i < ARRAY_SIZE(s_waveBufs); ++i) {
            if (s_waveBufs[i].status != NDSP_WBUF_DONE) {
                continue;
            }
            ndspWaveBuf *waveBuf = &s_waveBufs[i];
            if (audio->getSampleBufferQueued() >= audio->getSamplesSize() >> 1) {
                audio->lock();
                audio->getSampleBuffer()->pull((int16_t *) waveBuf->data_pcm16, audio->getSamplesSize() >> 1);
                audio->unlock();
                // Pass samples to NDSP
                waveBuf->nsamples = audio->getSamples();
                ndspChnWaveBufAdd(0, waveBuf);
                DSP_FlushDataCache(waveBuf->data_pcm16, audio->getSamplesSize());
            }
        }

        // Wait for a signal that we're needed again before continuing,
        // so that we can yield to other things that want to run
        // (Note that the 3DS uses cooperative threading)
        LightEvent_Wait(&s_event);
    }

    svcExitThread();
}

CTRAudio::CTRAudio(int rate, int samples, C2DAudioCallback cb) : Audio(rate, samples, cb) {
    // Setup LightEvent for synchronisation of audioThread
    LightEvent_Init(&s_event, RESET_ONESHOT);

    Result rc = ndspInit();
    if (R_FAILED(rc)) {
        if ((R_SUMMARY(rc) == RS_NOTFOUND) && (R_MODULE(rc) == RM_DSP)) {
            printf("CTRAudio: DSP init failed: dspfirm.cdc missing!\n");
        } else {
            printf("CTRAudio: DSP init failed. Error code: 0x%lx\n", rc);
        }
        available = false;
        return;
    }

    ndspChnReset(0);
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, (float) rate);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);

    // Setup waveBufs for NDSP
    s_audioBuffer = (int16_t *) linearAlloc(getSamplesSize() * ARRAY_SIZE(s_waveBufs));
    memset(&s_waveBufs, 0, sizeof(s_waveBufs));
    int16_t *s_buffer = s_audioBuffer;
    for (size_t i = 0; i < ARRAY_SIZE(s_waveBufs); ++i) {
        s_waveBufs[i].data_vaddr = s_buffer;
        s_waveBufs[i].status = NDSP_WBUF_DONE;
        s_buffer += getSamplesSize() / sizeof(s_buffer[0]);
    }

    // Set the ndsp sound frame callback which signals our audioThread
    ndspSetCallback(audioCallback, nullptr);

    // start audio thread callback
    s_quit = false;
    thread = new C2DThread(audioThread, this);

    available = true;
    paused = true;
}

void CTRAudio::pause(int pause) {
    Audio::pause(pause);

    if (available) {
        ndspChnSetPaused(0, pause == 1);
    }
}

void CTRAudio::reset() {
    if (available) {
        ndspChnReset(0);
    }

    Audio::reset();
}

CTRAudio::~CTRAudio() {
    if (!available) {
        return;
    }

    s_quit = true;
    available = false;
    LightEvent_Signal(&s_event);

    if (thread) {
        thread->join();
        delete (thread);
    }

    if (s_audioBuffer) {
        linearFree(s_audioBuffer);
    }

    ndspExit();
}
