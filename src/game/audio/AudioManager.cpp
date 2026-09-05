#include "game/audio/AudioManager.hpp"

#include "core/common.h"
#include "ui/utils/BackgroundAudioPlayer.hpp"
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
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
    if (count == 0 || m_ring.empty())
        return;

    if (count >= RING_CAPACITY) {
        data += count - RING_CAPACITY;
        count = RING_CAPACITY;
        m_readPos = 0;
        m_writePos = 0;
        m_available = 0;
    }

    if (m_available + count > RING_CAPACITY) {
        size_t excess = (m_available + count) - RING_CAPACITY + (RING_CAPACITY / 8);
        if (excess > m_available)
            excess = m_available;
        m_readPos = (m_readPos + excess) % RING_CAPACITY;
        m_available -= excess;
    }
    for (size_t i = 0; i < count; ++i) {
        m_ring[m_writePos] = data[i];
        m_writePos = (m_writePos + 1) % RING_CAPACITY;
        ++m_available;
    }
    m_dataCV.notify_one();
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

void AudioManager::applyFadeIn(int16_t* out, size_t count)
{
    if (!out || count == 0 || m_fadeInSamplesRemaining == 0 || m_fadeInTotalSamples == 0)
        return;

    const size_t n = std::min(count, m_fadeInSamplesRemaining);
    const size_t total = m_fadeInTotalSamples;
    const size_t channels = static_cast<size_t>(std::max(1, m_channels));
    for (size_t i = 0; i < n; ++i) {
        const size_t done = total - m_fadeInSamplesRemaining + i;
        float gain = static_cast<float>(done / channels + 1) /
                     static_cast<float>((total + channels - 1) / channels);
        if (gain > 1.0f) gain = 1.0f;
        out[i] = static_cast<int16_t>(static_cast<float>(out[i]) * gain);
    }
    m_fadeInSamplesRemaining -= n;
    if (m_fadeInSamplesRemaining == 0)
        m_fadeInTotalSamples = 0;
}

void AudioManager::setMasterVolume(float volume)
{
    m_masterVolume.store(std::clamp(volume, 0.0f, 1.0f), std::memory_order_release);
}

float AudioManager::applyMasterVolumeFromSetting()
{
    const int percent = GET_SETTING_KEY_INT(
        beiklive::SettingKey::KEY_AUDIO_MASTER_VOLUME, 100);
    const float volume = static_cast<float>(std::clamp(percent, 0, 100)) / 100.0f;
    AudioManager::instance().setMasterVolume(volume);
    return volume;
}

void AudioManager::applyMasterVolume(int16_t* out, size_t count)
{
    if (!out || count == 0)
        return;
    const float target = m_masterVolume.load(std::memory_order_acquire);
    if (target >= 0.999f && m_currentGain >= 0.999f)
        return;
    // 每样本至多向目标靠近 1/256，避免音量突变产生咔哒声；
    // 约 48000Hz 下 256 样本 ≈ 5.3ms 完成过渡。
    const float step = 1.0f / 256.0f;
    for (size_t i = 0; i < count; ++i) {
        float& gain = m_currentGain;
        if (gain < target)
            gain = std::min(target, gain + step);
        else if (gain > target)
            gain = std::max(target, gain - step);
        if (gain >= 0.999f) {
            // 已接近原始音量：余下样本原样拷贝（快路径）
            m_currentGain = 1.0f;
            return;
        }
        if (gain <= 0.0005f) {
            out[i] = 0;
            continue;
        }
        const int scaled = static_cast<int>(std::lround(
            static_cast<float>(out[i]) * gain));
        out[i] = static_cast<int16_t>(std::clamp(scaled, -32768, 32767));
    }
}

void AudioManager::resetOutputTailLocked()
{
    m_lastOutputSample = {0, 0};
    m_hasLastOutputSample = false;
}

void AudioManager::rememberOutputTailLocked(const int16_t* data, size_t count)
{
    if (!data || count == 0)
        return;

    const size_t channels = static_cast<size_t>(std::max(1, m_channels));
    const size_t frameStart = (count >= channels) ? (count - channels) : 0;
    for (size_t ch = 0; ch < m_lastOutputSample.size(); ++ch) {
        const size_t src = frameStart + std::min(ch, channels - 1);
        m_lastOutputSample[ch] = data[std::min(src, count - 1)];
    }
    m_hasLastOutputSample = true;
}

