#pragma once

#include "core/Singleton.hpp"
#include "core/enums.h"
#include <borealis.hpp>
#include <optional>
#include <set>
#include <map>
#include <vector>
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

    struct InputState
    {
        std::set<int> current;
        std::set<int> previous;

        bool isPressed(int key) const
        {
            return current.count(key);
        }

        bool isJustPressed(int key) const
        {
            return current.count(key) && !previous.count(key);
        }

        bool isReleased(int key) const
        {
            return !current.count(key) && previous.count(key);
        }
    };
    struct HotkeyBinding
    {
        EmuFunctionKey emuKey;
        BrlsButtonMatrix buttons; // 可能是多组组合键，如 "LB+START"和 "RB+BACK"同时对应一个热键
        std::function<void()> callback; // 热键触发时的回调函数
        TriggerType triggerType = TriggerType::PRESS;
        float threshold = 0.5f; // 长按阈值
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
            std::function<void()> callback,
            TriggerType type = TriggerType::PRESS,
            float threshold = 0.5f);
        void clearEmuFunctionKeys();

    private:
        bool inputDropped = false;
        bool inputEnabled = true;
        GamepadState lastGamepadStates[GAMEPADS_MAX];

        InputState inputState;
        std::map<int, uint64_t > pressTime;
        uint64_t  currentTime = 0;
        // 记录长按状态
        // std::map<int, bool> longPressTriggered;
        // 记录是否触发过长按，避免重复触发
        std::unordered_map<int, bool> longPressTriggered;

        std::vector<HotkeyBinding> hotkeyBindings;
        std::vector<int> activeInputs; // 当前正在按下的热键列表

        // 处理控制器的输入
        void handleControllerInput();
        void checkHotkeys();
        GamepadState getControllerState(int controllerNum);
        void updateInputState();

        bool containsCombo(const std::set<int>& active, const std::vector<int>& combo);
        bool isComboJustTriggered(const std::vector<int>& combo);

        void processStick(float x, float y, int axisX, int axisY);

        bool isLongPress(int key, float threshold = 0.5f);
        bool isShortPress(int key, float threshold = 0.5f);
        
        void printactiveInputs();

    };

    
    // std::vector<brls::ControllerButton> parseButton(const GamepadState& state);
    // void printGamepadState(const GamepadState& state);
}
