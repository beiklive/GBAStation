#include "game/audio/AudioManager.hpp"

#include <cstring>
#include <cstdio>
#include <algorithm>
#include <chrono>
#include <thread>

// ============================================================
// 各平台头文件
// ============================================================

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

// ============================================================
// 环形缓冲区辅助函数
// ============================================================

void AudioManager::ringWrite(const int16_t* data, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        if (m_available < RING_CAPACITY) {
            m_ring[m_writePos] = data[i];
            m_writePos = (m_writePos + 1) % RING_CAPACITY;
            ++m_available;
        } else {
            // 缓冲区满：覆盖最旧样本
            m_ring[m_writePos] = data[i];
            m_writePos  = (m_writePos + 1) % RING_CAPACITY;
            m_readPos   = (m_readPos  + 1) % RING_CAPACITY;
        }
    }
}

size_t AudioManager::ringRead(int16_t* out, size_t maxCount)
{
    size_t n = std::min(maxCount, m_available);
    for (size_t i = 0; i < n; ++i) {
        out[i]    = m_ring[m_readPos];
        m_readPos = (m_readPos + 1) % RING_CAPACITY;
    }
    m_available -= n;
    if (n > 0)
        m_spaceCV.notify_all();
    return n;
}

// ============================================================
// pushSamples – 由 libretro 音频回调调用
// ============================================================

void AudioManager::pushSamples(const int16_t* data, size_t frames)
{
    if (!m_running.load(std::memory_order_acquire)) return;
    const size_t count = frames * static_cast<size_t>(m_channels);
    std::unique_lock<std::mutex> lk(m_mutex);
    // 环满时阻塞，保持游戏与音频同步，防止延迟累积
    m_spaceCV.wait(lk, [&] {
        return m_available + count <= m_maxLatencySamples ||
               !m_running.load(std::memory_order_relaxed);
    });
    if (!m_running.load(std::memory_order_relaxed)) return;
    ringWrite(data, count);
}

// ============================================================
// flushRingBuffer – 丢弃所有缓冲样本
// ============================================================

void AudioManager::flushRingBuffer()
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_available = 0;
    m_writePos  = m_readPos; // 读写指针归位，清空缓冲
    m_spaceCV.notify_all();  // 唤醒阻塞的 pushSamples() 调用方
}

// ============================================================
// ============================================================
// SWITCH – libnx audout 后端
// ============================================================
// ============================================================
#ifdef __SWITCH__

static constexpr int    SWITCH_OUT_RATE  = 48000;
static constexpr size_t SWITCH_FRAMES    = 512;
static constexpr size_t SWITCH_BYTES     = SWITCH_FRAMES * 2 * sizeof(int16_t);
static constexpr int    SWITCH_N_BUFFERS = 4;

struct SwitchAudioState {
    alignas(0x1000) int16_t bufData[SWITCH_N_BUFFERS][SWITCH_FRAMES * 2];
    AudioOutBuffer outBuf[SWITCH_N_BUFFERS];
    int            curBuf          = 0;
    u32            enqueuedBuffers = 0;
};

bool AudioManager::init(int sampleRate, int channels)
{
    if (m_running.load(std::memory_order_acquire)) return true;
    m_sampleRate = sampleRate;
    m_channels   = channels;

    auto* sw = new SwitchAudioState();
    m_platformState = sw;

    if (R_FAILED(audoutInitialize())) { delete sw; m_platformState = nullptr; return false; }
    if (R_FAILED(audoutStartAudioOut()))
    {
        // 若BKAudioPlayer已先启动audout，此处可能返回错误，继续使用共享流
        brls::Logger::warning("AudioManager: audoutStartAudioOut失败（可能已由BKAudioPlayer启动），继续使用共享audout流");
    }

    for (int i = 0; i < SWITCH_N_BUFFERS; ++i) {
        memset(sw->bufData[i], 0, SWITCH_FRAMES * 2 * sizeof(int16_t));
        sw->outBuf[i].next        = nullptr;
        sw->outBuf[i].buffer      = sw->bufData[i];
        sw->outBuf[i].buffer_size = SWITCH_BYTES;
        sw->outBuf[i].data_size   = SWITCH_BYTES;
        sw->outBuf[i].data_offset = 0;
        audoutAppendAudioOutBuffer(&sw->outBuf[i]);
    }
    sw->enqueuedBuffers = SWITCH_N_BUFFERS;
    sw->curBuf          = 0;

    m_ring.resize(RING_CAPACITY);
    m_running.store(true, std::memory_order_release);
    m_thread = std::thread(&AudioManager::audioThreadFunc, this);
    return true;
}

