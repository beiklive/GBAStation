//
// Created by cpasjuste on 12/12/16.
//

#if 0

#include <cstdio>
#include <ppu-lv2.h>
#include <sys/thread.h>
#include <audio/audio.h>

#include "ps3/ps3_audio.h"

using namespace c2d;

static unsigned char *buffer_fba;
static int buffer_fba_size;

static int audio_pause = 0;
static int running = 1;
static sys_ppu_thread_t th_id;

static u64 snd_key;
static sys_event_queue_t snd_queue;

static audioPortConfig config;
static u32 portNum;

static void audio_thread(void *arg) {

    f32 *buf;
    s32 ret = 0;
    sys_event_t event;

    printf("audio_thread started\n");

    while (running) {

        u64 current_block = *(u64 * )((u64) config.readIndex);
        f32 *dataStart = (f32 * )((u64) config.audioDataStart);
        u32 audio_block_index = (current_block + 1) % config.numBlocks;

        ret = sysEventQueueReceive(snd_queue, &event, 20 * 1000);

        if (ret == 0 && buffer_fba != NULL && !audio_pause) {

            buf = dataStart + config.channelCount * AUDIO_BLOCK_SAMPLES * audio_block_index;

            static u32 pos = 0;
            for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
                buf[i * 2 + 0] = (f32) * ((s16 * ) & buffer_fba[pos]) / 32768.0f;
                buf[i * 2 + 1] = (f32) * ((s16 * ) & buffer_fba[pos + 2]) / 32768.0f;

                pos += 4;
                if (pos >= buffer_fba_size) {
                    pos = 0;
                }
            }
        }
    }

    sysThreadExit(0);
}

PS3Audio::PS3Audio(int freq, int fps) : Audio(freq, fps) {

    if (!available) {
        return;
    }

    // AUDIO THREAD
    s32 ret;
    u64 prio = 0;
    size_t stacksize = 0x1000;
    char *threadname = (char *) "audio_thread";
    void *threadarg = (void *) 0x1337;
    ret = sysThreadCreate(&th_id, audio_thread, threadarg, prio, stacksize, THREAD_JOINABLE, threadname);
    printf("sysThreadCreate: %d\n", ret);
    // AUDIO THREAD

    // AUDIO INIT
    audioPortParam params;

    ret = audioInit();
    printf("audioInit: %08x\n", ret);

    params.numChannels = AUDIO_PORT_2CH;
    params.numBlocks = AUDIO_BLOCK_8;
    params.attrib = AUDIO_PORT_INITLEVEL;
    params.level = 1.0f;
    ret = audioPortOpen(&params, &portNum);
    printf("audioPortOpen: %08x\n", ret);
    printf("  portNum: %d\n", portNum);

    ret = audioGetPortConfig(portNum, &config);
    printf("audioGetPortConfig: %08x\n", ret);
    printf("config.readIndex: %08x\n", config.readIndex);
    printf("config.status: %d\n", config.status);
    printf("config.channelCount: %ld\n", config.channelCount);
    printf("config.numBlocks: %ld\n", config.numBlocks);
    printf("config.portSize: %d\n", config.portSize);
    printf("config.audioDataStart: %08x\n", config.audioDataStart);

    ret = audioCreateNotifyEventQueue(&snd_queue, &snd_key);
    printf("audioCreateNotifyEventQueue: %08x\n", ret);
    printf("snd_queue: %16lx\n", (long unsigned int) snd_queue);
    printf("snd_key: %16lx\n", snd_key);

    ret = audioSetNotifyEventQueue(snd_key);
    printf("audioSetNotifyEventQueue: %08x\n", ret);

    ret = sysEventQueueDrain(snd_queue);
    printf("sysEventQueueDrain: %08x\n", ret);

    ret = audioPortStart(portNum);
    printf("audioPortStart: %08x\n", ret);
    // AUDIO INIT

}

PS3Audio::~PS3Audio() {

    if (!available) {
        return;
    }

    // AUDIO THREAD
    u64 retval;
    running = false;
    int ret = sysThreadJoin(th_id, &retval);
    printf("sysThreadJoin: %d - %llX\n", ret, (unsigned long long int) retval);
    // AUDIO THREAD

    // AUDIO EXIT
    ret = audioPortStop(portNum);
    printf("audioPortStop: %08x\n", ret);

    ret = audioRemoveNotifyEventQueue(snd_key);
    printf("audioRemoveNotifyEventQueue: %08x\n", ret);

    ret = audioPortClose(portNum);
    printf("audioPortClose: %08x\n", ret);

    ret = sysEventQueueDestroy(snd_queue, 0);
    printf("sysEventQueueDestroy: %08x\n", ret);

    ret = audioQuit();
    printf("audioQuit: %08x\n", ret);
    // AUDIO EXIT
}

void PS3Audio::Play() {

    if (!available) {
        return;
    }

    buffer_fba = (unsigned char *) buffer;
    buffer_fba_size = buffer_size;
}

void PS3Audio::Pause(int pause) {

    if (!available) {
        return;
    }

    audio_pause = pause;
}

#endif