void AudioManager::fillUnderrunTailLocked(int16_t* out, size_t validSamples, size_t totalSamples)
{
    if (!out || validSamples >= totalSamples)
        return;

    const size_t channels = static_cast<size_t>(std::max(1, m_channels));
    std::array<int16_t, 2> tail = m_lastOutputSample;
    if (validSamples >= channels) {
        const size_t frameStart = validSamples - channels;
        for (size_t ch = 0; ch < tail.size(); ++ch) {
            const size_t src = frameStart + std::min(ch, channels - 1);
            tail[ch] = out[std::min(src, validSamples - 1)];
        }
    } else if (!m_hasLastOutputSample) {
        tail = {0, 0};
    }

    constexpr size_t kFadeFrames = 96; // 2ms at 48kHz, enough to hide hard cuts.
    const size_t missingSamples = totalSamples - validSamples;
    const size_t missingFrames = std::max<size_t>(1, (missingSamples + channels - 1) / channels);
    const size_t fadeFrames = std::min(kFadeFrames, missingFrames);
    for (size_t i = validSamples; i < totalSamples; ++i) {
        const size_t rel = i - validSamples;
        const size_t frame = rel / channels;
        const size_t ch = std::min(rel % channels, tail.size() - 1);
        float gain = 0.0f;
        if (frame < fadeFrames) {
            gain = 1.0f - (static_cast<float>(frame + 1) / static_cast<float>(fadeFrames + 1));
        }
        out[i] = static_cast<int16_t>(static_cast<float>(tail[ch]) * gain);
    }
}

void AudioManager::resetBufferLocked()
{
    m_writePos = 0;
    m_readPos = 0;
    m_available = 0;
    m_ring.assign(RING_CAPACITY, 0);
    m_fadeInSamplesRemaining = 0;
    m_fadeInTotalSamples = 0;
    resetOutputTailLocked();
    m_resamplePhase = 0.0;
    m_resampleCarry.clear();
    m_resampleScratch.clear();
    m_outputPaused.store(false, std::memory_order_release);
    // 同步当前增益，避免每次 init 时从上次音量渐变
    m_currentGain = m_masterVolume.load(std::memory_order_acquire);
}

void AudioManager::configureLatencyMsLocked(int targetMs, int maxMs)
{
    if (targetMs < 30) targetMs = 30;
    if (targetMs > 300) targetMs = 300;
    if (maxMs < targetMs + 20) maxMs = targetMs + 20;
    if (maxMs > 500) maxMs = 500;

    const size_t channels = static_cast<size_t>(std::max(1, m_channels));
    auto msToSamples = [&](int ms) {
        const double frames = static_cast<double>(std::max(1, m_sampleRate)) *
                              static_cast<double>(ms) / 1000.0;
        return static_cast<size_t>(frames + 0.5) * channels;
    };

    m_targetLatencySamples = std::min(RING_CAPACITY / 2, msToSamples(targetMs));
    m_maxLatencySamples = std::min(RING_CAPACITY - channels, msToSamples(maxMs));
    if (m_maxLatencySamples <= m_targetLatencySamples)
        m_maxLatencySamples = std::min(RING_CAPACITY - channels,
                                       m_targetLatencySamples + RING_CAPACITY / 8);
}

void AudioManager::configureLatencyMs(int targetMs, int maxMs)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    configureLatencyMsLocked(targetMs, maxMs);
}

size_t AudioManager::available() const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_available;
}

size_t AudioManager::targetLatencySamples() const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_targetLatencySamples;
}

size_t AudioManager::maxLatencySamples() const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_maxLatencySamples;
}

// ============================================================
// pushSamples – 由 libretro 音频回调调用
// ============================================================

void AudioManager::pushSamples(const int16_t* data, size_t frames)
{
    if (!data || frames == 0) return;
    if (!m_running.load(std::memory_order_acquire)) return;
    if (m_outputPaused.load(std::memory_order_acquire)) return;
    size_t count = frames * static_cast<size_t>(m_channels);
    const int16_t* src = data;
    std::unique_lock<std::mutex> lk(m_mutex);
    if (count > m_maxLatencySamples) {
        const size_t keep = std::max(static_cast<size_t>(m_channels), m_maxLatencySamples);
        src += count - keep;
        count = keep;
    }
    // 正常播放允许短暂等待音频线程释放空间，但不能无限阻塞模拟线程。
    // 超时后丢弃最旧样本，让音频追上当前画面，避免启动阶段被 audout 反向拖慢。
    m_spaceCV.wait_for(lk, std::chrono::milliseconds(2), [&] {
        return m_available + count <= m_maxLatencySamples ||
               !m_running.load(std::memory_order_relaxed);
    });
    if (!m_running.load(std::memory_order_relaxed)) return;

    if (m_available + count > m_maxLatencySamples) {
        const size_t targetAvailable =
            (m_maxLatencySamples > count) ? (m_maxLatencySamples - count) : 0;
        if (m_available > targetAvailable) {
            const size_t drop = m_available - targetAvailable;
            m_readPos = (m_readPos + drop) % RING_CAPACITY;
            m_available -= drop;
        }
    }

    ringWrite(src, count);
}

