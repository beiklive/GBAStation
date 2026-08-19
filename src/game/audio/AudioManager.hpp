#pragma once

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <array>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <borealis.hpp>
#include "core/Singleton.hpp"

namespace beiklive {

/// 通用跨平台游戏音频管理器单例
///
/// 维护一个线程安全的 int16_t 立体声 PCM 环形缓冲区，
/// 后台线程持续从缓冲区读取数据并输出到平台音频接口。
///
/// 平台后端（通过宏定义选择）：
///   - Nintendo Switch  : libnx audout（直接在 __SWITCH__ 下启用）
///   - Linux            : ALSA (libasound，需 BK_AUDIO_ALSA 宏）
///   - Windows          : WinMM (waveOut，需 BK_AUDIO_WINMM 宏）
///   - macOS            : CoreAudio（需 BK_AUDIO_COREAUDIO 宏）
///   - 其他/回退        : 空输出（丢弃样本）
///
/// 典型用法：
/// @code
///   AudioManager::instance().init(32768, 2);
///   // libretro 音频回调中：
///   AudioManager::instance().pushSamples(data, frames);
///   // 关闭时：
///   AudioManager::instance().deinit();
/// @endcode
class AudioManager : public Singleton<AudioManager> {
    friend class Singleton<AudioManager>;

public:
    /// 初始化音频子系统。
    /// @param sampleRate  目标采样率（Hz，mGBA 默认：32768）。
    /// @param channels    声道数（mGBA 默认：2 立体声）。
    /// @return 成功打开音频输出时返回 true。
    bool init(int sampleRate = 32768, int channels = 2);

    /// 向缓冲区写入交错立体声 16 位 PCM 帧数据。
    /// 若环形缓冲区超过阈值则阻塞，以保持游戏与音频同步、防止延迟累积。
    /// 线程安全，可在 libretro 音频回调中调用。
    void pushSamples(const int16_t* data, size_t frames);

    /// 非阻塞写入：不阻塞调用方，缓冲区满时由 ringWrite 逐样本覆盖最旧数据。
    /// 用于快进等不希望因音频同步而拖慢模拟速度的场景。
    void pushSamplesNoBlocking(const int16_t* data, size_t frames);

    /// 设置当前模拟速度倍率（1.0=正常速度，音频线程据此动态调整重采样比例）
    void setSpeed(float speed) { m_currentSpeed.store(speed, std::memory_order_release); }

    /// 设置全局主音量（0.0=静音，1.0=原始音量）。
    /// 可在任意线程调用；音频线程输出前平滑过渡，避免爆音。
    void setMasterVolume(float volume);

    /// 查询当前全局主音量。
    float masterVolume() const { return m_masterVolume.load(std::memory_order_acquire); }

    /// 读取设置中的主音量（0-100）并应用，未初始化时返回 1.0。
    static float applyMasterVolumeFromSetting();

    /// 根据核心采样率设置延迟窗口。target 用于 GameView 音画同步，max 用于写入端限流/丢旧样本。
    void configureLatencyMs(int targetMs, int maxMs);

    /// 关闭音频子系统并停止后台线程。
    void deinit();

    bool isRunning() const { return m_running.load(std::memory_order_acquire); }

    /// 设置环形缓冲区最大填充量（立体声帧数），超过后 pushSamples() 开始阻塞。
    /// 在 init() 之后调用，默认值为 RING_CAPACITY / 2。
    void setMaxLatencyFrames(size_t frames) {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_maxLatencySamples = std::min(RING_CAPACITY, frames * static_cast<size_t>(m_channels));
    }

    /// 清空环形缓冲区中的所有样本，并唤醒被阻塞的 pushSamples() 调用方。
    /// 用于从快进切换回正常速度时，防止播放过时音频。
    void flushRingBuffer();
    void flushRingBufferWithFade(int fadeMs);
    void pauseOutput();
    void resumeOutputWithFade(int fadeMs);

    /// 查询环形缓冲区中当前可用样本数。
    size_t available() const;
    size_t targetLatencySamples() const;
    size_t maxLatencySamples() const;

    /// 查询当前音频采样率
    int sampleRate() const { return m_sampleRate; }

// private:
    // ---- 环形缓冲区 -------------------------------------------------
    // 将总延迟控制在约 250ms 以内
    static constexpr size_t RING_CAPACITY = 32768;

    std::atomic<float> m_currentSpeed{1.0f}; ///< 当前模拟速度倍率

    mutable std::mutex       m_mutex;
    std::condition_variable  m_spaceCV;   ///< 环形缓冲区排空（释放空间）时通知
    std::condition_variable  m_dataCV;    ///< 环形缓冲区有新数据时通知（唤醒音频线程）
    std::vector<int16_t>     m_ring;      ///< 循环 PCM 样本存储
    size_t                   m_writePos          = 0;
    size_t                   m_readPos           = 0;
    size_t                   m_available         = 0;
    size_t                   m_maxLatencySamples = RING_CAPACITY / 2;
    size_t                   m_targetLatencySamples = RING_CAPACITY / 4;
    size_t                   m_fadeInSamplesRemaining = 0;
    size_t                   m_fadeInTotalSamples = 0;
    std::array<int16_t, 2>   m_lastOutputSample{0, 0};
    bool                     m_hasLastOutputSample = false;
    double                   m_resamplePhase = 0.0;
    std::vector<int16_t>     m_resampleCarry;
    std::vector<int16_t>     m_resampleScratch;
    std::atomic<bool>        m_outputPaused{false};

    void   ringWrite(const int16_t* data, size_t count);
    size_t ringRead(int16_t* out, size_t maxCount);
    void   applyFadeIn(int16_t* out, size_t count);
    /// 对输出缓冲应用主音量增益（带平滑过渡，须在持有 m_mutex 时调用）。
    void   applyMasterVolume(int16_t* out, size_t count);
    void   fillUnderrunTailLocked(int16_t* out, size_t validSamples, size_t totalSamples);
    void   rememberOutputTailLocked(const int16_t* data, size_t count);
    void   resetOutputTailLocked();
    void   resetBufferLocked();
    void   configureLatencyMsLocked(int targetMs, int maxMs);

    // ---- 后台音频线程 -----------------------------------------------
    std::thread       m_thread;
    std::atomic<bool> m_running{false};
    int               m_sampleRate = 32768;
    int               m_channels   = 2;

    // ---- 主音量 -----------------------------------------------
    std::atomic<float> m_masterVolume{1.0f}; ///< 目标主音量（0.0~1.0）
    float              m_currentGain = 1.0f; ///< 当前实际增益（音频线程内逐步逼近目标，防爆音）

    void audioThreadFunc();

    // ---- 平台状态（不透明指针，保持头文件简洁）---------------------
    void* m_platformState = nullptr;
};

} // namespace beiklive
