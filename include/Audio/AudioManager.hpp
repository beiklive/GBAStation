#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

namespace beiklive {

/// 通用跨平台音频管理器。
///
/// 维护一个线程安全的 int16_t 立体声PCM环形缓冲区，
/// 后台线程持续从缓冲区读取数据并输出到平台音频接口。
///
/// 平台后端：
///   - Nintendo Switch  : libnx audout
///   - Linux            : ALSA (libasound)
///   - Windows          : WinMM (waveOut)
///   - macOS            : CoreAudio (AudioUnit，回调驱动)
///   - 其他/回退        : 空输出（丢弃样本）
///
/// 典型用法：
/// @code
///   AudioManager::instance().init(32768, 2);
///   // libretro音频回调中：
///   AudioManager::instance().pushSamples(data, frames);
///   // 关闭时：
///   AudioManager::instance().deinit();
/// @endcode
class AudioManager {
public:
    static AudioManager& instance();

    /// 初始化音频子系统。
    /// @param sampleRate  目标采样率（Hz，mGBA默认：32768）。
    /// @param channels    声道数（mGBA默认：2立体声）。
    /// @return 成功打开音频输出时返回true。
    bool init(int sampleRate = 32768, int channels = 2);

    /// 向缓冲区写入交错立体声16位PCM帧数据。
    /// 若环形缓冲区超过阈值则阻塞，以保持游戏与音频同步、防止延迟累积。
    /// 线程安全，可在libretro音频回调中调用。
    void pushSamples(const int16_t* data, size_t frames);

    /// 关闭音频子系统并停止后台线程。
    void deinit();

    bool isRunning() const { return m_running.load(); }

    /// 设置环形缓冲区最大填充量（立体声帧数），超过后pushSamples()开始阻塞。
    /// 在init()之后调用，默认值为 RING_CAPACITY / 2。
    void setMaxLatencyFrames(size_t frames) { m_maxLatencySamples = frames * static_cast<size_t>(m_channels); }

    /// 清空环形缓冲区中的所有样本，并唤醒被阻塞的pushSamples()调用方。
    /// 用于从快进切换回正常速度时，防止播放过时音频。
    void flushRingBuffer();

private:
    AudioManager()  = default;
    ~AudioManager() = default;

    AudioManager(const AudioManager&)            = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // ---- 环形缓冲区 -------------------------------------------------
    // 从 32768*2 缩减至 8192，将总延迟控制在 ~250ms 以内
    static constexpr size_t RING_CAPACITY = 8192;

    std::mutex               m_mutex;
    std::condition_variable  m_spaceCV;   ///< 环形缓冲区排空（释放空间）时通知
    std::vector<int16_t>     m_ring;      ///< 循环PCM样本存储
    size_t                   m_writePos   = 0;
    size_t                   m_readPos    = 0;
    size_t                   m_available  = 0;
    size_t                   m_maxLatencySamples = RING_CAPACITY / 2;

    void ringWrite(const int16_t* data, size_t count);
    size_t ringRead(int16_t* out, size_t maxCount);

    // ---- 后台音频线程 -----------------------------------------------
    std::thread       m_thread;
    std::atomic<bool> m_running { false };
    int               m_sampleRate = 32768;
    int               m_channels   = 2;

    void audioThreadFunc();

    // ---- 平台状态（不透明指针，保持头文件简洁）---------------------
    void* m_platformState = nullptr;
};

} // namespace beiklive
