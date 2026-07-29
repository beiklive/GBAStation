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
        constexpr float stickThreshold = 0.35f;

        if (state.buttons[static_cast<int>(brls::BUTTON_LEFT)] ||
            state.buttons[static_cast<int>(brls::BUTTON_NAV_LEFT)] ||
            leftX < -stickThreshold || rightX < -stickThreshold)
            held |= P8_KEY_LEFT;
        if (state.buttons[static_cast<int>(brls::BUTTON_RIGHT)] ||
            state.buttons[static_cast<int>(brls::BUTTON_NAV_RIGHT)] ||
            leftX > stickThreshold || rightX > stickThreshold)
            held |= P8_KEY_RIGHT;
        if (state.buttons[static_cast<int>(brls::BUTTON_UP)] ||
            state.buttons[static_cast<int>(brls::BUTTON_NAV_UP)] ||
            leftY < -stickThreshold || rightY < -stickThreshold)
            held |= P8_KEY_UP;
        if (state.buttons[static_cast<int>(brls::BUTTON_DOWN)] ||
            state.buttons[static_cast<int>(brls::BUTTON_NAV_DOWN)] ||
            leftY > stickThreshold || rightY > stickThreshold)
            held |= P8_KEY_DOWN;
        if (state.buttons[static_cast<int>(brls::BUTTON_A)] ||
            state.buttons[static_cast<int>(brls::BUTTON_Y)] ||
            state.buttons[static_cast<int>(brls::BUTTON_LB)])
            held |= P8_KEY_X;
        if (state.buttons[static_cast<int>(brls::BUTTON_B)] ||
            state.buttons[static_cast<int>(brls::BUTTON_X)] ||
            state.buttons[static_cast<int>(brls::BUTTON_RB)])
            held |= P8_KEY_O;

        if (m_invertButtons) {
            const bool o = (held & P8_KEY_O) != 0;
            const bool x = (held & P8_KEY_X) != 0;
            held &= static_cast<uint8_t>(~(P8_KEY_O | P8_KEY_X));
            if (o) held |= P8_KEY_X;
            if (x) held |= P8_KEY_O;
        }

        if (m_suppressActions) {
            if ((held & (P8_KEY_O | P8_KEY_X)) != 0) {
                m_previousHeld = held;
                return {};
            }
            m_suppressActions = false;
            m_previousHeld = 0;
        }

        InputState result;
        result.held = held;
        result.down = held & static_cast<uint8_t>(~m_previousHeld);
        m_previousHeld = held;
        return result;
    }

    void Input::reset()
    {
        m_previousHeld = 0;
        m_suppressActions = false;
    }

    void Input::suppressActionsUntilRelease()
    {
        m_previousHeld = 0;
        m_suppressActions = true;
    }
}
