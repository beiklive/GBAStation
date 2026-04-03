#pragma once

#include <borealis/core/audio.hpp>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Switch 平台：引入 libpulsar 头文件
#ifdef __SWITCH__
#include <pulsar.h>
#endif

namespace beiklive {

/// borealis UI音效播放器，实现 AudioPlayer 接口。
///
/// 多平台实现：
///   - Nintendo Switch : 通过 libpulsar 加载并播放 qlaunch 官方音效，
///                       完全接管 borealis 原本的 SwitchAudioPlayer。
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
    /// 已加载WAV文件的内存表示（非Switch平台使用）。
    struct WavData
    {
        std::vector<int16_t> samples;   ///< 交错立体声16位PCM
        int  sampleRate = 44100;
        int  channels   = 2;
        bool loaded     = false;        ///< 是否已加载（Switch平台也用此标志）
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
    /// @param soundIdx  brls::Sound 对应的整数索引（Switch平台用于查找 pulsar 句柄）
    /// @param wav       已加载的 WAV 数据（非Switch平台使用）
    /// @param pitch     播放音调倍率
    void playSoundDirect(int soundIdx, const WavData& wav, float pitch);

    static bool        loadWav(const std::string& path, WavData& out);
    static std::string soundFileName(brls::Sound sound);
    static std::string soundsDir();

#ifdef __SWITCH__
    void               _initSwitch();  ///< Switch 平台：初始化 libpulsar 和 BFSAR
    // ---- Switch 平台 libpulsar 状态 ----
    bool               m_switchInit = false;  ///< libpulsar 是否初始化成功
    PLSR_BFSAR         m_qlaunchBfsar{};      ///< qlaunch BFSAR 存档句柄
    PLSR_PlayerSoundId m_switchSounds[brls::_SOUND_MAX]; ///< 各音效的播放器句柄
#endif
};

} // namespace beiklive
