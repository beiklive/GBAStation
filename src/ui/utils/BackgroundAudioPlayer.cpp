#include "ui/utils/BackgroundAudioPlayer.hpp"
#include "core/common.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

#ifdef __SWITCH__
#include <switch.h>
#elif defined(BK_AUDIO_ALSA)
#include <alsa/asoundlib.h>
#elif defined(BK_AUDIO_WINMM)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#elif defined(BK_AUDIO_COREAUDIO)
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudio.h>
#endif

namespace beiklive {

std::atomic<int> BackgroundAudioPlayer::s_activePlayers{0};
std::atomic<BackgroundAudioPlayer*> BackgroundAudioPlayer::s_activePlayer{nullptr};

namespace {
struct PlatformState {
#ifdef __SWITCH__
    static constexpr int Count = 4;
    int16_t* data[Count]{};
    AudioOutBuffer buffers[Count]{};
    bool queued[Count]{};
    int enqueued = 0;
    bool started = false;
#elif defined(BK_AUDIO_WINMM)
    static constexpr int Count = 3;
    HWAVEOUT wave = nullptr;
    HANDLE event = nullptr;
    WAVEHDR headers[Count]{};
    int16_t data[Count][BackgroundAudioPlayer::BLOCK_FRAMES * 2]{};
#elif defined(BK_AUDIO_ALSA)
    snd_pcm_t* pcm = nullptr;
#elif defined(BK_AUDIO_COREAUDIO)
    AudioUnit unit = nullptr;
    BackgroundAudioPlayer* owner = nullptr;
#endif
};

#ifdef BK_AUDIO_WINMM
void CALLBACK waveCallback(HWAVEOUT, UINT msg, DWORD_PTR instance, DWORD_PTR, DWORD_PTR)
{
    if (msg == WOM_DONE) {
        auto* state = reinterpret_cast<PlatformState*>(instance);
        if (state && state->event)
            SetEvent(state->event);
    }
}
#endif

#ifdef BK_AUDIO_COREAUDIO
OSStatus audioCallback(void* refCon, AudioUnitRenderActionFlags*, const AudioTimeStamp*,
                       UInt32, UInt32 frames, AudioBufferList* ioData)
{
    auto* player = static_cast<BackgroundAudioPlayer*>(refCon);
    auto* output = static_cast<int16_t*>(ioData->mBuffers[0].mData);
    const size_t samples = static_cast<size_t>(frames) * 2;
    const size_t got = player->readSamples(output, samples);
    if (got < samples)
        std::memset(output + got, 0, (samples - got) * sizeof(int16_t));
    return noErr;
}
#endif
} // namespace

bool BackgroundAudioPlayer::isAnyActive()
{
    return s_activePlayers.load(std::memory_order_acquire) > 0;
}

bool BackgroundAudioPlayer::mixActiveSamples(const int16_t* data, size_t frames)
{
    auto* player = s_activePlayer.load(std::memory_order_acquire);
    if (!player || !player->isRunning())
        return false;
    player->pushOverlaySamples(data, frames);
    return true;
}

std::mutex& BackgroundAudioPlayer::switchOutputMutex()
{
    static std::mutex mutex;
    return mutex;
}

void BackgroundAudioPlayer::resetLocked()
{
    m_ring.assign(RING_CAPACITY, 0);
    m_readPos = m_writePos = m_available = 0;
    m_overlay.clear();
    m_lastOutput = {0, 0};
    m_needsFadeIn = false;
}

size_t BackgroundAudioPlayer::readSamples(int16_t* out, size_t count)
{
    if (!out || count == 0)
        return 0;
    std::lock_guard<std::mutex> lock(m_mutex);
    const size_t n = std::min(count, m_available);
    const float gain = std::clamp(m_volume.load(std::memory_order_acquire), 0.0f, 2.0f);
    size_t fadeInFrames = 0;
    for (size_t i = 0; i < n; ++i) {
        const int channel = static_cast<int>(i & 1u);
        float value = static_cast<float>(m_ring[m_readPos++]) * gain;
        if (gain > 1.0f && std::fabs(value) > 0.95f * 32767.0f) {
            constexpr float knee = 0.95f * 32767.0f;
            constexpr float range = 32767.0f - knee;
            const float magnitude = knee + range * std::tanh((std::fabs(value) - knee) / range);
            value = std::copysign(magnitude, value);
        }
        int sample = static_cast<int>(std::lround(value));
        sample = std::clamp(sample, -32768, 32767);
        if (m_needsFadeIn) {
            fadeInFrames = std::max(fadeInFrames, i / 2 + 1);
            const float factor = std::min(1.0f,
                static_cast<float>(i / 2 + 1) / static_cast<float>(RAMP_FRAMES));
            sample = static_cast<int>(std::lround(static_cast<float>(sample) * factor));
        }
        out[i] = static_cast<int16_t>(sample);
        m_lastOutput[static_cast<size_t>(channel)] = out[i];
        if (m_readPos == m_ring.size()) m_readPos = 0;
    }
    m_available -= n;
    if (n > 0)
        m_spaceCv.notify_all();
    if (m_needsFadeIn && fadeInFrames >= RAMP_FRAMES)
        m_needsFadeIn = false;

    if (n < count) {
        const size_t missing = count - n;
        for (size_t i = 0; i < missing; ++i) {
            const size_t frame = i / 2;
            const int channel = static_cast<int>(i & 1u);
            const float factor = frame < RAMP_FRAMES
                ? 1.0f - static_cast<float>(frame + 1) / static_cast<float>(RAMP_FRAMES)
                : 0.0f;
            out[n + i] = static_cast<int16_t>(std::lround(
                static_cast<float>(m_lastOutput[static_cast<size_t>(channel)]) * factor));
        }
        m_needsFadeIn = true;
    }
    for (size_t i = 0; i < count && !m_overlay.empty(); ++i) {
        const int mixed = static_cast<int>(out[i]) + static_cast<int>(m_overlay.front());
        out[i] = static_cast<int16_t>(std::clamp(mixed, -32768, 32767));
        m_overlay.pop_front();
    }
    return n;
}

void BackgroundAudioPlayer::setVolume(float volume)
{
    m_volume.store(std::clamp(volume, 0.0f, 2.0f), std::memory_order_release);
}

void BackgroundAudioPlayer::waitForInitialBuffer()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
    m_cv.wait_until(lock, deadline, [this]() {
        return !m_running.load(std::memory_order_acquire) || m_available >= PREBUFFER_SAMPLES;
    });
}

