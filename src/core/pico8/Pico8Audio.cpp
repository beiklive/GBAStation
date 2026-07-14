#include "Pico8Audio.hpp"

#include "game/audio/AudioManager.hpp"

#include <algorithm>
#include <cmath>

namespace beiklive::pico8
{
    namespace
    {
        constexpr size_t FADE_SAMPLES =
            static_cast<size_t>(22050 * 2 * 0.12f);
    }

    bool Audio::initialize()
    {
        if (m_initialized) {
            m_suspended = false;
            beiklive::AudioManager::instance().flushRingBufferWithFade(20);
            m_fadeSamplesRemaining = FADE_SAMPLES;
            m_fadeSamplesTotal = FADE_SAMPLES;
            return true;
        }
        m_initialized = beiklive::AudioManager::instance().init(22050, 2);
        if (m_initialized) {
            beiklive::AudioManager::instance().configureLatencyMs(70, 150);
            beiklive::AudioManager::instance().setSpeed(1.f);
            beiklive::AudioManager::instance().flushRingBufferWithFade(20);
            m_fadeSamplesRemaining = FADE_SAMPLES;
            m_fadeSamplesTotal = FADE_SAMPLES;
            m_suspended = false;
        }
        return m_initialized;
    }

    void Audio::shutdown()
    {
        if (!m_initialized)
            return;
        beiklive::AudioManager::instance().deinit();
        m_initialized = false;
        m_suspended = false;
    }

    void Audio::submit(const int16_t* samples, size_t frames)
    {
        if (!m_initialized || m_suspended || !samples || frames == 0)
            return;
        constexpr float gain = 3.00f;
        const size_t sampleCount = frames * 2;
        m_gainBuffer.resize(sampleCount);
        for (size_t i = 0; i < sampleCount; ++i) {
            float fade = 1.f;
            if (m_fadeSamplesRemaining > 0 && m_fadeSamplesTotal > 0) {
                const size_t completed =
                    m_fadeSamplesTotal - m_fadeSamplesRemaining;
                const float linear = static_cast<float>(completed) /
                    static_cast<float>(m_fadeSamplesTotal);
                fade = linear * linear * (3.f - 2.f * linear);
                --m_fadeSamplesRemaining;
            }
            const int amplified = static_cast<int>(
                std::lround(static_cast<float>(samples[i]) * gain * fade));
            m_gainBuffer[i] = static_cast<int16_t>(std::max(
                -32768, std::min(32767, amplified)));
        }
        beiklive::AudioManager::instance().pushSamples(
            m_gainBuffer.data(), frames);
    }

    void Audio::pause()
    {
        if (m_initialized) {
            m_suspended = true;
            // Let the short buffered tail drain naturally. AudioManager then
            // fades its last sample to zero on underrun, avoiding a hard cut.
            m_fadeSamplesRemaining = 0;
            m_fadeSamplesTotal = 0;
        }
    }

    void Audio::resume()
    {
        if (m_initialized) {
            beiklive::AudioManager::instance().flushRingBufferWithFade(20);
            m_fadeSamplesRemaining = FADE_SAMPLES;
            m_fadeSamplesTotal = FADE_SAMPLES;
            m_suspended = false;
        }
    }
}
