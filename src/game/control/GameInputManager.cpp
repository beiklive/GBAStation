#include "GameInputManager.hpp"

namespace beiklive
{
    float fsqrt_(float f)
    {
        int i = *(int *)&f;
        i = (i >> 1) + 0x1fbb67ae;
        float f1 = *(float *)&i;
        return 0.5F * (f1 + f / f1);
    }

    // std::vector<brls::ControllerButton> parseButton(const GamepadState &state)
    // {
    //     short code = state.buttonFlags;
    //     std::vector<brls::ControllerButton> buttons;
    //     if (code & UP_FLAG)
    //         buttons.push_back(brls::BUTTON_UP);
    //     if (code & DOWN_FLAG)
    //         buttons.push_back(brls::BUTTON_DOWN);
    //     if (code & LEFT_FLAG)
    //         buttons.push_back(brls::BUTTON_LEFT);
    //     if (code & RIGHT_FLAG)
    //         buttons.push_back(brls::BUTTON_RIGHT);
    //     if (code & A_FLAG)
    //         buttons.push_back(brls::BUTTON_A);
    //     if (code & B_FLAG)
    //         buttons.push_back(brls::BUTTON_B);
    //     if (code & X_FLAG)
    //         buttons.push_back(brls::BUTTON_X);
    //     if (code & Y_FLAG)
    //         buttons.push_back(brls::BUTTON_Y);
    //     if (code & BACK_FLAG)
    //         buttons.push_back(brls::BUTTON_BACK);
    //     if (code & PLAY_FLAG)
    //         buttons.push_back(brls::BUTTON_START);
    //     if (code & LB_FLAG)
    //         buttons.push_back(brls::BUTTON_LB);
    //     if (code & RB_FLAG)
    //         buttons.push_back(brls::BUTTON_RB);
    //     if(state.leftTrigger > 0)
    //         buttons.push_back(brls::BUTTON_LT);
    //     if(state.rightTrigger > 0)
    //         buttons.push_back(brls::BUTTON_RT);

    //     return buttons;
    // }

    // void printGamepadState(const GamepadState &state)
    // {
    //     std::vector<brls::ControllerButton> buttons = parseButton(state);
    //     if(buttons.empty())
    //     {
    //         return;
    //     }
    //     std::string buttonStr;
    //     for (auto button : buttons)
    //     {
    //         for (const auto &it : beiklive::input::k_brlsNames)
    //         {
    //             if (it.id == button)
    //             {
    //                 buttonStr += it.name;
    //                 buttonStr.push_back(' ');
    //                 break;
    //             }
    //         }
    //     }
    //     brls::Logger::debug("GamepadState: buttons: [{}]",
    //                         buttonStr);
    // }

    GameInputManager::GameInputManager()
    {
        auto inputManager = brls::Application::getPlatform()->getInputManager();
        if (inputManager)
        {
            // 订阅键盘按键状态变化事件，更新输入状态
            inputManager
                ->getKeyboardKeyStateChanged()
                ->subscribe([this](brls::KeyState state)
                            {
                                if (!inputEnabled)
                                    return;

                                brls::Logger::debug("GameInputManager: Key {} {} with mods {}", static_cast<int>(state.key), state.pressed, state.mods);
                                // 这里可以根据 state.key 和 state.mods 更新游戏输入状态
                            });

            // 获取陀螺仪和加速度
            inputManager
                ->getControllerSensorStateChanged()
                ->subscribe([this](brls::SensorEvent event)
                            {
                    if (!inputEnabled) return;
                    
                    switch (event.type) {
                        case brls::SensorEventType::ACCEL:
                            brls::Logger::debug("GameInputManager: Controller {} ACCEL data: x={} y={} z={}", event.controllerIndex, event.data[0], event.data[1], event.data[2]);
                            break;
                        case brls::SensorEventType::GYRO:
                            brls::Logger::debug("GameInputManager: Controller {} GYRO data: x={} y={} z={}", event.controllerIndex,
                                event.data[0] * 57.2957795f, 
                                event.data[1] * 57.2957795f, 
                                event.data[2] * 57.2957795f);
                            break;
                    } });
        }
    }

    void GameInputManager::sayHello()
    {
        brls::Logger::info("Hello from GameInputManager!");
    }

    void GameInputManager::dropInput()
    {
        if (inputDropped)
            return;
        bool res = true;
        // Drop gamepad state
        GamepadState gamepadState;
        auto controllersCount = brls::Application::getPlatform()
                                    ->getInputManager()
                                    ->getControllersConnectedCount();

        for (int i = 0; i < controllersCount; i++)
        {
            // 全部清空
            lastGamepadStates[i] = gamepadState;
        }
    }