void AudioManager::audioThreadFunc()
{
    // 将音频输出线程绑定到核心 2（核心 0=UI，核心 1=模拟）
    svcSetThreadCoreMask(CUR_THREAD_HANDLE, 2, 1ULL << 2);

    auto* sw = static_cast<SwitchAudioState*>(m_platformState);
    while (m_running.load(std::memory_order_acquire)) {
        // 非阻塞回收：取回硬件已播完的缓冲
        {
            AudioOutBuffer* released = nullptr;
            u32 relCount = 0;
            audoutWaitPlayFinish(&released, &relCount, 0);
            if (relCount > 0 && sw->enqueuedBuffers >= relCount)
                sw->enqueuedBuffers -= relCount;
        }

        // 所有硬件槽占满时等待释放
        while (sw->enqueuedBuffers >= SWITCH_N_BUFFERS &&
               m_running.load(std::memory_order_acquire)) {
            AudioOutBuffer* released = nullptr;
            u32 relCount = 0;
            audoutWaitPlayFinish(&released, &relCount, 10000000); // 10ms 超时
            if (relCount > 0 && sw->enqueuedBuffers >= relCount)
                sw->enqueuedBuffers -= relCount;
        }

        if (!m_running.load(std::memory_order_acquire)) break;

        int16_t* dst = sw->bufData[sw->curBuf];

        // 计算重采样所需输入帧数（核心采样率 → SWITCH_OUT_RATE）
        double ratio = static_cast<double>(m_sampleRate) / SWITCH_OUT_RATE;
        size_t inputFrames = static_cast<size_t>(SWITCH_FRAMES * ratio + 0.5);
        if (inputFrames == 0) inputFrames = 1;
        if (inputFrames > SWITCH_FRAMES) inputFrames = SWITCH_FRAMES;

        int16_t tmp[SWITCH_FRAMES * 2];
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            size_t got = ringRead(tmp, inputFrames * 2);
            if (got < inputFrames * 2)
                memset(tmp + got, 0, (inputFrames * 2 - got) * sizeof(int16_t));
        }

        if (m_sampleRate != SWITCH_OUT_RATE) {
            // 最近邻重采样
            for (size_t i = 0; i < SWITCH_FRAMES; ++i) {
                size_t s = static_cast<size_t>(i * ratio);
                if (s >= inputFrames) s = inputFrames - 1;
                dst[i * 2]     = tmp[s * 2];
                dst[i * 2 + 1] = tmp[s * 2 + 1];
            }
        } else {
            memcpy(dst, tmp, SWITCH_FRAMES * 2 * sizeof(int16_t));
        }

        audoutAppendAudioOutBuffer(&sw->outBuf[sw->curBuf]);
        sw->curBuf = (sw->curBuf + 1) % SWITCH_N_BUFFERS;
        ++sw->enqueuedBuffers;
    }
}

void AudioManager::deinit()
{
    if (!m_running.load(std::memory_order_acquire)) return;
    m_running.store(false, std::memory_order_release);
    m_spaceCV.notify_all();
    if (m_thread.joinable()) m_thread.join();
    auto* sw = static_cast<SwitchAudioState*>(m_platformState);
    audoutStopAudioOut();
    audoutExit();
    delete sw;
    m_platformState = nullptr;
    m_ring.clear();
}

// ============================================================
// ============================================================
// LINUX – ALSA 后端
// ============================================================
// ============================================================
#elif defined(BK_AUDIO_ALSA)

static constexpr size_t ALSA_PERIOD_FRAMES = 256;

struct AlsaState {
    snd_pcm_t* handle = nullptr;
};

