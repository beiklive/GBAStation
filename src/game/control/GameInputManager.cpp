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

        inputDropped = true;

        // 清空手柄状态
        GamepadState emptyState;
        auto controllersCount = brls::Application::getPlatform()
                                    ->getInputManager()
                                    ->getControllersConnectedCount();

        for (int i = 0; i < controllersCount; i++)
        {
            lastGamepadStates[i] = emptyState;
        }

        // 清空按键时间（LONG_PRESS 依赖这个）
        longPressTriggered.clear();

        currentTime = 0;
        activeInputs.clear();
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
            // 更新对应控制器的状态
            lastGamepadStates[i] = gamepadState;
            // 更新时间（假设60FPS）
            currentTime += 16;
            // 更新输入状态
            updateInputState();
            checkHotkeys();
            // 状态有变化
            if (!gamepadState.is_equal(lastGamepadStates[i]))
            {
                printactiveInputs();
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
    void GameInputManager::checkHotkeys()
    {
        for (auto &hk : hotkeyBindings)
        {
            for (auto &combo : hk.buttons)
            {
                bool now = containsCombo(inputState.current, combo);
                bool before = containsCombo(inputState.previous, combo);

                switch (hk.triggerType)
                {
                case TriggerType::PRESS:
                    if (now && !before)
                        hk.callback();
                    break;

                case TriggerType::RELEASE:
                    if (!now && before)
                        hk.callback();
                    break;

                case TriggerType::HOLD:
                    if (now)
                        hk.callback();
                    break;

                case TriggerType::LONG_PRESS:
                {

                    if (now)
                    {
                        uint64_t latestPressTime = 0;
                        for (int key : combo)
                        {
                            // 获取组合键最后一个按下的时间，作为长按的起始时间
                            latestPressTime = std::max(latestPressTime, pressTime[key]);
                        }
                        int comboId = hk.emuKey;

                        if ((currentTime - latestPressTime) > static_cast<uint64_t>(hk.threshold * 1000))
                        {
                            if (!longPressTriggered[comboId])
                            {
                                hk.callback();
                                longPressTriggered[comboId] = true;
                            }
                        }
                    }
                    
                    if(!now && before) // 前后帧检测松开
                    {
                        // 松开后重置
                        longPressTriggered[hk.emuKey] = false;
                    }
                    break;
                }
                }
            }
        }
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
            float magnitude = fsqrt_(std::pow(leftXAxis, 2) + std::pow(leftYAxis, 2));
            if (magnitude < leftStickDeadzone)
            {
                leftXAxis = 0;
                leftYAxis = 0;
            }
        }

        if (rightStickDeadzone > 0)
        {
            float magnitude = fsqrt_(std::pow(rightXAxis, 2) + std::pow(rightYAxis, 2));
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
        processStick(leftXAxis, leftYAxis,
                     STATE_PAD_LEFT_STICK_X,
                     STATE_PAD_LEFT_STICK_Y,
                     STATE_PAD_LEFT_STICK_LEFT,
                     STATE_PAD_LEFT_STICK_RIGHT,
                     STATE_PAD_LEFT_STICK_UP,
                     STATE_PAD_LEFT_STICK_DOWN);

        processStick(rightXAxis, rightYAxis,
                     STATE_PAD_RIGHT_STICK_X,
                     STATE_PAD_RIGHT_STICK_Y,
                     STATE_PAD_RIGHT_STICK_LEFT,
                     STATE_PAD_RIGHT_STICK_RIGHT,
                     STATE_PAD_RIGHT_STICK_UP,
                     STATE_PAD_RIGHT_STICK_DOWN);

        // 开始逐个处理按钮输入，根据按钮状态设置对应的位
        auto SET_GAME_PAD_STATE = [&](int LIMELIGHT_KEY, int GAMEPAD_BUTTON)
        {
            if (controller.buttons[GAMEPAD_BUTTON])
            {
                gamepadState.buttonFlags |= LIMELIGHT_KEY; // 设置对应位
                activeInputs.push_back(GAMEPAD_BUTTON);    // 记录这个按键被按下了
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

    void GameInputManager::updateInputState()
    {
        inputState.previous = inputState.current;
        inputState.current.clear();

        for (int input : activeInputs)
        {
            inputState.current.insert(input);

            if (!inputState.previous.count(input))
            {
                pressTime[input] = currentTime;
            }
        }
    }

    bool GameInputManager::containsCombo(const std::set<int> &active, const std::vector<int> &combo)
    {
        for (int key : combo)
        {
            if (!active.count(key))
                return false;
        }
        return true;
    }

    bool GameInputManager::isComboJustTriggered(const std::vector<int> &combo)
    {
        bool now = containsCombo(inputState.current, combo);
        bool before = containsCombo(inputState.previous, combo);

        return now && !before;
    }

    void GameInputManager::processStick(float x, float y, int axisX, int axisY,
                                        int dirLeft, int dirRight, int dirUp, int dirDown)
    {
        const float DEADZONE = 0.2f;
        const float AXIS_DOMINANCE = 1.5f;

        float absX = std::abs(x);
        float absY = std::abs(y);

        if (absX < DEADZONE && absY < DEADZONE)
            return;

        if (m_diagonalMode) {
            // 斜向模式：允许 X 和 Y 轴同时激活
            if (absX >= DEADZONE) {
                activeInputs.push_back(axisX);
                activeInputs.push_back(x > 0 ? dirRight : dirLeft);
            }
            if (absY >= DEADZONE) {
                activeInputs.push_back(axisY);
                activeInputs.push_back(y > 0 ? dirDown : dirUp);
            }
        } else {
            // 非斜向模式：仅触发绝对值更大的轴方向
            if (absX > absY * AXIS_DOMINANCE)
            {
                // 水平方向为主
                activeInputs.push_back(axisX);
                activeInputs.push_back(x > 0 ? dirRight : dirLeft);
            }
            else if (absY > absX * AXIS_DOMINANCE)
            {
                // 垂直方向为主（brls Y轴：正值朝下，负值朝上）
                activeInputs.push_back(axisY);
                activeInputs.push_back(y > 0 ? dirDown : dirUp);
            }
        }
    }

    bool GameInputManager::isLongPress(int key, float threshold)
    {
        if (!inputState.current.count(key))
            return false;
        return (currentTime - pressTime[key]) > threshold;
    }

    bool GameInputManager::isShortPress(int key, float threshold)
    {
        if (!inputState.previous.count(key) || inputState.current.count(key))
            return false;

        return (currentTime - pressTime[key]) < threshold;
    }

    void GameInputManager::printactiveInputs()
    {
        if (activeInputs.empty())
        {
            return;
        }
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
    }

    GamepadState GameInputManager::getGamepadState(int controllerNum)
    {
        return lastGamepadStates[controllerNum];
    }

    void GameInputManager::registerEmuFunctionKey(EmuFunctionKey emuKey, BrlsButtonMatrix buttons, std::function<void()> callback, TriggerType type, float threshold)
    {
        hotkeyBindings.push_back({emuKey, buttons, callback, type, threshold});
    }

    void GameInputManager::clearEmuFunctionKeys()
    {
        hotkeyBindings.clear();
    }

} // namespace beiklive