void AudioManager::pushSamplesNoBlocking(const int16_t* data, size_t frames)
{
    if (!data || frames == 0) return;
    if (!m_running.load(std::memory_order_acquire)) return;
    if (m_outputPaused.load(std::memory_order_acquire)) return;
    size_t count = frames * static_cast<size_t>(m_channels);
    const int16_t* src = data;
    std::lock_guard<std::mutex> lk(m_mutex);
    if (count > m_maxLatencySamples) {
        const size_t keep = std::max(static_cast<size_t>(m_channels), m_maxLatencySamples);
        src += count - keep;
        count = keep;
    }
    if (m_available + count > m_maxLatencySamples) {
        const size_t targetAvailable =
            (m_maxLatencySamples > count) ? (m_maxLatencySamples - count) : 0;
        if (m_available > targetAvailable) {
            const size_t drop = m_available - targetAvailable;
            m_readPos = (m_readPos + drop) % RING_CAPACITY;
            m_available -= drop;
        }
    }
    ringWrite(src, count);
}

// ============================================================
// flushRingBuffer – 丢弃所有缓冲样本
// ============================================================

void AudioManager::flushRingBuffer()
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_available = 0;
    m_writePos  = m_readPos; // 读写指针归位，清空缓冲
    m_resamplePhase = 0.0;
    m_resampleCarry.clear();
    m_fadeInSamplesRemaining = 0;
    m_fadeInTotalSamples = 0;
    resetOutputTailLocked();
    m_spaceCV.notify_all();  // 唤醒阻塞的 pushSamples() 调用方
    m_dataCV.notify_all();   // 唤醒阻塞的音频线程
}

void AudioManager::flushRingBufferWithFade(int fadeMs)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_available = 0;
    m_writePos  = m_readPos;
    m_resamplePhase = 0.0;
    m_resampleCarry.clear();
    resetOutputTailLocked();
    if (fadeMs > 0) {
        if (fadeMs > 20) fadeMs = 20;
        const size_t channels = static_cast<size_t>(std::max(1, m_channels));
        const double frames = static_cast<double>(std::max(1, m_sampleRate)) *
                              static_cast<double>(fadeMs) / 1000.0;
        m_fadeInTotalSamples = static_cast<size_t>(frames + 0.5) * channels;
        m_fadeInSamplesRemaining = m_fadeInTotalSamples;
    } else {
        m_fadeInSamplesRemaining = 0;
        m_fadeInTotalSamples = 0;
    }
    m_spaceCV.notify_all();
    m_dataCV.notify_all();
}

void AudioManager::pauseOutput()
{
    m_outputPaused.store(true, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_available = 0;
        m_writePos  = m_readPos;
        m_resamplePhase = 0.0;
        m_resampleCarry.clear();
        m_fadeInSamplesRemaining = 0;
        m_fadeInTotalSamples = 0;
        resetOutputTailLocked();
    }
    m_spaceCV.notify_all();
    m_dataCV.notify_all();
}

void AudioManager::resumeOutputWithFade(int fadeMs)
{
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_available = 0;
        m_writePos  = m_readPos;
        m_resamplePhase = 0.0;
        m_resampleCarry.clear();
        resetOutputTailLocked();
        if (fadeMs > 0) {
            if (fadeMs > 20) fadeMs = 20;
            const size_t channels = static_cast<size_t>(std::max(1, m_channels));
            const double frames = static_cast<double>(std::max(1, m_sampleRate)) *
                                  static_cast<double>(fadeMs) / 1000.0;
            m_fadeInTotalSamples = static_cast<size_t>(frames + 0.5) * channels;
            m_fadeInSamplesRemaining = m_fadeInTotalSamples;
        } else {
            m_fadeInSamplesRemaining = 0;
            m_fadeInTotalSamples = 0;
        }
    }
    m_outputPaused.store(false, std::memory_order_release);
    m_spaceCV.notify_all();
    m_dataCV.notify_all();
}

// ============================================================
// ============================================================
// SWITCH – libnx audout 后端
// ============================================================
// ============================================================
#ifdef __SWITCH__

