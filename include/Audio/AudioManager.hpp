#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

namespace beiklive {

/// Universal cross-platform audio manager.
///
/// Maintains a thread-safe ring buffer of int16_t stereo PCM samples.
/// A background thread continuously drains the ring buffer and feeds it
/// to the platform audio output.
///
/// Platform backends:
///   - Nintendo Switch  : libnx audout
///   - Linux            : ALSA (libasound)
///   - Windows          : WinMM (waveOut)
///   - macOS            : CoreAudio (AudioUnit, callback-driven)
///   - Other / fallback : null output (samples discarded)
///
/// Typical usage:
/// @code
///   AudioManager::instance().init(32768, 2);
///   // ...in libretro audio callback:
///   AudioManager::instance().pushSamples(data, frames);
///   // ...on shutdown:
///   AudioManager::instance().deinit();
/// @endcode
class AudioManager {
public:
    static AudioManager& instance();

    /// Initialize audio subsystem.
    /// @param sampleRate  Target sample rate in Hz  (mGBA default: 32768).
    /// @param channels    Number of audio channels  (mGBA default: 2 stereo).
    /// @return true if audio output was successfully opened.
    bool init(int sampleRate = 32768, int channels = 2);

    /// Push @a frames frames of interleaved stereo 16-bit PCM into the buffer.
    /// Blocks if the ring buffer fill level exceeds the configured threshold,
    /// which keeps game/audio in sync and prevents latency buildup.
    /// Thread-safe – safe to call from the libretro audio callback.
    void pushSamples(const int16_t* data, size_t frames);

    /// Shut down the audio subsystem and stop the background thread.
    void deinit();

    bool isRunning() const { return m_running.load(); }

    /// Set maximum allowed ring-buffer fill (in stereo frames) before
    /// pushSamples() starts blocking.  Call after init().
    /// Default: RING_CAPACITY / 2.
    void setMaxLatencyFrames(size_t frames) { m_maxLatencySamples = frames * static_cast<size_t>(m_channels); }

    /// Discard all samples currently held in the ring buffer and wake any
    /// pushSamples() caller that is blocked.  Use this when switching from
    /// fast-forward back to normal speed to prevent stale audio being played.
    void flushRingBuffer();

private:
    AudioManager()  = default;
    ~AudioManager() = default;

    AudioManager(const AudioManager&)            = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // ---- Ring buffer ------------------------------------------------
    // Reduced from 32768*2 to 8192 to keep total latency below ~250ms
    static constexpr size_t RING_CAPACITY = 8192;

    std::mutex               m_mutex;
    std::condition_variable  m_spaceCV;   ///< Notified when ring drains (space freed)
    std::vector<int16_t>     m_ring;      ///< Circular PCM sample storage
    size_t                   m_writePos   = 0;
    size_t                   m_readPos    = 0;
    size_t                   m_available  = 0;
    size_t                   m_maxLatencySamples = RING_CAPACITY / 2;

    void ringWrite(const int16_t* data, size_t count);
    size_t ringRead(int16_t* out, size_t maxCount);

    // ---- Background audio thread ------------------------------------
    std::thread       m_thread;
    std::atomic<bool> m_running { false };
    int               m_sampleRate = 32768;
    int               m_channels   = 2;

    void audioThreadFunc();

    // ---- Platform state (opaque pointers to keep header clean) ------
    void* m_platformState = nullptr;
};

} // namespace beiklive
