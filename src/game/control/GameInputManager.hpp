#pragma once

#include "core/Singleton.hpp"
#include "InputMap.hpp"
#include <borealis.hpp>
#include <optional>

namespace beiklive
{

    /*
    updateControllerState   获取指定按键索引的状态
    updateUnifiedControllerState  获取当前所有手柄按键的状态
    */

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

        bool compareControllerState(const brls::ControllerState &a, const brls::ControllerState &b);
        const char *getPadEnumString(int value);
        const char *getAxisEnumString(int value);

    private:
        bool inputDropped = false;
        bool inputEnabled = true;

        GamepadState lastGamepadStates[GAMEPADS_MAX];

        // 处理控制器的输入
        void handleControllerInput();
        GamepadState getControllerState(int controllerNum);
    };
}