static constexpr int    SWITCH_OUT_RATE  = 48000;
static constexpr size_t SWITCH_FRAMES    = 1024;
static constexpr size_t SWITCH_BYTES     = SWITCH_FRAMES * 2 * sizeof(int16_t);
static constexpr int    SWITCH_N_BUFFERS = 4;

struct SwitchAudioState {
    int16_t*       bufData[SWITCH_N_BUFFERS] = {};
    AudioOutBuffer outBuf[SWITCH_N_BUFFERS];
    bool           queued[SWITCH_N_BUFFERS] = {};
    int            freeList[SWITCH_N_BUFFERS] = {};
    int            freeCount       = 0;
    u32            enqueuedBuffers = 0;
    bool           loggedFirstAppend = false;
    int            appendFailLogs = 0;

    bool owns(const AudioOutBuffer* buf, int* index = nullptr) const
    {
        for (int i = 0; i < SWITCH_N_BUFFERS; ++i) {
            if (buf == &outBuf[i]) {
                if (index) *index = i;
                return true;
            }
        }
        return false;
    }

    void markQueued(int index)
    {
        if (!queued[index]) {
            queued[index] = true;
            ++enqueuedBuffers;
        }
    }

    void markReleased(int index)
    {
        if (!queued[index])
            return;

        queued[index] = false;
        if (enqueuedBuffers > 0)
            --enqueuedBuffers;
        if (freeCount < SWITCH_N_BUFFERS)
            freeList[freeCount++] = index;
    }

    void refreshReleasedFromHardware()
    {
        for (int i = 0; i < SWITCH_N_BUFFERS; ++i) {
            if (!queued[i])
                continue;
            bool contains = true;
            if (R_SUCCEEDED(audoutContainsAudioOutBuffer(&outBuf[i], &contains)) && !contains)
                markReleased(i);
        }
    }

    int takeFree()
    {
        if (freeCount <= 0)
            return -1;
        return freeList[--freeCount];
    }

    void freeBuffers()
    {
        for (int i = 0; i < SWITCH_N_BUFFERS; ++i) {
            std::free(bufData[i]);
            bufData[i] = nullptr;
            outBuf[i] = {};
        }
        freeCount = 0;
        enqueuedBuffers = 0;
        std::fill(std::begin(queued), std::end(queued), false);
    }
};

bool AudioManager::init(int sampleRate, int channels)
{
    if (m_running.load(std::memory_order_acquire)) return true;
    m_sampleRate = sampleRate;
    m_channels   = channels;

    auto* sw = new SwitchAudioState();
    m_platformState = sw;

    // BKAudioPlayer owns the process-wide audout service. AudioManager only
    // contributes game buffers to that stream and must not initialize/exit the
    // service independently; doing so can invalidate the UI player's session
    // while the application is still tearing down.

    for (int i = 0; i < SWITCH_N_BUFFERS; ++i) {
        sw->bufData[i] = static_cast<int16_t*>(std::aligned_alloc(0x1000, SWITCH_BYTES));
        if (!sw->bufData[i]) {
            brls::Logger::error("AudioManager: Switch audio buffer allocation failed ({} bytes)", SWITCH_BYTES);
            sw->freeBuffers();
            delete sw;
            m_platformState = nullptr;
            return false;
        }
        memset(sw->bufData[i], 0, SWITCH_BYTES);
        sw->outBuf[i].next        = nullptr;
        sw->outBuf[i].buffer      = sw->bufData[i];
        sw->outBuf[i].buffer_size = SWITCH_BYTES;
        sw->outBuf[i].data_size   = SWITCH_BYTES;
        sw->outBuf[i].data_offset = 0;
        sw->freeList[sw->freeCount++] = i;
    }

    // 每次初始化时重置环形缓冲区状态，防止上次会话的残留指针/计数导致第二次启动时读到
    // 零数据与真实音频混合的数据块，产生撕裂或刺耳声
    applyMasterVolumeFromSetting();
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        resetBufferLocked();
        configureLatencyMsLocked(90, 180);
    }
    m_running.store(true, std::memory_order_release);
    m_thread = std::thread(&AudioManager::audioThreadFunc, this);
    return true;
}

