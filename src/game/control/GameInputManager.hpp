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

    // 方向键
    #define UP_FLAG        0x00000001
    #define DOWN_FLAG      0x00000002
    #define LEFT_FLAG      0x00000004
    #define RIGHT_FLAG     0x00000008

    // ABXY
    #define A_FLAG         0x00000010
    #define B_FLAG         0x00000020
    #define X_FLAG         0x00000040
    #define Y_FLAG         0x00000080

    // 功能键
    #define BACK_FLAG      0x00000100
    #define PLAY_FLAG      0x00000200   // START

    // 肩键
    #define LB_FLAG        0x00000400
    #define RB_FLAG        0x00000800

    // 摇杆按压
    #define LS_CLK_FLAG    0x00001000
    #define RS_CLK_FLAG    0x00002000




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

    private:
        bool inputDropped = false;
        bool inputEnabled = true;

        std::vector<brls::ControllerButton> SPECIAL_FLAG_COMBO = {brls::BUTTON_LB, brls::BUTTON_RB, brls::BUTTON_BACK, brls::BUTTON_START};
        GamepadState lastGamepadStates[GAMEPADS_MAX];

        // 处理控制器的输入
        void handleControllerInput();
        GamepadState getControllerState(int controllerNum);
    };

    
    std::vector<brls::ControllerButton> parseButton(const GamepadState& state);
    void printGamepadState(const GamepadState& state);
}