void BackgroundAudioPlayer::pushOverlaySamples(const int16_t* data, size_t frames)
{
    if (!data || frames == 0 || !m_running.load(std::memory_order_acquire))
        return;
    const size_t count = frames * 2;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running.load(std::memory_order_acquire))
        return;
    for (size_t i = 0; i < count; ++i) {
        if (m_overlay.size() >= OVERLAY_CAPACITY)
            m_overlay.pop_front();
        m_overlay.push_back(data[i]);
    }
}

bool BackgroundAudioPlayer::start(int sampleRate, int channels)
{
    if (m_running.load(std::memory_order_acquire))
        return true;
    m_sampleRate = std::max(1, sampleRate);
    m_channels = std::max(1, channels);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        resetLocked();
    }

    auto* state = new PlatformState();
    m_platformState = state;
#ifdef __SWITCH__
    // BKAudioPlayer owns the process-wide audout service. The background
    // player only contributes its own buffers to that already-running stream;
    // it never starts or stops the service itself.
    for (int i = 0; i < PlatformState::Count; ++i) {
        state->data[i] = static_cast<int16_t*>(std::aligned_alloc(0x1000, BLOCK_FRAMES * 2 * sizeof(int16_t)));
        if (!state->data[i]) { stop(); return false; }
        std::memset(state->data[i], 0, BLOCK_FRAMES * 2 * sizeof(int16_t));
        state->buffers[i].buffer = state->data[i];
        state->buffers[i].buffer_size = BLOCK_FRAMES * 2 * sizeof(int16_t);
        state->buffers[i].data_size = state->buffers[i].buffer_size;
    }
