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
/// 从 "resources/sounds/switch/" 目录加载16位PCM WAV文件，
/// 通过平台原生音频API（ALSA / WinMM / CoreAudio）播放。
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
    /// 已加载WAV文件的内存表示。
    struct WavData
    {
        std::vector<int16_t> samples;   ///< 交错立体声16位PCM
        int  sampleRate = 44100;
        int  channels   = 2;
        bool loaded     = false;
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
    void playSoundDirect(const WavData& wav, float pitch);

    static bool        loadWav(const std::string& path, WavData& out);
    static std::string soundFileName(brls::Sound sound);
    static std::string soundsDir();
};

} // namespace beiklive
