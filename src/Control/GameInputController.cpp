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
        // 检查所有注册按键是否同时按下
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
                // 上升沿：触发 Press 并记录时间戳
                action.active         = true;
                action.longPressFired = false;
                action.pressTime      = now;
                action.callback(KeyEvent::Press);
            }
            else if (!action.longPressFired)
            {
                // 持续按下：检查长按阈值
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
                // 下降沿：若无长按则触发 ShortPress，然后触发 Release
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
        // 重置所有动作状态，防止挂起期间按键释放后重新启用时触发误报
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
