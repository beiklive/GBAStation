#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <array>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace beiklive {

/// 独立的前端背景音频输出。它不使用 AudioManager，避免和模拟器音频共享
/// 模拟器专用的同步、倍速和生命周期状态。
class BackgroundAudioPlayer {
public:
    static constexpr size_t BLOCK_FRAMES = 1024;
    BackgroundAudioPlayer() = default;
    ~BackgroundAudioPlayer();

    bool start(int sampleRate = 48000, int channels = 2);
    void stop();
    void pushSamples(const int16_t* data, size_t frames);
    bool isRunning() const { return m_running.load(std::memory_order_acquire); }
    void setVolume(float volume);
    size_t readSamples(int16_t* out, size_t count);
    static bool mixActiveSamples(const int16_t* data, size_t frames);

    /// Switch 的 UI 音效播放器用于避免与背景音频同时提交 audout 缓冲区。
    static bool isAnyActive();
    /// audout 是进程级队列，所有前端音频提交和回收必须串行化。
    static std::mutex& switchOutputMutex();

private:
    static constexpr size_t RING_CAPACITY = 48000 * 2 * 4;
    static constexpr size_t PREBUFFER_SAMPLES = 24000; // 250 ms at 48 kHz stereo
    static constexpr size_t RAMP_FRAMES = 128;
    static constexpr size_t OVERLAY_CAPACITY = 48000 * 2;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::condition_variable m_spaceCv;
    std::vector<int16_t> m_ring;
    std::deque<int16_t> m_overlay;
    size_t m_readPos = 0;
    size_t m_writePos = 0;
    size_t m_available = 0;
    std::array<int16_t, 2> m_lastOutput{{0, 0}};
    bool m_needsFadeIn = false;
    int m_sampleRate = 48000;
    int m_channels = 2;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_registered{false};
    std::atomic<float> m_volume{1.0f};
    std::thread m_thread;
    void* m_platformState = nullptr;

    void audioThread();
    void resetLocked();
    void waitForInitialBuffer();
    void pushOverlaySamples(const int16_t* data, size_t frames);
    static std::atomic<int> s_activePlayers;
    static std::atomic<BackgroundAudioPlayer*> s_activePlayer;
};

} // namespace beiklive
