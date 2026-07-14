#include "Pico8Input.hpp"

#include "hostVmShared.h"

#include <borealis.hpp>
#include <cmath>

namespace beiklive::pico8
{
    InputState Input::poll()
    {
        const auto& state = brls::Application::getControllerState();
        uint8_t held = 0;
        const float leftX = state.axes[static_cast<int>(brls::LEFT_X)];
        const float leftY = state.axes[static_cast<int>(brls::LEFT_Y)];
        const float rightX = state.axes[static_cast<int>(brls::RIGHT_X)];
        const float rightY = state.axes[static_cast<int>(brls::RIGHT_Y)];
        const float x = std::abs(rightX) > std::abs(leftX) ? rightX : leftX;
        const float y = std::abs(rightY) > std::abs(leftY) ? rightY : leftY;

        if (state.buttons[static_cast<int>(brls::BUTTON_LEFT)] ||
            state.buttons[static_cast<int>(brls::BUTTON_NAV_LEFT)] ||
            x < -0.42f)
            held |= P8_KEY_LEFT;
        if (state.buttons[static_cast<int>(brls::BUTTON_RIGHT)] ||
            state.buttons[static_cast<int>(brls::BUTTON_NAV_RIGHT)] ||
            x > 0.42f)
            held |= P8_KEY_RIGHT;
        if (state.buttons[static_cast<int>(brls::BUTTON_UP)] ||
            state.buttons[static_cast<int>(brls::BUTTON_NAV_UP)] ||
            y < -0.42f)
            held |= P8_KEY_UP;
        if (state.buttons[static_cast<int>(brls::BUTTON_DOWN)] ||
            state.buttons[static_cast<int>(brls::BUTTON_NAV_DOWN)] ||
            y > 0.42f)
            held |= P8_KEY_DOWN;
        if (state.buttons[static_cast<int>(brls::BUTTON_A)] ||
            state.buttons[static_cast<int>(brls::BUTTON_X)])
            held |= P8_KEY_O;
        if (state.buttons[static_cast<int>(brls::BUTTON_B)] ||
            state.buttons[static_cast<int>(brls::BUTTON_Y)])
            held |= P8_KEY_X;

        InputState result;
        result.held = held;
        result.down = held & static_cast<uint8_t>(~m_previousHeld);
        m_previousHeld = held;
        return result;
    }

    void Input::reset()
    {
        m_previousHeld = 0;
    }
}
