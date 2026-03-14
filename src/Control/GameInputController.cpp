#include "Control/GameInputController.hpp"
#include <borealis/core/logger.hpp>

namespace beiklive {

void GameInputController::registerAction(std::vector<int> buttons, Callback callback)
{
    if (buttons.empty())
    {
        brls::Logger::warning("GameInputController: registerAction called with empty buttons array – ignored");
        return;
    }
    if (!callback)
    {
        brls::Logger::warning("GameInputController: registerAction called with null callback – ignored");
        return;
    }
    Action a;
    a.buttons  = std::move(buttons);
    a.callback = std::move(callback);
    m_actions.push_back(std::move(a));
}

void GameInputController::update(const brls::ControllerState& state)
{
    if (!m_enabled) return;

    const auto now = std::chrono::steady_clock::now();

    for (auto& action : m_actions)
    {
        // Check whether ALL registered buttons are currently pressed.
        bool allPressed = true;
        for (int btn : action.buttons)
        {
            if (btn < 0 || btn >= static_cast<int>(brls::_BUTTON_MAX) ||
                !state.buttons[btn])
            {
                allPressed = false;
                break;
            }
        }

        if (allPressed)
        {
            if (!action.active)
            {
                // Rising edge: fire Press and record the press timestamp.
                action.active         = true;
                action.longPressFired = false;
                action.pressTime      = now;
                action.callback(KeyEvent::Press);
            }
            else if (!action.longPressFired)
            {
                // Still pressed – check for long-press threshold.
                auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     now - action.pressTime).count();
                if (elapsedMs >= LONG_PRESS_MS)
                {
                    action.longPressFired = true;
                    action.callback(KeyEvent::LongPress);
                }
            }
        }
        else
        {
            if (action.active)
            {
                // Falling edge: fire ShortPress if no LongPress occurred,
                // then fire Release unconditionally.
                action.active = false;
                if (!action.longPressFired)
                {
                    action.callback(KeyEvent::ShortPress);
                }
                action.longPressFired = false;
                action.callback(KeyEvent::Release);
            }
        }
    }
}

void GameInputController::setEnabled(bool enabled)
{
    if (m_enabled == enabled) return;
    m_enabled = enabled;

    if (!enabled)
    {
        // Reset all action states so that no stale "active" press lingers.
        // This prevents a spurious Release/ShortPress firing when the
        // controller is re-enabled after the hardware buttons have been
        // released while we were suspended.
        for (auto& action : m_actions)
        {
            action.active         = false;
            action.longPressFired = false;
        }
    }
}

void GameInputController::clear()
{
    m_actions.clear();
}

} // namespace beiklive