void AudioManager::audioThreadFunc()
{
    // 将音频输出线程绑定到核心 2（核心 0=UI，核心 1=模拟）
    Result affinityRc = svcSetThreadCoreMask(CUR_THREAD_HANDLE, 2, 1ULL << 2);
    if (R_FAILED(affinityRc))
        brls::Logger::warning("AudioManager: failed to pin Switch audio thread to core2 rc={:#x}", affinityRc);
    else
        brls::Logger::info("AudioManager: Switch audio thread pinned to core2");

    auto* sw = static_cast<SwitchAudioState*>(m_platformState);
    auto collectReleased = [sw](AudioOutBuffer* released) {
        for (AudioOutBuffer* buf = released; buf != nullptr; buf = buf->next) {
            int index = -1;
            if (sw->owns(buf, &index))
                sw->markReleased(index);
        }
    };

    // Let the emulation thread establish a small lead before the first
    // hardware submission. Starting with four zero-filled blocks makes the
    // transition into a game sound like a burst of tearing, and leaves too
    // little margin if the first few frames are scheduled late.
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        const size_t startupTarget = std::min(
            m_targetLatencySamples,
            static_cast<size_t>(SWITCH_OUT_RATE / 20) * static_cast<size_t>(std::max(1, m_channels)));
        m_dataCV.wait_for(lk, std::chrono::milliseconds(100), [this, startupTarget] {
            return m_available >= startupTarget ||
                   !m_running.load(std::memory_order_acquire);
        });
    }

    while (m_running.load(std::memory_order_acquire)) {
        // 非阻塞回收：取回硬件已播完的缓冲
        {
            AudioOutBuffer* released = nullptr;
            u32 relCount = 0;
            std::lock_guard<std::mutex> outputLock(BackgroundAudioPlayer::switchOutputMutex());
            audoutWaitPlayFinish(&released, &relCount, 0);
            if (relCount > 0 && released)
                collectReleased(released);
            // audout 的完成通知是进程级的，可能已被背景音频或 UI 音效
            // 消费；直接查询硬件队列，恢复本实例丢失的释放状态。
            sw->refreshReleasedFromHardware();
        }

        while (m_outputPaused.load(std::memory_order_acquire) &&
               m_running.load(std::memory_order_acquire)) {
            if (sw->enqueuedBuffers > 0) {
                AudioOutBuffer* released = nullptr;
                u32 relCount = 0;
                std::lock_guard<std::mutex> outputLock(BackgroundAudioPlayer::switchOutputMutex());
                audoutWaitPlayFinish(&released, &relCount, 10000000);
                if (relCount > 0 && released)
                    collectReleased(released);
                sw->refreshReleasedFromHardware();
            } else {
                std::unique_lock<std::mutex> lk(m_mutex);
                m_dataCV.wait_for(lk, std::chrono::milliseconds(10), [this] {
                    return !m_outputPaused.load(std::memory_order_acquire) ||
                           !m_running.load(std::memory_order_acquire);
                });
            }
        }

        if (!m_running.load(std::memory_order_acquire)) break;

        // 所有硬件槽占满时等待释放
        while (sw->freeCount == 0 &&
               m_running.load(std::memory_order_acquire)) {
            AudioOutBuffer* released = nullptr;
            u32 relCount = 0;
            std::lock_guard<std::mutex> outputLock(BackgroundAudioPlayer::switchOutputMutex());
            audoutWaitPlayFinish(&released, &relCount, 10000000); // 10ms 超时
            if (relCount > 0 && released)
                collectReleased(released);
            sw->refreshReleasedFromHardware();
        }

        if (!m_running.load(std::memory_order_acquire)) break;

        int bufIndex = sw->takeFree();
        if (bufIndex < 0)
            continue;

        int16_t* dst = sw->bufData[bufIndex];
        if (!dst) {
            if (sw->freeCount < SWITCH_N_BUFFERS)
                sw->freeList[sw->freeCount++] = bufIndex;
            continue;
        }

        // 动态重采样比例 = (输入采样率 / 输出采样率) * 当前模拟速度。
        // 使用跨缓冲区延续的相位，避免每个 audout 块重新起算造成边界抖动。
        float speed = m_currentSpeed.load(std::memory_order_acquire);
        if (speed < 0.1f) speed = 1.0f;
        double step = static_cast<double>(m_sampleRate) / SWITCH_OUT_RATE * speed;
        if (step <= 0.0)
            step = static_cast<double>(m_sampleRate) / SWITCH_OUT_RATE;

        {
            constexpr size_t kOutChannels = 2;
            std::unique_lock<std::mutex> lk(m_mutex);
            const size_t carryFrames = m_resampleCarry.size() / kOutChannels;
            size_t inputFrames = static_cast<size_t>(
                std::ceil(m_resamplePhase + step * static_cast<double>(SWITCH_FRAMES))) + 2u;
            const size_t maxInputFrames = (RING_CAPACITY / kOutChannels) - 1u;
            if (inputFrames > maxInputFrames)
                inputFrames = maxInputFrames;
            if (inputFrames < carryFrames + 2u)
                inputFrames = carryFrames + 2u;

            m_resampleScratch.resize(inputFrames * kOutChannels);
            if (!m_resampleCarry.empty())
                std::copy(m_resampleCarry.begin(), m_resampleCarry.end(), m_resampleScratch.begin());

            const size_t framesToRead = inputFrames - carryFrames;
            const size_t needed = framesToRead * kOutChannels;
            // A 60 Hz core normally supplies about 800 frames at 48 kHz, while
            // one audout block needs 1026 source frames. Waking for any stereo
            // frame therefore padded every hardware block with a fade to zero,
            // which sounds like a periodic dull pop or hiss. Wait for a complete
            // block; only use the underrun tail when the core genuinely stalls.
            const auto sourceBlockMs = static_cast<int>(std::ceil(
                1000.0 * static_cast<double>(framesToRead) /
                static_cast<double>(std::max(1, m_sampleRate)))) + 8;
            const auto waitMs = std::clamp(sourceBlockMs, 16, 60);
            m_dataCV.wait_for(lk, std::chrono::milliseconds(waitMs), [&] {
                return m_available >= needed ||
                       !m_running.load(std::memory_order_relaxed);
            });
            size_t got = ringRead(m_resampleScratch.data() + carryFrames * kOutChannels, needed);
            if (got < needed) {
                const size_t validSamples = carryFrames * kOutChannels + got;
                fillUnderrunTailLocked(m_resampleScratch.data(), validSamples,
                                       inputFrames * kOutChannels);
            }

            for (size_t i = 0; i < SWITCH_FRAMES; ++i) {
                const double pos = m_resamplePhase + static_cast<double>(i) * step;
                size_t idx = static_cast<size_t>(pos);
                if (idx >= inputFrames)
                    idx = inputFrames - 1u;
                const double frac = pos - static_cast<double>(idx);
                const size_t nextIdx = (idx + 1u < inputFrames) ? idx + 1u : idx;

                for (size_t ch = 0; ch < kOutChannels; ++ch) {
                    const float a = static_cast<float>(m_resampleScratch[idx * kOutChannels + ch]);
                    const float b = static_cast<float>(m_resampleScratch[nextIdx * kOutChannels + ch]);
                    dst[i * kOutChannels + ch] =
                        static_cast<int16_t>(a + (b - a) * static_cast<float>(frac));
                }
            }

            const double nextPhase = m_resamplePhase + static_cast<double>(SWITCH_FRAMES) * step;
            size_t consumedFrames = static_cast<size_t>(nextPhase);
            if (consumedFrames >= inputFrames)
                consumedFrames = inputFrames - 1u;
            m_resamplePhase = nextPhase - static_cast<double>(consumedFrames);
            if (m_resamplePhase < 0.0 || m_resamplePhase >= 1.0)
                m_resamplePhase -= std::floor(m_resamplePhase);
            m_resampleCarry.assign(
                m_resampleScratch.begin() + static_cast<std::ptrdiff_t>(consumedFrames * kOutChannels),
                m_resampleScratch.end());
            applyFadeIn(dst, SWITCH_FRAMES * kOutChannels);
            applyMasterVolume(dst, SWITCH_FRAMES * kOutChannels);
            rememberOutputTailLocked(dst, SWITCH_FRAMES * kOutChannels);
        }

        armDCacheFlush(dst, SWITCH_BYTES);
        sw->outBuf[bufIndex].next = nullptr;
        Result appendRc;
        {
            std::lock_guard<std::mutex> outputLock(BackgroundAudioPlayer::switchOutputMutex());
            appendRc = audoutAppendAudioOutBuffer(&sw->outBuf[bufIndex]);
        }
        if (R_SUCCEEDED(appendRc)) {
            if (!sw->loggedFirstAppend) {
                int16_t peak = 0;
                for (size_t i = 0; i < SWITCH_FRAMES * 2; ++i) {
                    const int sample = dst[i] < 0 ? -static_cast<int>(dst[i]) : static_cast<int>(dst[i]);
                    if (sample > peak)
                        peak = static_cast<int16_t>(std::min(sample, static_cast<int>(INT16_MAX)));
                }
                brls::Logger::info("AudioManager: first Switch audio buffer appended peak={} queued={}",
                                   peak, sw->enqueuedBuffers);
                sw->loggedFirstAppend = true;
            }
            sw->markQueued(bufIndex);
        } else {
            if (sw->appendFailLogs < 5) {
                brls::Logger::warning("AudioManager: audoutAppendAudioOutBuffer failed rc={:#x}", appendRc);
                ++sw->appendFailLogs;
            }
            if (sw->freeCount < SWITCH_N_BUFFERS)
                sw->freeList[sw->freeCount++] = bufIndex;
        }
    }
}