#elif defined(BK_AUDIO_WINMM)
    state->event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 2;
    format.nSamplesPerSec = static_cast<DWORD>(m_sampleRate);
    format.wBitsPerSample = 16;
    format.nBlockAlign = 4;
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    if (!state->event || waveOutOpen(&state->wave, WAVE_MAPPER, &format,
                                     reinterpret_cast<DWORD_PTR>(waveCallback),
                                     reinterpret_cast<DWORD_PTR>(state), CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
        stop(); return false;
    }
    for (int i = 0; i < PlatformState::Count; ++i) {
        state->headers[i].lpData = reinterpret_cast<LPSTR>(state->data[i]);
        state->headers[i].dwBufferLength = static_cast<DWORD>(sizeof(state->data[i]));
        waveOutPrepareHeader(state->wave, &state->headers[i], sizeof(WAVEHDR));
        state->headers[i].dwFlags |= WHDR_DONE;
    }
#elif defined(BK_AUDIO_ALSA)
    if (snd_pcm_open(&state->pcm, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0 ||
        snd_pcm_set_params(state->pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
                           2, static_cast<unsigned>(m_sampleRate), 1, 100000) < 0) {
        stop(); return false;
    }
#elif defined(BK_AUDIO_COREAUDIO)
    state->owner = this;
    AudioComponentDescription description{};
    description.componentType = kAudioUnitType_Output;
    description.componentSubType = kAudioUnitSubType_DefaultOutput;
    description.componentManufacturer = kAudioUnitManufacturer_Apple;
    AudioComponent component = AudioComponentFindNext(nullptr, &description);
    if (!component || AudioComponentInstanceNew(component, &state->unit) != noErr) { stop(); return false; }
    AURenderCallbackStruct callback{audioCallback, this};
    AudioUnitSetProperty(state->unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &callback, sizeof(callback));
    AudioStreamBasicDescription format{};
    format.mSampleRate = m_sampleRate;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    format.mFramesPerPacket = 1;
    format.mChannelsPerFrame = 2;
    format.mBitsPerChannel = 16;
    format.mBytesPerFrame = 4;
    format.mBytesPerPacket = 4;
    AudioUnitSetProperty(state->unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &format, sizeof(format));
    if (AudioUnitInitialize(state->unit) != noErr || AudioOutputUnitStart(state->unit) != noErr) { stop(); return false; }
#endif
    m_running.store(true, std::memory_order_release);
    s_activePlayer.store(this, std::memory_order_release);
    m_registered.store(true, std::memory_order_release);
    s_activePlayers.fetch_add(1, std::memory_order_acq_rel);
#ifndef BK_AUDIO_COREAUDIO
    m_thread = std::thread(&BackgroundAudioPlayer::audioThread, this);
#endif
    return true;
}

void BackgroundAudioPlayer::pushSamples(const int16_t* data, size_t frames)
{
    if (!data || frames == 0 || !m_running.load(std::memory_order_acquire))
        return;
    const size_t count = frames * 2;
    std::unique_lock<std::mutex> lock(m_mutex);
    if (count >= RING_CAPACITY) {
        data += count - RING_CAPACITY;
        m_readPos = m_writePos = m_available = 0;
    }
    const size_t writeCount = std::min(count, RING_CAPACITY);
    if (count < RING_CAPACITY) {
        m_spaceCv.wait(lock, [this, count]() {
            return !m_running.load(std::memory_order_acquire) ||
                m_available + count <= RING_CAPACITY;
        });
        if (!m_running.load(std::memory_order_acquire))
            return;
    }
    for (size_t i = 0; i < writeCount; ++i) {
        m_ring[m_writePos++] = data[i];
        if (m_writePos == m_ring.size()) m_writePos = 0;
        if (m_available < RING_CAPACITY) ++m_available;
        else { m_readPos = (m_readPos + 1) % m_ring.size(); }
    }
    m_cv.notify_one();
}

void BackgroundAudioPlayer::audioThread()
{
    // Let the decoder build a short queue before touching the device. This
    // prevents the first hardware blocks from being silence while FFmpeg is
    // still opening and demuxing the audio stream.
    waitForInitialBuffer();
#ifdef __SWITCH__
    auto* state = static_cast<PlatformState*>(m_platformState);
    while (m_running.load(std::memory_order_acquire)) {
        std::lock_guard<std::mutex> outputLock(switchOutputMutex());
        AudioOutBuffer* released = nullptr; u32 count = 0;
        audoutWaitPlayFinish(&released, &count, 0);
        for (AudioOutBuffer* item = released; item; item = item->next)
            for (int i = 0; i < PlatformState::Count; ++i)
                if (item == &state->buffers[i] && state->queued[i]) {
                    state->queued[i] = false;
                    if (state->enqueued > 0) --state->enqueued;
                }
        // Completion notifications are process-global. AudioManager or a UI
        // sound may consume ours first, so reconcile every owned slot against
        // the hardware queue before deciding which buffers are reusable.
        for (int i = 0; i < PlatformState::Count; ++i) {
            if (!state->queued[i])
                continue;
            bool contains = true;
            if (R_SUCCEEDED(audoutContainsAudioOutBuffer(&state->buffers[i], &contains)) && !contains) {
                state->queued[i] = false;
                if (state->enqueued > 0) --state->enqueued;
            }
        }
        for (int i = 0; i < PlatformState::Count; ++i)
            if (!state->queued[i] && state->buffers[i].buffer) {
                readSamples(state->data[i], BLOCK_FRAMES * 2);
                armDCacheFlush(state->data[i], BLOCK_FRAMES * 2 * sizeof(int16_t));
                if (R_SUCCEEDED(audoutAppendAudioOutBuffer(&state->buffers[i]))) {
                    state->queued[i] = true;
                    ++state->enqueued;
                }
            }
        svcSleepThread(5'000'000);
    }
#elif defined(BK_AUDIO_WINMM)
    auto* state = static_cast<PlatformState*>(m_platformState);
    int next = 0;
    while (m_running.load(std::memory_order_acquire)) {
        auto& header = state->headers[next];
        while (!(header.dwFlags & WHDR_DONE) && m_running.load(std::memory_order_acquire)) WaitForSingleObject(state->event, 10);
        if (!m_running.load(std::memory_order_acquire)) break;
        readSamples(state->data[next], BLOCK_FRAMES * 2);
        header.dwFlags &= ~WHDR_DONE;
        waveOutWrite(state->wave, &header, sizeof(WAVEHDR));
        next = (next + 1) % PlatformState::Count;
    }
#elif defined(BK_AUDIO_ALSA)
    auto* state = static_cast<PlatformState*>(m_platformState);
    std::vector<int16_t> block(BLOCK_FRAMES * 2);
    while (m_running.load(std::memory_order_acquire)) {
        readSamples(block.data(), block.size());
        snd_pcm_sframes_t written = snd_pcm_writei(state->pcm, block.data(), BLOCK_FRAMES);
        if (written < 0) snd_pcm_prepare(state->pcm);
    }
#else
    std::vector<int16_t> block(BLOCK_FRAMES * 2);
    while (m_running.load(std::memory_order_acquire)) {
        readSamples(block.data(), block.size());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
#endif
}

void BackgroundAudioPlayer::stop()
{
    const bool wasRunning = m_running.exchange(false, std::memory_order_acq_rel);
    m_cv.notify_all();
    m_spaceCv.notify_all();

#ifndef BK_AUDIO_COREAUDIO
    if (wasRunning && m_thread.joinable())
        m_thread.join();
#endif

    auto* state = static_cast<PlatformState*>(m_platformState);
    if (state) {
#ifdef __SWITCH__
        // audoutWaitPlayFinish() returns process-global completion events. A
        // different audio producer may consume the notification first, so
        // verify each of our buffers with audoutContainsAudioOutBuffer before
        // releasing the backing memory.
        constexpr u64 drainTimeoutNs = 16000000ULL;
        constexpr int maxRetries = PlatformState::Count * 8;
        auto collectReleased = [state](AudioOutBuffer* released) {
            for (AudioOutBuffer* item = released; item; item = item->next)
                for (int i = 0; i < PlatformState::Count; ++i)
                    if (item == &state->buffers[i] && state->queued[i]) {
                        state->queued[i] = false;
                        if (state->enqueued > 0) --state->enqueued;
                    }
        };
        for (int retry = 0; state->enqueued > 0 && retry < maxRetries; ++retry) {
            std::lock_guard<std::mutex> outputLock(switchOutputMutex());
            AudioOutBuffer* released = nullptr; u32 count = 0;
            audoutWaitPlayFinish(&released, &count, drainTimeoutNs);
            collectReleased(released);
        }
        for (int retry = 0; state->enqueued > 0 && retry < 200; ++retry) {
            bool pending = false;
            {
                std::lock_guard<std::mutex> outputLock(switchOutputMutex());
                for (int i = 0; i < PlatformState::Count; ++i) {
                    if (!state->queued[i])
                        continue;
                    bool contains = true;
                    if (R_SUCCEEDED(audoutContainsAudioOutBuffer(&state->buffers[i], &contains))) {
                        if (!contains) {
                            state->queued[i] = false;
                            if (state->enqueued > 0) --state->enqueued;
                        } else {
                            pending = true;
                        }
                    } else {
                        pending = true;
                    }
                }
            }
            if (!pending || state->enqueued == 0)
                break;
            svcSleepThread(10000000ULL); // 10ms
        }
        if (state->enqueued > 0) {
            brls::Logger::error("BackgroundAudioPlayer: timed out waiting for Switch audio buffers; retaining state to avoid DMA use-after-free");
            m_platformState = nullptr;
            if (m_registered.exchange(false, std::memory_order_acq_rel)) {
                BackgroundAudioPlayer* expected = this;
                s_activePlayer.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
                s_activePlayers.fetch_sub(1, std::memory_order_acq_rel);
            }
            return;
        }
        for (int i = 0; i < PlatformState::Count; ++i) std::free(state->data[i]);
#elif defined(BK_AUDIO_WINMM)
        if (state->wave) {
            waveOutReset(state->wave);
            for (int i = 0; i < PlatformState::Count; ++i)
                waveOutUnprepareHeader(state->wave, &state->headers[i], sizeof(WAVEHDR));
            waveOutClose(state->wave);
        }
        if (state->event) CloseHandle(state->event);
#elif defined(BK_AUDIO_ALSA)
        if (state->pcm) { snd_pcm_drop(state->pcm); snd_pcm_close(state->pcm); }
#elif defined(BK_AUDIO_COREAUDIO)
        if (state->unit) {
            AudioOutputUnitStop(state->unit);
            AudioUnitUninitialize(state->unit);
            AudioComponentInstanceDispose(state->unit);
        }
#endif
        delete state;
    }
    m_platformState = nullptr;
    if (m_registered.exchange(false, std::memory_order_acq_rel)) {
        BackgroundAudioPlayer* expected = this;
        s_activePlayer.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
        s_activePlayers.fetch_sub(1, std::memory_order_acq_rel);
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    resetLocked();
}

BackgroundAudioPlayer::~BackgroundAudioPlayer()
{
    stop();
}

} // namespace beiklive
