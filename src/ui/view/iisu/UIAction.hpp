#pragma once

namespace beiklive
{
    /// 抽象 UI 动作：HID → InputManager → UIAction → 各组件（解耦手柄与 UI 逻辑）
    enum class UIAction
    {
        Up,
        Down,
        Left,
        Right,
        Confirm,
        Back,
    };
} // namespace beiklive