void AudioManager::deinit()
{
    if (!m_running.load(std::memory_order_acquire)) return;
    m_running.store(false, std::memory_order_release);
    m_spaceCV.notify_all();
    m_dataCV.notify_all();  // 唤醒可能正在等待数据的音频线程
    if (m_thread.joinable()) m_thread.join();
    auto* sw = static_cast<SwitchAudioState*>(m_platformState);
    // 音频线程退出后，硬件队列中可能仍有 1-3 个未播完的 AudioManager 缓冲区。
    // 注意：此处【不调用 audoutStopAudioOut()】，原因如下：
    //   - audout 流由 BKAudioPlayer 负责启动（audoutStartAudioOut）并持续保持，
    //     AudioManager 只是向共享流提交缓冲区，不拥有流的生命周期。
    //   - 若在此处调用 audoutStopAudioOut()，第二次启动游戏时会重新启动共享流，
    //     导致两次游戏运行的初始化路径不一致。更严重的是：流被停止后再重启时，
    //     BKAudioPlayer 在间隙期提交的
    //     音效缓冲区会残留在硬件队列中。第二次游戏音频线程的 audoutWaitPlayFinish()
    //     会"拦截"该外来缓冲区的完成事件，使 enqueuedBuffers 计数错误（偏少 1），
    //     导致线程向仍在硬件 DMA 中的缓冲区写入数据，产生全程爆音和撕裂音。
    // 正确做法：等待本次 AudioManager 自身的缓冲区自然播完后释放资源，
    //           让流继续由 BKAudioPlayer 管理，两次游戏看到完全相同的 audout 状态。
    if (sw) {
        // 每次等待超时略超一个硬件缓冲区的播放时长
        // （SWITCH_FRAMES / SWITCH_OUT_RATE = 512 / 48000 ≈ 10.7ms），
        // 重试次数为 SWITCH_N_BUFFERS 的 5 倍，足以覆盖所有缓冲区并留有余量。
        // 通过遍历返回指针链表，仅统计属于本 AudioManager 实例的缓冲区，
        // 过滤掉 BKAudioPlayer 提交的外来缓冲区，避免计数错乱。
        constexpr u64 kDrainTimeoutNs = 16000000ULL;         // ~16ms per iteration
        constexpr int kMaxRetries     = SWITCH_N_BUFFERS * 5; // 安全系数 5，约 320ms 总超时
        u32 ourEnqueued = sw->enqueuedBuffers;
        for (int retry = 0; ourEnqueued > 0 && retry < kMaxRetries; ++retry) {
            AudioOutBuffer* released = nullptr;
            u32 relCount = 0;
            {
                std::lock_guard<std::mutex> outputLock(BackgroundAudioPlayer::switchOutputMutex());
                audoutWaitPlayFinish(&released, &relCount, kDrainTimeoutNs);
            }
            if (relCount == 0 || released == nullptr)
                continue; // 超时，继续等待
            // 遍历返回缓冲区链表，仅统计属于本 AudioManager 的缓冲区
            // 若为外来缓冲区（BKAudioPlayer），丢弃完成事件并继续等待自身缓冲区
            for (AudioOutBuffer* buf = released; buf != nullptr; buf = buf->next) {
                int index = -1;
                if (sw->owns(buf, &index)) {
                    sw->markReleased(index);
                    if (ourEnqueued > 0) --ourEnqueued;
                }
            }
        }
        // Completion notifications are global to audout and can be consumed
        // by another producer. Reconcile the per-buffer state directly before
        // freeing memory, otherwise a still-DMA-owned buffer can be released.
        for (int retry = 0; sw->enqueuedBuffers > 0 && retry < 200; ++retry) {
            bool pending = false;
            {
                std::lock_guard<std::mutex> outputLock(BackgroundAudioPlayer::switchOutputMutex());
                sw->refreshReleasedFromHardware();
                for (bool queued : sw->queued)
                    pending = pending || queued;
            }
            if (!pending || sw->enqueuedBuffers == 0)
                break;
            svcSleepThread(10000000ULL); // 10ms
        }
        if (sw->enqueuedBuffers > 0) {
            brls::Logger::error("AudioManager: timed out waiting for Switch audio buffers; retaining state to avoid DMA use-after-free");
            m_platformState = nullptr;
            m_ring.clear();
            return;
        }
        // 注意：不调用 audoutStopAudioOut()，保持流持续运行供 BKAudioPlayer 使用
    }
    sw->freeBuffers();
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

    // 重置环形缓冲区状态，防止上次会话残留导致第二次启动时音频撕裂
    applyMasterVolumeFromSetting();
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        resetBufferLocked();
        configureLatencyMsLocked(90, 180);
    }
    m_running.store(true, std::memory_order_release);
    m_thread = std::thread(&AudioManager::audioThreadFunc, this);
    return true;
}

