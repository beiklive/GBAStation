#pragma once

#include <borealis/core/audio.hpp>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace beiklive {

/// borealis UI音效播放器，实现 AudioPlayer 接口。
///
/// 多平台实现：
///   - Nintendo Switch : 加载 WAV 文件并通过 libnx audout 播放。
///   - Linux (ALSA)    : 加载 WAV 文件并通过 ALSA 播放。
///   - Windows (WinMM) : 加载 WAV 文件并通过 WinMM 播放。
///   - macOS (CoreAudio): 加载 WAV 文件并通过 CoreAudio 播放。
///
/// 单后台线程串行播放；新音效入队时若有未播放的待处理音效，
/// 则以新音效替换旧的，保持响应及时。
///
/// 典型用法（main.cpp，Application::init()之后）：
/// @code
///   brls::Application::setAudioPlayer(new beiklive::BKAudioPlayer());
/// @endcode
class BKAudioPlayer : public brls::AudioPlayer
{
  public:
    BKAudioPlayer();
    ~BKAudioPlayer() override;

    bool load(brls::Sound sound) override;
    bool play(brls::Sound sound, float pitch = 1.0f) override;

  private:
    /// 已加载WAV文件的内存表示（所有平台使用）。
    struct WavData
    {
        std::vector<int16_t> samples;   ///< 交错立体声16位PCM
        int  sampleRate = 44100;
        int  channels   = 2;
        bool loaded     = false;        ///< 是否已加载
    };

    WavData m_sounds[brls::_SOUND_MAX];

    // ---- 后台播放线程 ----
    std::thread             m_thread;
    std::mutex              m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool>       m_running { false };

    // 最新优先的待处理槽（未播放的旧音效会被新音效替换）
    bool        m_hasPending   = false;
    int         m_pendingIdx   = 0;
    float       m_pendingPitch = 1.0f;

    void playbackThread();
    /// 实际播放音效（平台相关）
    /// @param soundIdx  brls::Sound 对应的整数索引
    /// @param wav       已加载的 WAV 数据
    /// @param pitch     播放音调倍率
    void playSoundDirect(int soundIdx, const WavData& wav, float pitch);

    static bool        loadWav(const std::string& path, WavData& out);
    static std::string soundFileName(brls::Sound sound);
    static std::string soundsDir();

#ifdef __SWITCH__
    void _initSwitch();         ///< Switch 平台：初始化 audout 服务
    bool m_switchInit = false;  ///< audout 是否初始化成功
#endif
};

} // namespace beiklive
