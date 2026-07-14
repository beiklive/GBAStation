#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace beiklive::pico8
{
    class Audio
    {
    public:
        bool initialize();
        void shutdown();
        void submit(const int16_t* samples, size_t frames);
        void pause();
        void resume();
        bool isInitialized() const { return m_initialized; }

    private:
        bool m_initialized = false;
        bool m_suspended = false;
        std::vector<int16_t> m_gainBuffer;
        size_t m_fadeSamplesRemaining = 0;
        size_t m_fadeSamplesTotal = 0;
    };
}