void AudioManager::audioThreadFunc()
{
    auto* st = static_cast<AlsaState*>(m_platformState);
    static int16_t buf[ALSA_PERIOD_FRAMES * 2];

    while (m_running.load(std::memory_order_acquire)) {
        if (m_outputPaused.load(std::memory_order_acquire)) {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_dataCV.wait_for(lk, std::chrono::milliseconds(10), [this] {
                return !m_outputPaused.load(std::memory_order_acquire) ||
                       !m_running.load(std::memory_order_acquire);
            });
            continue;
        }

        size_t got = 0;
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            got = ringRead(buf, ALSA_PERIOD_FRAMES * 2);
            if (got < ALSA_PERIOD_FRAMES * 2)
                fillUnderrunTailLocked(buf, got, ALSA_PERIOD_FRAMES * 2);
            applyFadeIn(buf, ALSA_PERIOD_FRAMES * 2);
            applyMasterVolume(buf, ALSA_PERIOD_FRAMES * 2);
            rememberOutputTailLocked(buf, ALSA_PERIOD_FRAMES * 2);
        }

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
    m_dataCV.notify_all();  // 唤醒可能正在等待数据的音频线程
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

    // 重置环形缓冲区状态，防止上次会话残留导致第二次启动时音频撕裂
    applyMasterVolumeFromSetting();
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        resetBufferLocked();
        configureLatencyMsLocked(90, 180);
    }
    m_running.store(true, std::memory_order_release);
    m_thread = std::thread(&AudioManager::audioThreadFunc, this);
    return true;
}

