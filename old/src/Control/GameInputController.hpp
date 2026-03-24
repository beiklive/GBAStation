#pragma once

#include <borealis.hpp>
#include <chrono>
#include <functional>
#include <vector>

namespace beiklive {

/**
 * GameInputController – 原始手柄输入注册系统。
 *
 * 完全绕过borealis导航/事件系统，直接操作每帧捕获的原始ControllerState快照。
 *
 * 功能特性：
 *  - 注册：registerAction(buttons, callback) 将一组按钮绑定到回调函数。
 *  - 事件类型：
 *      Press      – 上升沿（按钮刚被按下）
 *      Release    – 下降沿（按钮松开）
 *      ShortPress – 松开时触发（本次按压无长按则触发）
 *      LongPress  – 持续按压 >= LONG_PRESS_MS 毫秒后触发一次
 *  - 组合键支持：buttons数组含多个条目时，所有按钮同时按下才触发。
 *  - update() 需在每帧游戏循环中以当前ControllerState调用，同步触发回调。
 *  - setEnabled(false) 暂停所有事件分发但不丢弃已注册动作；
 *    setEnabled(true) 恢复。
 *
 * 线程安全：非线程安全。update() 和 registerAction() 须在同一线程调用。
 */
class GameInputController
{
public:
    enum class KeyEvent { Press, Release, ShortPress, LongPress };
    using Callback = std::function<void(KeyEvent)>;

    /// 触发长按事件所需的持续时间（毫秒）。
    static constexpr int LONG_PRESS_MS = 500;

    /**
     * 注册一个或多个手柄按钮的动作。
     *
     * @param buttons  brls::ControllerButton值数组（转换为int）。
     *                 所有按钮同时按下时触发（组合键检测）。
     * @param callback 以 Press、Release 或 LongPress 调用的回调函数。
     *
     * 单键示例：
     *   registerAction({brls::BUTTON_X}, cb);
     *
     * 组合键示例（L + R 同时按下）：
     *   registerAction({brls::BUTTON_LB, brls::BUTTON_RB}, cb);
     */
    void registerAction(std::vector<int> buttons, Callback callback);

    /**
     * 更新按键状态并触发已注册的回调。
     *
     * 每帧调用一次，传入当前手柄快照。控制器禁用时（setEnabled(false)）无操作。
     *
     * 对每个已注册动作：
     *   - 所有按钮刚被按下 → 触发Press回调。
     *   - 所有按钮持续按压 >= LONG_PRESS_MS → 触发LongPress（每次按压仅触发一次）。
     *   - 动作激活期间任意按钮松开：
     *       * 触发ShortPress（本次按压无LongPress时）
     *       * 触发Release（始终触发，在ShortPress之后）
     *
     * 事件顺序保证：同帧内ShortPress始终先于Release分发。
     */
    void update(const brls::ControllerState& state);

    /**
     * 启用或禁用事件分发。
     *
     * 禁用时，update()为空操作：不触发回调，所有动作状态重置，
     * 以防止重新启用时产生虚假事件。已注册动作不会被丢弃，重新启用后正常恢复。
     *
     * 须与update()在同一线程调用，或在游戏线程启动前调用。
     *
     * 注意：禁用时若有按钮处于按下状态，飞行中的动作将被静默丢弃
     * （不触发ShortPress或Release），这是预期行为。
     *
     * 典型用法：
     *   setEnabled(false)  // 菜单打开时暂停
     *   setEnabled(true)   // 游戏重新获得焦点时恢复
     */
    void setEnabled(bool enabled);

    /// 返回控制器当前是否已启用。
    bool isEnabled() const { return m_enabled; }

    /// 移除所有已注册的动作。
    void clear();

private:
    struct Action
    {
        std::vector<int> buttons;
        Callback         callback;
        bool             active        = false; ///< 所有按钮按下时为true
        bool             longPressFired = false; ///< 本次按压已发送LongPress
        std::chrono::steady_clock::time_point pressTime; ///< Press触发时刻
    };

    std::vector<Action> m_actions;
    bool                m_enabled = true; ///< 为false时update()为空操作
};

} // namespace beiklive