    void GameInputManager::handleInput(bool ignoreTouch)
    {
        inputDropped = false;

        // 处理输入状态变化，转换为游戏逻辑需要的格式
        if (!inputEnabled)
            return;
        handleControllerInput();
    }

    void GameInputManager::handleControllerInput()
    {
        static int lastControllerCount = 0;

        // 获取所有控制器数量
        auto controllersCount = brls::Application::getPlatform()
                                    ->getInputManager()
                                    ->getControllersConnectedCount();
        int i = 0;
#ifdef __SWITCH__
        // 遍历控制器获取状态并处理输入，最多GAMEPADS_MAX个控制器
        for (i = 0; i < controllersCount; i++)
        {
#endif

            // 获取指定控制器的状态
            GamepadState gamepadState = getControllerState(i);
            // 状态有变化
            if (!gamepadState.is_equal(lastGamepadStates[i]))
            {
                // 更新对应控制器的状态
                lastGamepadStates[i] = gamepadState;
                // TODO: 按键检测机制可以再改进，目前为简单的严格判断，对摇杆输入处理不佳
                if (!activeInputs.empty())
                {

                    // 热键检查，严格匹配模式
                    std::set<int> activeSet(activeInputs.begin(), activeInputs.end());

                    for (auto &hk : hotkeyBindings)
                    {
                        for (auto &combo : hk.buttons)
                        {
                            if (combo.size() != activeSet.size())
                                continue;

                            bool match = true;
                            for (auto &btn : combo)
                            {
                                if (activeSet.find(btn) == activeSet.end())
                                {
                                    match = false;
                                    break;
                                }
                            }

                            if (match)
                            {
                                hk.callback();
                                break;
                            }
                        }
                    }

                    auto printactiveInputs = [&]()
                    {
                        std::string activeStr;
                        for (int input : activeInputs)
                        {
                            for (const auto &it : beiklive::k_gameInputNames)
                            {
                                if (it.id == input)
                                {
                                    activeStr += it.name;
                                    activeStr.push_back(' ');
                                    break;
                                }
                            }
                        }
                        brls::Logger::debug("Active Inputs: [{}]", activeStr);
                    };
                    printactiveInputs();
                }
#ifdef __SWITCH__

                // 检测手柄数量变化，发送特定消息通知游戏
                if (lastControllerCount != controllersCount)
                {
                    lastControllerCount = controllersCount;

                    for (int i = 0; i < controllersCount; i++)
                    {
                        brls::Logger::debug("GameInputManager: Controller #{} connected", i);
                        std::string buttonStr = "控制器" + std::to_string(i) + "已连接";
                        brls::Application::notify(buttonStr);
                    }
                }
#endif
            }
#ifdef __SWITCH__
        }
#endif
    }
    GamepadState GameInputManager::getControllerState(int controllerNum)
    {
        activeInputs.clear();
        brls::ControllerState rawController{};
        brls::ControllerState controller{};

// brls::Application::setSwapHalfJoyconStickToDpad(Settings::instance().swap_joycon_stick_to_dpad());
#ifdef __SWITCH__
        brls::Application::getPlatform()->getInputManager()->updateControllerState(
            &rawController, controllerNum);
#else
        brls::Application::getPlatform()
            ->getInputManager()
            ->updateUnifiedControllerState(&rawController);
#endif
        // 防止以后按键需要调整映射，先把原始输入状态保存下来，后续处理都基于这个状态进行转换
        controller = rawController;

        // 开始处理控制器输入，转换为GamepadState格式
        // 处理线性触发器输入（LT和RT），如果轴值大于0则使用轴值，否则根据按钮状态设置为1或0
        float lzAxis = controller.axes[brls::LEFT_Z] > 0 ? controller.axes[brls::LEFT_Z] : (controller.buttons[brls::BUTTON_LT] ? 1.f : 0.f);
        float rzAxis = controller.axes[brls::RIGHT_Z] > 0 ? controller.axes[brls::RIGHT_Z] : (controller.buttons[brls::BUTTON_RT] ? 1.f : 0.f);

        // 处理摇杆死区，TODO: 以后如果需要调整死区可以在这里修改
        float leftStickDeadzone = 0.0f;
        float rightStickDeadzone = 0.0f;

        float leftXAxis = controller.axes[brls::LEFT_X];
        float leftYAxis = controller.axes[brls::LEFT_Y];
        float rightXAxis = controller.axes[brls::RIGHT_X];
        float rightYAxis = controller.axes[brls::RIGHT_Y];

        if (leftStickDeadzone > 0)
        {
            float magnitude = fsqrt_(std::powf(leftXAxis, 2) + std::powf(leftYAxis, 2));
            if (magnitude < leftStickDeadzone)
            {
                leftXAxis = 0;
                leftYAxis = 0;
            }
        }

        if (rightStickDeadzone > 0)
        {
            float magnitude = fsqrt_(std::powf(rightXAxis, 2) + std::powf(rightYAxis, 2));
            if (magnitude < rightStickDeadzone)
            {
                rightXAxis = 0;
                rightYAxis = 0;
            }
        }

        // 线性扳机和手柄的摇杆轴值范围是-1到1的浮点数，游戏需要的范围是0到255或者-32768到32767，所以需要进行转换
        GamepadState gamepadState{
            .buttonFlags = 0,
            .leftTrigger = static_cast<unsigned char>(
                0xFF * lzAxis),
            .rightTrigger = static_cast<unsigned char>(
                0xFF * rzAxis),
            .leftStickX = static_cast<short>(
                0x7FFF * leftXAxis),
            .leftStickY = static_cast<short>(
                -0x7FFF * leftYAxis),
            .rightStickX = static_cast<short>(
                0x7FFF * rightXAxis),
            .rightStickY = static_cast<short>(
                -0x7FFF * rightYAxis),
        };

        // 存入手柄状态中，后续处理热键时会用到
        if (gamepadState.leftTrigger >= 0xFF)
        {
            activeInputs.push_back(STATE_PAD_LT);
        }
        if (gamepadState.rightTrigger >= 0xFF)
        {
            activeInputs.push_back(STATE_PAD_RT);
        }
        if (gamepadState.leftStickX != 0)
        {
            activeInputs.push_back(STATE_PAD_LEFT_STICK_X);
        }
        if (gamepadState.leftStickY != 0)
        {
            activeInputs.push_back(STATE_PAD_LEFT_STICK_Y);
        }
        if (gamepadState.rightStickX != 0)
        {
            activeInputs.push_back(STATE_PAD_RIGHT_STICK_X);
        }
        if (gamepadState.rightStickY != 0)
        {
            activeInputs.push_back(STATE_PAD_RIGHT_STICK_Y);
        }

        // 开始逐个处理按钮输入，根据按钮状态设置对应的位
        auto SET_GAME_PAD_STATE = [&](int LIMELIGHT_KEY, int GAMEPAD_BUTTON)
        {
            if (controller.buttons[GAMEPAD_BUTTON])
            {
                gamepadState.buttonFlags |= LIMELIGHT_KEY; // 设置对应位
                activeInputs.push_back(GAMEPAD_BUTTON);     // 记录这个按键被按下了
            }
            else
            {
                gamepadState.buttonFlags &= ~LIMELIGHT_KEY;
            }
        };

        SET_GAME_PAD_STATE(UP_FLAG, brls::BUTTON_UP);
        SET_GAME_PAD_STATE(DOWN_FLAG, brls::BUTTON_DOWN);
        SET_GAME_PAD_STATE(LEFT_FLAG, brls::BUTTON_LEFT);
        SET_GAME_PAD_STATE(RIGHT_FLAG, brls::BUTTON_RIGHT);

        SET_GAME_PAD_STATE(A_FLAG, brls::BUTTON_A);
        SET_GAME_PAD_STATE(B_FLAG, brls::BUTTON_B);
        SET_GAME_PAD_STATE(X_FLAG, brls::BUTTON_X);
        SET_GAME_PAD_STATE(Y_FLAG, brls::BUTTON_Y);

        SET_GAME_PAD_STATE(BACK_FLAG, brls::BUTTON_BACK);
        SET_GAME_PAD_STATE(PLAY_FLAG, brls::BUTTON_START);

        SET_GAME_PAD_STATE(LB_FLAG, brls::BUTTON_LB);
        SET_GAME_PAD_STATE(RB_FLAG, brls::BUTTON_RB);

        SET_GAME_PAD_STATE(LS_CLK_FLAG, brls::BUTTON_LSB);
        SET_GAME_PAD_STATE(RS_CLK_FLAG, brls::BUTTON_RSB);

        return gamepadState;
    }

    GamepadState GameInputManager::getGamepadState(int controllerNum)
    {
        return lastGamepadStates[controllerNum];
    }

    void GameInputManager::registerEmuFunctionKey(EmuFunctionKey emuKey, BrlsButtonMatrix buttons, std::function<void()> callback)
    {
        hotkeyBindings.push_back({emuKey, buttons, callback});
    }

    void GameInputManager::clearEmuFunctionKeys()
    {
        hotkeyBindings.clear();
    }

} // namespace beiklive
