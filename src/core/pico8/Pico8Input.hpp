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
        void suppressActionsUntilRelease();
        void setInvertButtons(bool invert) { m_invertButtons = invert; }

    private:
        uint8_t m_previousHeld = 0;
        bool m_suppressActions = false;
        bool m_invertButtons = false;
    };
}
