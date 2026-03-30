#pragma once

#include "core/Singleton.hpp"
#include "core/enums.h"
// 引入 libretro 手柄按键 ID。
#include "third_party/mgba/src/platform/libretro/libretro.h"
#include <borealis.hpp>
#include <optional>
#include <functional>


using BrlsButtonMatrix = std::vector<std::vector<int>>;

namespace beiklive
{


    struct InputMap 
    {
        // retro_btn
        int retro_id;
        // 对应的 borealis 按键列表（可能是多组组合键，如 "LB+START"和 "RB+BACK"同时对应一个 retro 按键）
        BrlsButtonMatrix brls_buttons_list;
        std::string displayName; // 显示名称
    };

    struct HotkeyBinding
    {
        EmuFunctionKey emuKey;
        BrlsButtonMatrix buttons; // 可能是多组组合键，如 "LB+START"和 "RB+BACK"同时对应一个热键
        std::function<void()> callback; // 热键触发时的回调函数
    };

    // Moonlight ready gamepad
    struct GamepadState
    {
        short buttonFlags = 0;
        unsigned char leftTrigger = 0;
        unsigned char rightTrigger = 0;
        short leftStickX = 0;
        short leftStickY = 0;
        short rightStickX = 0;
        short rightStickY = 0;

        bool is_equal(GamepadState other)
        {
            return buttonFlags == other.buttonFlags &&
                   leftTrigger == other.leftTrigger &&
                   rightTrigger == other.rightTrigger &&
                   leftStickX == other.leftStickX &&
                   leftStickY == other.leftStickY &&
                   rightStickX == other.rightStickX &&
                   rightStickY == other.rightStickY;
        }
    };

    class GameInputManager : public Singleton<GameInputManager>
    {
    public:
        GameInputManager();
        void sayHello();

        void dropInput();
        void handleInput(bool ignoreTouch = false);

        void setInputEnabled(bool enabled) { inputEnabled = enabled; }
        bool isInputEnabled() const { return inputEnabled; }

        GamepadState getGamepadState(int controllerNum);
        int getControllerCount() const
        {
            return brls::Application::getPlatform()
                ->getInputManager()
                ->getControllersConnectedCount();
        }

        // 注册一个模拟器功能键的回调函数，当对应的按键组合被按下时调用回调函数
        void registerEmuFunctionKey(
                EmuFunctionKey emuKey, 
                BrlsButtonMatrix buttons, 
                std::function<void()> callback);
        void clearEmuFunctionKeys();

    private:
        bool inputDropped = false;
        bool inputEnabled = true;
        GamepadState lastGamepadStates[GAMEPADS_MAX];

        std::vector<HotkeyBinding> hotkeyBindings;
        std::vector<int> activeInputs; // 当前正在按下的热键列表

        // 处理控制器的输入
        void handleControllerInput();
        GamepadState getControllerState(int controllerNum);
    };

    
    // std::vector<brls::ControllerButton> parseButton(const GamepadState& state);
    // void printGamepadState(const GamepadState& state);
}
