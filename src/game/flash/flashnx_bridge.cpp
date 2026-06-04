#include "flashnx_bridge.h"
#include <cstdio>
#include <ctime>
#include <chrono>

#ifdef __SWITCH__
#include <switch.h>
#include <borealis.hpp>
#else
#include <borealis.hpp>
#endif

extern "C" {

void ruffle_log_cstr(const char* msg)
{
    if (msg) {
        fprintf(stdout, "%s", msg);
        fflush(stdout);
        brls::Logger::debug("FlashNX: {}", msg);
    }
}

void ruffle_crash_dump(const char* msg)
{
    if (msg)
        fprintf(stderr, "%s", msg);
    fflush(stderr);
#ifdef __SWITCH__
    FILE* f = fopen("sdmc:/switch/ruffle-crash.log", "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
    svcSleepThread(150000000);
#else
    fflush(stdout);
#endif
}

int ruffle_query_ram(uint64_t* used_out, uint64_t* total_out)
{
#ifdef __SWITCH__
    uint64_t used = 0, total = 0;
    Result rc = svcGetInfo(&used, InfoType_UsedMemorySize, INVALID_HANDLE, 0);
    if (R_FAILED(rc)) return -1;
    rc = svcGetInfo(&total, InfoType_TotalMemorySize, INVALID_HANDLE, 0);
    if (R_FAILED(rc)) return -1;
    if (used_out) *used_out = used;
    if (total_out) *total_out = total;
    return 0;
#else
    if (used_out) *used_out = 0;
    if (total_out) *total_out = 1024 * 1024 * 1024;
    return 0;
#endif
}

uint64_t ruffle_tick_now()
{
#ifdef __SWITCH__
    return armGetSystemTick();
#else
    static auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
#endif
}

uint64_t ruffle_tick_freq()
{
#ifdef __SWITCH__
    return armGetSystemTickFreq();
#else
    return 1'000'000;
#endif
}

int ruffle_audio_init(unsigned sample_rate, unsigned channels)
{
    (void)sample_rate; (void)channels;
    return 0;
}

void ruffle_audio_shutdown() {}
void ruffle_audio_play() {}
void ruffle_audio_pause() {}

}