void AudioManager::audioThreadFunc()
{
    auto* st = static_cast<WinMMState*>(m_platformState);

    while (m_running.load(std::memory_order_acquire)) {
        if (m_outputPaused.load(std::memory_order_acquire)) {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_dataCV.wait_for(lk, std::chrono::milliseconds(10), [this] {
                return !m_outputPaused.load(std::memory_order_acquire) ||
                       !m_running.load(std::memory_order_acquire);
            });
            continue;
        }

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
            if (got < WINMM_BUF_FRAMES * 2)
                fillUnderrunTailLocked(dst, got, WINMM_BUF_FRAMES * 2);
            applyFadeIn(dst, WINMM_BUF_FRAMES * 2);
            applyMasterVolume(dst, WINMM_BUF_FRAMES * 2);
            rememberOutputTailLocked(dst, WINMM_BUF_FRAMES * 2);
        }

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

    if (mgr->m_outputPaused.load(std::memory_order_acquire)) {
        memset(dst, 0, samples * sizeof(int16_t));
        return noErr;
    }

    std::lock_guard<std::mutex> lk(mgr->m_mutex);
    size_t got = mgr->ringRead(dst, samples);
    if (got < samples)
        mgr->fillUnderrunTailLocked(dst, got, samples);
    mgr->applyFadeIn(dst, samples);
    mgr->applyMasterVolume(dst, samples);
    mgr->rememberOutputTailLocked(dst, samples);

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

    // 重置环形缓冲区状态，防止上次会话残留导致第二次启动时音频撕裂
    applyMasterVolumeFromSetting();
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        resetBufferLocked();
        configureLatencyMsLocked(90, 180);
    }
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
    // 重置环形缓冲区状态，防止上次会话残留导致第二次启动时音频撕裂
    applyMasterVolumeFromSetting();
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        resetBufferLocked();
        configureLatencyMsLocked(90, 180);
    }
    m_running.store(true, std::memory_order_release);
    m_thread = std::thread(&AudioManager::audioThreadFunc, this);
    return true;
}

void AudioManager::audioThreadFunc()
{
    static int16_t sink[512 * 2];
    while (m_running.load(std::memory_order_acquire)) {
        if (m_outputPaused.load(std::memory_order_acquire)) {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_dataCV.wait_for(lk, std::chrono::milliseconds(10), [this] {
                return !m_outputPaused.load(std::memory_order_acquire) ||
                       !m_running.load(std::memory_order_acquire);
            });
            continue;
        }

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
    m_dataCV.notify_all();
    if (m_thread.joinable()) m_thread.join();
    m_ring.clear();
}

#endif // 各平台后端

} // namespace beiklive
