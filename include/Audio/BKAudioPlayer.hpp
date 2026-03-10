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

/// Desktop UI-sound player that implements borealis's AudioPlayer interface.
///
/// Loads 16-bit PCM WAV files from the "resources/sounds/" directory and
/// plays them via the platform's native audio API (ALSA / WinMM / CoreAudio).
/// A single background thread serialises playback; when a new sound is queued
/// while another is still pending (not yet playing), the pending sound is
/// replaced by the newer one so the player always feels responsive.
///
/// Typical usage (main.cpp, after Application::init()):
/// @code
///   auto* player = new beiklive::BKAudioPlayer();
///   brls::Application::setAudioPlayer(player);
/// @endcode
class BKAudioPlayer : public brls::AudioPlayer
{
  public:
    BKAudioPlayer();
    ~BKAudioPlayer() override;

    bool load(brls::Sound sound) override;
    bool play(brls::Sound sound, float pitch = 1.0f) override;

  private:
    /// In-memory representation of a loaded WAV file.
    struct WavData
    {
        std::vector<int16_t> samples;   ///< Interleaved stereo 16-bit PCM
        int  sampleRate = 44100;
        int  channels   = 2;
        bool loaded     = false;
    };

    WavData m_sounds[brls::_SOUND_MAX];

    // ---- Background playback thread ----
    std::thread             m_thread;
    std::mutex              m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool>       m_running { false };

    // Latest-wins pending slot (replaces unplayed pending sound)
    bool        m_hasPending  = false;
    int         m_pendingIdx  = 0;
    float       m_pendingPitch = 1.0f;

    void playbackThread();
    void playSoundDirect(const WavData& wav, float pitch);

    static bool        loadWav(const std::string& path, WavData& out);
    static std::string soundFileName(brls::Sound sound);
    static std::string soundsDir();
};

} // namespace beiklive
