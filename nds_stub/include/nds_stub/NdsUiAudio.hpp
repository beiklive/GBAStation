#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

#include "nds_stub/NdsMenuLayer.hpp"

namespace beiklive::nds_stub {

class NdsUiAudioPlayer {
public:
    NdsUiAudioPlayer();
    ~NdsUiAudioPlayer();

    void stop();
    bool play(NdsMenuSound sound, float pitch = 1.0f);

private:
    struct WavData {
        std::vector<std::int16_t> samples;
        int sampleRate = 44100;
        int channels = 2;
        bool loaded = false;
    };

    enum class SoundSlot {
        Focus,
        Click,
        Back,
        Error,
        Slider,
        Count,
    };

    bool load(SoundSlot slot);
    void playbackThread();
    void playSoundDirect(const WavData& wav, float pitch);
    static SoundSlot slotForMenuSound(NdsMenuSound sound);
    static const char* soundFileName(SoundSlot slot);
    static bool loadWav(const char* path, WavData& out);

    bool m_init = false;
    std::array<WavData, static_cast<std::size_t>(SoundSlot::Count)> m_sounds {};
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running { false };
    bool m_hasPending = false;
    SoundSlot m_pendingSlot = SoundSlot::Click;
    float m_pendingPitch = 1.0f;
};

} // namespace beiklive::nds_stub
