#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

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

    /// 关闭音频子系统并停止后台线程。
    void deinit();

    bool isRunning() const { return m_running.load(std::memory_order_acquire); }

    /// 设置环形缓冲区最大填充量（立体声帧数），超过后 pushSamples() 开始阻塞。
    /// 在 init() 之后调用，默认值为 RING_CAPACITY / 2。
    void setMaxLatencyFrames(size_t frames) {
        m_maxLatencySamples = frames * static_cast<size_t>(m_channels);
    }

    /// 清空环形缓冲区中的所有样本，并唤醒被阻塞的 pushSamples() 调用方。
    /// 用于从快进切换回正常速度时，防止播放过时音频。
    void flushRingBuffer();

private:
    // ---- 环形缓冲区 -------------------------------------------------
    // 将总延迟控制在约 250ms 以内
    static constexpr size_t RING_CAPACITY = 8192;

    std::mutex               m_mutex;
    std::condition_variable  m_spaceCV;   ///< 环形缓冲区排空（释放空间）时通知
    std::vector<int16_t>     m_ring;      ///< 循环 PCM 样本存储
    size_t                   m_writePos          = 0;
    size_t                   m_readPos           = 0;
    size_t                   m_available         = 0;
    size_t                   m_maxLatencySamples = RING_CAPACITY / 2;

    void   ringWrite(const int16_t* data, size_t count);
    size_t ringRead(int16_t* out, size_t maxCount);

    // ---- 后台音频线程 -----------------------------------------------
    std::thread       m_thread;
    std::atomic<bool> m_running{false};
    int               m_sampleRate = 32768;
    int               m_channels   = 2;

    void audioThreadFunc();

    // ---- 平台状态（不透明指针，保持头文件简洁）---------------------
    void* m_platformState = nullptr;
};

} // namespace beiklive
