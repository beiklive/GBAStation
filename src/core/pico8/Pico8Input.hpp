#pragma once

#include <cstdint>

namespace beiklive::pico8
{
    struct InputState
    {
        uint8_t down = 0;
        uint8_t held = 0;
    };

    class Input
    {
    public:
        InputState poll();
        void reset();

    private:
        uint8_t m_previousHeld = 0;
    };
}