bool AudioManager::init(int sampleRate, int channels)
{
    if (m_running.load(std::memory_order_acquire)) return true;
    m_sampleRate = sampleRate;
    m_channels   = channels;

    auto* st = new AlsaState();
    m_platformState = st;

    if (snd_pcm_open(&st->handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        fprintf(stderr, "[AudioManager] ALSA: snd_pcm_open 失败\n");
        delete st;
        m_platformState = nullptr;
        return false;
    }

    snd_pcm_hw_params_t* params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(st->handle, params);
    snd_pcm_hw_params_set_access(st->handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(st->handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(st->handle, params, static_cast<unsigned>(channels));
    unsigned int rate = static_cast<unsigned int>(sampleRate);
    snd_pcm_hw_params_set_rate_near(st->handle, params, &rate, nullptr);
    snd_pcm_uframes_t period = ALSA_PERIOD_FRAMES;
    snd_pcm_hw_params_set_period_size_near(st->handle, params, &period, nullptr);

    if (snd_pcm_hw_params(st->handle, params) < 0) {
        fprintf(stderr, "[AudioManager] ALSA: snd_pcm_hw_params 失败\n");
        snd_pcm_close(st->handle);
        delete st;
        m_platformState = nullptr;
        return false;
    }

    m_ring.resize(RING_CAPACITY);
    m_running.store(true, std::memory_order_release);
    m_thread = std::thread(&AudioManager::audioThreadFunc, this);
    return true;
}

void AudioManager::audioThreadFunc()
{
    auto* st = static_cast<AlsaState*>(m_platformState);
    static int16_t buf[ALSA_PERIOD_FRAMES * 2];

    while (m_running.load(std::memory_order_acquire)) {
        size_t got = 0;
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            got = ringRead(buf, ALSA_PERIOD_FRAMES * 2);
        }
        if (got < ALSA_PERIOD_FRAMES * 2)
            memset(buf + got, 0, (ALSA_PERIOD_FRAMES * 2 - got) * sizeof(int16_t));

        snd_pcm_sframes_t rc = snd_pcm_writei(st->handle, buf, ALSA_PERIOD_FRAMES);
        if (rc == -EPIPE) {
            snd_pcm_prepare(st->handle);
        } else if (rc < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

void AudioManager::deinit()
{
    if (!m_running.load(std::memory_order_acquire)) return;
    m_running.store(false, std::memory_order_release);
    m_spaceCV.notify_all();
    if (m_thread.joinable()) m_thread.join();
    auto* st = static_cast<AlsaState*>(m_platformState);
    if (st->handle) {
        snd_pcm_drain(st->handle);
        snd_pcm_close(st->handle);
    }
    delete st;
    m_platformState = nullptr;
    m_ring.clear();
}

// ============================================================
// ============================================================
// WINDOWS – WinMM (waveOut) 后端
// ============================================================
// ============================================================
#elif defined(BK_AUDIO_WINMM)

static constexpr int    WINMM_NUM_BUFS   = 3;
static constexpr size_t WINMM_BUF_FRAMES = 512;
static constexpr size_t WINMM_BUF_BYTES  = WINMM_BUF_FRAMES * 2 * sizeof(int16_t);

struct WinMMState {
    HWAVEOUT  hwo     = nullptr;
    WAVEHDR   hdrs[WINMM_NUM_BUFS];
    int16_t   data[WINMM_NUM_BUFS][WINMM_BUF_FRAMES * 2];
    int       nextBuf = 0;
    HANDLE    event   = nullptr;
};

static void CALLBACK s_waveOutCallback(HWAVEOUT, UINT msg, DWORD_PTR inst,
                                        DWORD_PTR, DWORD_PTR)
{
    if (msg == WOM_DONE) {
        auto* st = reinterpret_cast<WinMMState*>(inst);
        if (st && st->event) SetEvent(st->event);
    }
}

bool AudioManager::init(int sampleRate, int channels)
{
    if (m_running.load(std::memory_order_acquire)) return true;
    m_sampleRate = sampleRate;
    m_channels   = channels;

    auto* st = new WinMMState();
    m_platformState = st;

    st->event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    WAVEFORMATEX wfx{};
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = static_cast<WORD>(channels);
    wfx.nSamplesPerSec  = static_cast<DWORD>(sampleRate);
    wfx.wBitsPerSample  = 16;
    wfx.nBlockAlign     = wfx.nChannels * (wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize          = 0;

    if (waveOutOpen(&st->hwo, WAVE_MAPPER, &wfx,
                    reinterpret_cast<DWORD_PTR>(s_waveOutCallback),
                    reinterpret_cast<DWORD_PTR>(st),
                    CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
        fprintf(stderr, "[AudioManager] waveOutOpen 失败\n");
        CloseHandle(st->event);
        delete st;
        m_platformState = nullptr;
        return false;
    }

    for (int i = 0; i < WINMM_NUM_BUFS; ++i) {
        memset(&st->hdrs[i], 0, sizeof(WAVEHDR));
        st->hdrs[i].lpData         = reinterpret_cast<LPSTR>(st->data[i]);
        st->hdrs[i].dwBufferLength = static_cast<DWORD>(WINMM_BUF_BYTES);
        waveOutPrepareHeader(st->hwo, &st->hdrs[i], sizeof(WAVEHDR));
        st->hdrs[i].dwFlags |= WHDR_DONE; // 初始标记为可用
    }

    m_ring.resize(RING_CAPACITY);
    m_running.store(true, std::memory_order_release);
    m_thread = std::thread(&AudioManager::audioThreadFunc, this);
    return true;
}

void AudioManager::audioThreadFunc()
{
    auto* st = static_cast<WinMMState*>(m_platformState);

    while (m_running.load(std::memory_order_acquire)) {
        WAVEHDR& hdr = st->hdrs[st->nextBuf];

        // 等待当前缓冲可用
        while (!(hdr.dwFlags & WHDR_DONE) &&
               m_running.load(std::memory_order_acquire)) {
            WaitForSingleObject(st->event, 10);
        }

        if (!m_running.load(std::memory_order_acquire)) break;

        int16_t* dst = reinterpret_cast<int16_t*>(hdr.lpData);
        size_t got = 0;
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            got = ringRead(dst, WINMM_BUF_FRAMES * 2);
        }
        if (got < WINMM_BUF_FRAMES * 2)
            memset(dst + got, 0, (WINMM_BUF_FRAMES * 2 - got) * sizeof(int16_t));

        hdr.dwFlags &= ~WHDR_DONE;
        hdr.dwBufferLength = static_cast<DWORD>(WINMM_BUF_BYTES);
        waveOutWrite(st->hwo, &hdr, sizeof(WAVEHDR));

        st->nextBuf = (st->nextBuf + 1) % WINMM_NUM_BUFS;
    }
}

void AudioManager::deinit()
{
    if (!m_running.load(std::memory_order_acquire)) return;
    auto* st = static_cast<WinMMState*>(m_platformState);
    m_running.store(false, std::memory_order_release);
    m_spaceCV.notify_all();
    if (st && st->event) SetEvent(st->event);
    if (m_thread.joinable()) m_thread.join();

    if (st) {
        if (st->hwo) {
            waveOutReset(st->hwo);
            for (int i = 0; i < WINMM_NUM_BUFS; ++i)
                waveOutUnprepareHeader(st->hwo, &st->hdrs[i], sizeof(WAVEHDR));
            waveOutClose(st->hwo);
        }
        if (st->event) CloseHandle(st->event);
        delete st;
    }
    m_platformState = nullptr;
    m_ring.clear();
}

// ============================================================
// ============================================================
// macOS – CoreAudio (AudioUnit) 后端
// ============================================================
// ============================================================
#elif defined(BK_AUDIO_COREAUDIO)

struct CoreAudioState {
    AudioUnit     audioUnit = nullptr;
    AudioManager* mgr       = nullptr;
};

static OSStatus s_coreAudioCallback(void*                       inRefCon,
                                     AudioUnitRenderActionFlags* /*ioFlags*/,
                                     const AudioTimeStamp*       /*inTimeStamp*/,
                                     UInt32                      /*inBusNumber*/,
                                     UInt32                      inNumFrames,
                                     AudioBufferList*            ioData)
{
    auto* mgr = static_cast<AudioManager*>(inRefCon);
    auto* dst = static_cast<int16_t*>(ioData->mBuffers[0].mData);
    size_t samples = inNumFrames * 2; // 立体声

    std::lock_guard<std::mutex> lk(mgr->m_mutex);
    size_t got = mgr->ringRead(dst, samples);
    if (got < samples)
        memset(dst + got, 0, (samples - got) * sizeof(int16_t));

    return noErr;
}

bool AudioManager::init(int sampleRate, int channels)
{
    if (m_running.load(std::memory_order_acquire)) return true;
    m_sampleRate = sampleRate;
    m_channels   = channels;

    auto* st = new CoreAudioState();
    st->mgr  = this;
    m_platformState = st;

    AudioComponentDescription desc{};
    desc.componentType         = kAudioUnitType_Output;
    desc.componentSubType      = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
    if (!comp || AudioComponentInstanceNew(comp, &st->audioUnit) != noErr) {
        fprintf(stderr, "[AudioManager] CoreAudio: 创建 AudioUnit 失败\n");
        delete st;
        m_platformState = nullptr;
        return false;
    }

    AURenderCallbackStruct cb{ s_coreAudioCallback, this };
    AudioUnitSetProperty(st->audioUnit, kAudioUnitProperty_SetRenderCallback,
                         kAudioUnitScope_Input, 0, &cb, sizeof(cb));

    AudioStreamBasicDescription fmt{};
    fmt.mSampleRate       = sampleRate;
    fmt.mFormatID         = kAudioFormatLinearPCM;
    fmt.mFormatFlags      = kLinearPCMFormatFlagIsSignedInteger |
                            kLinearPCMFormatFlagIsPacked;
    fmt.mFramesPerPacket  = 1;
    fmt.mChannelsPerFrame = static_cast<UInt32>(channels);
    fmt.mBitsPerChannel   = 16;
    fmt.mBytesPerFrame    = static_cast<UInt32>(channels) * 2;
    fmt.mBytesPerPacket   = fmt.mBytesPerFrame;
    AudioUnitSetProperty(st->audioUnit, kAudioUnitProperty_StreamFormat,
                         kAudioUnitScope_Input, 0, &fmt, sizeof(fmt));

    if (AudioUnitInitialize(st->audioUnit) != noErr ||
        AudioOutputUnitStart(st->audioUnit) != noErr) {
        fprintf(stderr, "[AudioManager] CoreAudio: AudioUnit 启动失败\n");
        AudioComponentInstanceDispose(st->audioUnit);
        delete st;
        m_platformState = nullptr;
        return false;
    }

    m_ring.resize(RING_CAPACITY);
    m_running.store(true, std::memory_order_release);
    // CoreAudio 由回调驱动，无需后台线程
    return true;
}

void AudioManager::audioThreadFunc()
{
    // CoreAudio 后端不使用此函数（回调驱动）
    while (m_running.load(std::memory_order_acquire))
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void AudioManager::deinit()
{
    if (!m_running.load(std::memory_order_acquire)) return;
    m_running.store(false, std::memory_order_release);
    m_spaceCV.notify_all();
    auto* st = static_cast<CoreAudioState*>(m_platformState);
    if (st->audioUnit) {
        AudioOutputUnitStop(st->audioUnit);
        AudioUnitUninitialize(st->audioUnit);
        AudioComponentInstanceDispose(st->audioUnit);
    }
    delete st;
    m_platformState = nullptr;
    m_ring.clear();
}

// ============================================================
// ============================================================
// 兜底实现 – 无音频输出（丢弃样本）
// ============================================================
// ============================================================
#else

bool AudioManager::init(int sampleRate, int channels)
{
    m_sampleRate = sampleRate;
    m_channels   = channels;
    m_ring.resize(RING_CAPACITY);
    m_running.store(true, std::memory_order_release);
    m_thread = std::thread(&AudioManager::audioThreadFunc, this);
    return true;
}

void AudioManager::audioThreadFunc()
{
    static int16_t sink[512 * 2];
    while (m_running.load(std::memory_order_acquire)) {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            ringRead(sink, 512 * 2);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void AudioManager::deinit()
{
    if (!m_running.load(std::memory_order_acquire)) return;
    m_running.store(false, std::memory_order_release);
    m_spaceCV.notify_all();
    if (m_thread.joinable()) m_thread.join();
    m_ring.clear();
}

#endif // 各平台后端

} // namespace beiklive
