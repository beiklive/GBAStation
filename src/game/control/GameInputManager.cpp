#include "GameInputManager.hpp"
#include "game/control/InputMappingDefaults.hpp"
#include "core/Tools.hpp"

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
        for (auto& playerInput : m_playerInputs)
            playerInput.buttonMask = 0;
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
#ifdef __SWITCH__
        static int lastControllerCount = 0;
#endif

        // 获取所有控制器数量
        auto controllersCount = brls::Application::getPlatform()
                                    ->getInputManager()
                                    ->getControllersConnectedCount();
        updatePlayerAssignments(controllersCount);
        int pollCount = controllersCount;
#ifndef __SWITCH__
        if (pollCount <= 0)
            pollCount = 1;
#endif
        if (pollCount > GAMEPADS_MAX)
            pollCount = GAMEPADS_MAX;

        for (int i = 0; i < pollCount; i++)
        {
            GamepadState gamepadState = getControllerState(i);
            GamepadState prevGamepadState = lastGamepadStates[i];
            lastGamepadStates[i] = gamepadState;
            currentTime += 16;

            if (!gamepadState.is_equal(prevGamepadState))
            {
                printactiveInputs();
#ifdef __SWITCH__
                if (lastControllerCount != controllersCount)
                {
                    lastControllerCount = controllersCount;
                    for (int i = 0; i < controllersCount; i++)
                    {
                        brls::Logger::debug("GameInputManager: Controller #{} connected", i);
                        std::string buttonStr = "手柄 " + std::to_string(i) + " 已连接";
                        brls::Application::notify(buttonStr);
                    }
                }
#endif
            }
        }
        rebuildActiveInputsForHotkeys(pollCount);
        updatePlayerStates();
        updateInputState();
        checkHotkeys();
        updatePreviousNesFunctionStates();
    }
    void GameInputManager::checkHotkeys()
    {
        for (auto &hk : hotkeyBindings)
        {
            if (isNesDualPlayerMode() && hk.emuKey == EmuFunctionKey::EMU_OPEN_MENU)
            {
                if (consumeNesPlayerMenuPress())
                    hk.callback();
                continue;
            }
            if (isNesDualPlayerMode() &&
                (hk.emuKey == EmuFunctionKey::EMU_FAST_FORWARD ||
                 hk.emuKey == EmuFunctionKey::EMU_REWIND))
            {
                if (consumeNesFunctionPress(hk.emuKey, hk.triggerType))
                    hk.callback();
                continue;
            }

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
        brls::ControllerState rawController{};
        brls::ControllerState controller{};

// brls::Application::setSwapHalfJoyconStickToDpad(Settings::instance().swap_joycon_stick_to_dpad());
#ifdef __SWITCH__
        brls::Application::getPlatform()->getInputManager()->updateControllerState(
            &rawController, controllerNum);
#else
        auto* im = brls::Application::getPlatform()->getInputManager();
        if (im && im->getControllersConnectedCount() > controllerNum)
            im->updateControllerState(&rawController, controllerNum);
        else if (im)
            im->updateUnifiedControllerState(&rawController);
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

        // 开始逐个处理按钮输入，根据按钮状态设置对应的位
        auto SET_GAME_PAD_STATE = [&](int LIMELIGHT_KEY, int GAMEPAD_BUTTON)
        {
            if (controller.buttons[GAMEPAD_BUTTON])
            {
                gamepadState.buttonFlags |= LIMELIGHT_KEY; // 设置对应位
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

    PlayerInputState GameInputManager::getPlayerInputState(int playerIndex) const
    {
        if (playerIndex < 0 || playerIndex >= GAME_INPUT_MAX_PLAYERS)
            return {};
        return m_playerInputs[playerIndex];
    }

    uint32_t GameInputManager::getControllerButtonMask(int controllerIndex) const
    {
        if (controllerIndex < 0 || controllerIndex >= GAMEPADS_MAX)
            return 0;
        return buildMaskFromGamepadState(lastGamepadStates[controllerIndex]);
    }

    int GameInputManager::getAssignedControllerForPlayer(int playerIndex) const
    {
        if (playerIndex < 0 || playerIndex >= GAME_INPUT_MAX_PLAYERS)
            return -1;
        return m_playerAssignments[playerIndex];
    }

    void GameInputManager::registerEmuFunctionKey(EmuFunctionKey emuKey, BrlsButtonMatrix buttons, std::function<void()> callback, TriggerType type, float threshold)
    {
        hotkeyBindings.push_back({emuKey, buttons, callback, type, threshold});
    }

    void GameInputManager::clearEmuFunctionKeys()
    {
        hotkeyBindings.clear();
    }

    void GameInputManager::updatePlayerAssignments(int controllersCount)
    {
        m_playerAssignments[0] = controllersCount > 0 ? 0 : -1;
        m_playerAssignments[1] = controllersCount > 1 ? 1 : -1;
    }

    void GameInputManager::updatePlayerStates()
    {
        for (auto& playerInput : m_playerInputs)
            playerInput.buttonMask = 0;

        for (int player = 0; player < GAME_INPUT_MAX_PLAYERS; ++player)
        {
            int controllerIndex = m_playerAssignments[player];
            if (controllerIndex < 0 || controllerIndex >= GAMEPADS_MAX)
                continue;

            m_playerInputs[player].buttonMask = buildMaskFromGamepadState(lastGamepadStates[controllerIndex]);
        }
    }

    bool GameInputManager::isNesDualPlayerMode() const
    {
        return m_activePlatform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNES) &&
               m_nesDualPlayerEnabled;
    }

    bool GameInputManager::consumeNesPlayerMenuPress()
    {
        bool triggered = false;
        for (int player = 0; player < GAME_INPUT_MAX_PLAYERS; ++player)
        {
            if (m_nesMenuPressed[player] && !m_prevNesMenuPressed[player])
                triggered = true;
            m_prevNesMenuPressed[player] = m_nesMenuPressed[player];
        }
        return triggered;
    }

    bool GameInputManager::isNesPlayerFunctionPressed(EmuFunctionKey emuKey) const
    {
        const bool* states = nullptr;
        if (emuKey == EmuFunctionKey::EMU_FAST_FORWARD)
            states = m_nesFastForwardPressed;
        else if (emuKey == EmuFunctionKey::EMU_REWIND)
            states = m_nesRewindPressed;
        else
            return false;

        for (int player = 0; player < GAME_INPUT_MAX_PLAYERS; ++player)
            if (states[player])
                return true;
        return false;
    }

    bool GameInputManager::consumeNesFunctionPress(EmuFunctionKey emuKey, TriggerType triggerType)
    {
        bool* current = nullptr;
        bool* previous = nullptr;
        if (emuKey == EmuFunctionKey::EMU_FAST_FORWARD)
        {
            current = m_nesFastForwardPressed;
            previous = m_prevNesFastForwardPressed;
        }
        else if (emuKey == EmuFunctionKey::EMU_REWIND)
        {
            current = m_nesRewindPressed;
            previous = m_prevNesRewindPressed;
        }
        else
        {
            return false;
        }

        bool now = false;
        bool before = false;
        for (int player = 0; player < GAME_INPUT_MAX_PLAYERS; ++player)
        {
            now = now || current[player];
            before = before || previous[player];
        }

        bool triggered = false;
        switch (triggerType)
        {
            case TriggerType::PRESS:
                triggered = now && !before;
                break;
            case TriggerType::RELEASE:
                triggered = !now && before;
                break;
            case TriggerType::HOLD:
                triggered = now;
                break;
            case TriggerType::LONG_PRESS:
                triggered = now && !before;
                break;
        }

        if (triggerType == TriggerType::PRESS || triggerType == TriggerType::LONG_PRESS)
        {
            for (int player = 0; player < GAME_INPUT_MAX_PLAYERS; ++player)
                previous[player] = current[player];
        }
        return triggered;
    }

    void GameInputManager::updatePreviousNesFunctionStates()
    {
        for (int player = 0; player < GAME_INPUT_MAX_PLAYERS; ++player)
        {
            m_prevNesFastForwardPressed[player] = m_nesFastForwardPressed[player];
            m_prevNesRewindPressed[player] = m_nesRewindPressed[player];
        }
    }

    void GameInputManager::appendActiveInputsFromGamepadState(const GamepadState& state)
    {
        if (state.leftTrigger >= 0xFF)
            activeInputs.push_back(STATE_PAD_LT);
        if (state.rightTrigger >= 0xFF)
            activeInputs.push_back(STATE_PAD_RT);

        processStick(
            static_cast<float>(state.leftStickX) / 0x7FFF,
            -static_cast<float>(state.leftStickY) / 0x7FFF,
            STATE_PAD_LEFT_STICK_X,
            STATE_PAD_LEFT_STICK_Y,
            STATE_PAD_LEFT_STICK_LEFT,
            STATE_PAD_LEFT_STICK_RIGHT,
            STATE_PAD_LEFT_STICK_UP,
            STATE_PAD_LEFT_STICK_DOWN);

        processStick(
            static_cast<float>(state.rightStickX) / 0x7FFF,
            -static_cast<float>(state.rightStickY) / 0x7FFF,
            STATE_PAD_RIGHT_STICK_X,
            STATE_PAD_RIGHT_STICK_Y,
            STATE_PAD_RIGHT_STICK_LEFT,
            STATE_PAD_RIGHT_STICK_RIGHT,
            STATE_PAD_RIGHT_STICK_UP,
            STATE_PAD_RIGHT_STICK_DOWN);

        if (state.buttonFlags & UP_FLAG)     activeInputs.push_back(brls::BUTTON_UP);
        if (state.buttonFlags & DOWN_FLAG)   activeInputs.push_back(brls::BUTTON_DOWN);
        if (state.buttonFlags & LEFT_FLAG)   activeInputs.push_back(brls::BUTTON_LEFT);
        if (state.buttonFlags & RIGHT_FLAG)  activeInputs.push_back(brls::BUTTON_RIGHT);
        if (state.buttonFlags & A_FLAG)      activeInputs.push_back(brls::BUTTON_A);
        if (state.buttonFlags & B_FLAG)      activeInputs.push_back(brls::BUTTON_B);
        if (state.buttonFlags & X_FLAG)      activeInputs.push_back(brls::BUTTON_X);
        if (state.buttonFlags & Y_FLAG)      activeInputs.push_back(brls::BUTTON_Y);
        if (state.buttonFlags & BACK_FLAG)   activeInputs.push_back(brls::BUTTON_BACK);
        if (state.buttonFlags & PLAY_FLAG)   activeInputs.push_back(brls::BUTTON_START);
        if (state.buttonFlags & LB_FLAG)     activeInputs.push_back(brls::BUTTON_LB);
        if (state.buttonFlags & RB_FLAG)     activeInputs.push_back(brls::BUTTON_RB);
        if (state.buttonFlags & LS_CLK_FLAG) activeInputs.push_back(brls::BUTTON_LSB);
        if (state.buttonFlags & RS_CLK_FLAG) activeInputs.push_back(brls::BUTTON_RSB);
    }

    void GameInputManager::appendKeyboardHotkeyInputs()
    {
#ifndef __SWITCH__
        auto* im = brls::Application::getPlatform()->getInputManager();
        if (!im)
            return;
        for (const auto& hk : hotkeyBindings)
            for (const auto& combo : hk.buttons)
                for (int key : combo)
                    if (key >= 32 && im->getKeyboardKeyState(
                            static_cast<brls::BrlsKeyboardScancode>(key)))
                        activeInputs.push_back(key);
#endif
    }

    void GameInputManager::rebuildActiveInputsForHotkeys(int pollCount)
    {
        activeInputs.clear();
        appendKeyboardHotkeyInputs();

        if (!isNesDualPlayerMode())
        {
            for (int i = 0; i < pollCount && i < GAMEPADS_MAX; ++i)
                appendActiveInputsFromGamepadState(lastGamepadStates[i]);
            return;
        }

        for (int player = 0; player < GAME_INPUT_MAX_PLAYERS; ++player)
        {
            m_nesMenuPressed[player] = false;
            m_nesFastForwardPressed[player] = false;
            m_nesRewindPressed[player] = false;
            const std::string playerPrefix = player == 0 ? "nes.p1." : "nes.p2.";
            const int defaultController = player == 0 ? 0 : 1;
            const int controller = GET_SETTING_KEY_INT((playerPrefix + "controller").c_str(), defaultController);
            if (controller < 0 || controller >= pollCount || controller >= GAMEPADS_MAX)
                continue;

            auto readFunction = [&](const std::string& suffix, const std::string& fallback) -> bool {
                const std::string cfgKey = playerPrefix + "handle." + suffix;
                const std::string value = GET_SETTING_KEY_STR(cfgKey.c_str(), fallback);
                if (value.empty() || value == "none")
                    return false;
                auto combos = beiklive::tools::parseMultiCombo(value);
                for (const auto& combo : combos)
                    if (containsComboInMask(lastGamepadStates[controller], combo))
                        return true;
                return false;
            };

            m_nesMenuPressed[player] = readFunction("menu", player == 0 ? "PAD_LB" : "PAD_RB");
            m_nesFastForwardPressed[player] = readFunction("fastforward", "none");
            m_nesRewindPressed[player] = readFunction("rewind", "none");
        }
    }

    void GameInputManager::refreshPlayerInputStatesForPlatform(int platform)
    {
        for (auto& playerInput : m_playerInputs)
            playerInput.buttonMask = 0;

        const int controllersCount = std::min(getControllerCount(), GAMEPADS_MAX);
        if (controllersCount <= 0)
        {
            m_playerInputs[0].buttonMask = buildMaskFromGamepadState(lastGamepadStates[0]);
            return;
        }

        const bool isNes = platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNES);
        const bool nesTwoPlayer = isNes && m_nesDualPlayerEnabled;
        if (nesTwoPlayer)
        {
            for (int player = 0; player < GAME_INPUT_MAX_PLAYERS; ++player)
            {
                std::string playerPrefix = player == 0 ? "nes.p1." : "nes.p2.";
                const int defaultController = player == 0 ? 0 : 1;
                const int controller = GET_SETTING_KEY_INT((playerPrefix + "controller").c_str(), defaultController);
                if (controller < 0 || controller >= controllersCount)
                    continue;
                m_playerInputs[player].buttonMask =
                    buildMaskFromConfiguredMapping(lastGamepadStates[controller], playerPrefix);
            }
            return;
        }

        const std::string mappingPrefix = beiklive::input_mapping::platformPrefix(platform);
        uint32_t mergedMask = 0;
        for (int i = 0; i < controllersCount; ++i)
        {
            mergedMask |= buildMaskFromConfiguredMapping(lastGamepadStates[i], mappingPrefix);
        }
        m_playerInputs[0].buttonMask = mergedMask;
    }

    uint32_t GameInputManager::buildMaskFromGamepadState(const GamepadState& pad) const
    {
        uint32_t mask = 0;
        if (pad.buttonFlags & A_FLAG)      mask |= (1u << RETRO_DEVICE_ID_JOYPAD_A);
        if (pad.buttonFlags & B_FLAG)      mask |= (1u << RETRO_DEVICE_ID_JOYPAD_B);
        if (pad.buttonFlags & X_FLAG)      mask |= (1u << RETRO_DEVICE_ID_JOYPAD_X);
        if (pad.buttonFlags & Y_FLAG)      mask |= (1u << RETRO_DEVICE_ID_JOYPAD_Y);
        if (pad.buttonFlags & UP_FLAG)     mask |= (1u << RETRO_DEVICE_ID_JOYPAD_UP);
        if (pad.buttonFlags & DOWN_FLAG)   mask |= (1u << RETRO_DEVICE_ID_JOYPAD_DOWN);
        if (pad.buttonFlags & LEFT_FLAG)   mask |= (1u << RETRO_DEVICE_ID_JOYPAD_LEFT);
        if (pad.buttonFlags & RIGHT_FLAG)  mask |= (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT);
        if (pad.buttonFlags & LB_FLAG)     mask |= (1u << RETRO_DEVICE_ID_JOYPAD_L);
        if (pad.buttonFlags & RB_FLAG)     mask |= (1u << RETRO_DEVICE_ID_JOYPAD_R);
        if (pad.buttonFlags & BACK_FLAG)   mask |= (1u << RETRO_DEVICE_ID_JOYPAD_SELECT);
        if (pad.buttonFlags & PLAY_FLAG)   mask |= (1u << RETRO_DEVICE_ID_JOYPAD_START);
        if (pad.leftTrigger > 0)           mask |= (1u << RETRO_DEVICE_ID_JOYPAD_L2);
        if (pad.rightTrigger > 0)          mask |= (1u << RETRO_DEVICE_ID_JOYPAD_R2);
        if (pad.buttonFlags & LS_CLK_FLAG) mask |= (1u << RETRO_DEVICE_ID_JOYPAD_L3);
        if (pad.buttonFlags & RS_CLK_FLAG) mask |= (1u << RETRO_DEVICE_ID_JOYPAD_R3);
        return mask;
    }

    bool GameInputManager::containsComboInMask(const GamepadState& pad, const std::vector<int>& combo) const
    {
        for (int key : combo)
        {
            bool pressed = false;
            switch (key)
            {
                case brls::BUTTON_A: pressed = (pad.buttonFlags & A_FLAG) != 0; break;
                case brls::BUTTON_B: pressed = (pad.buttonFlags & B_FLAG) != 0; break;
                case brls::BUTTON_X: pressed = (pad.buttonFlags & X_FLAG) != 0; break;
                case brls::BUTTON_Y: pressed = (pad.buttonFlags & Y_FLAG) != 0; break;
                case brls::BUTTON_UP: pressed = (pad.buttonFlags & UP_FLAG) != 0; break;
                case brls::BUTTON_DOWN: pressed = (pad.buttonFlags & DOWN_FLAG) != 0; break;
                case brls::BUTTON_LEFT: pressed = (pad.buttonFlags & LEFT_FLAG) != 0; break;
                case brls::BUTTON_RIGHT: pressed = (pad.buttonFlags & RIGHT_FLAG) != 0; break;
                case brls::BUTTON_LB: pressed = (pad.buttonFlags & LB_FLAG) != 0; break;
                case brls::BUTTON_RB: pressed = (pad.buttonFlags & RB_FLAG) != 0; break;
                case brls::BUTTON_BACK: pressed = (pad.buttonFlags & BACK_FLAG) != 0; break;
                case brls::BUTTON_START: pressed = (pad.buttonFlags & PLAY_FLAG) != 0; break;
                case brls::BUTTON_LSB: pressed = (pad.buttonFlags & LS_CLK_FLAG) != 0; break;
                case brls::BUTTON_RSB: pressed = (pad.buttonFlags & RS_CLK_FLAG) != 0; break;
                case STATE_PAD_LT: pressed = pad.leftTrigger > 0; break;
                case STATE_PAD_RT: pressed = pad.rightTrigger > 0; break;
                case STATE_PAD_LEFT_STICK_LEFT: pressed = pad.leftStickX < -0x4000; break;
                case STATE_PAD_LEFT_STICK_RIGHT: pressed = pad.leftStickX > 0x4000; break;
                case STATE_PAD_LEFT_STICK_UP: pressed = pad.leftStickY > 0x4000; break;
                case STATE_PAD_LEFT_STICK_DOWN: pressed = pad.leftStickY < -0x4000; break;
                case STATE_PAD_RIGHT_STICK_LEFT: pressed = pad.rightStickX < -0x4000; break;
                case STATE_PAD_RIGHT_STICK_RIGHT: pressed = pad.rightStickX > 0x4000; break;
                case STATE_PAD_RIGHT_STICK_UP: pressed = pad.rightStickY > 0x4000; break;
                case STATE_PAD_RIGHT_STICK_DOWN: pressed = pad.rightStickY < -0x4000; break;
                default: break;
            }
            if (!pressed)
                return false;
        }
        return true;
    }

    uint32_t GameInputManager::buildMaskFromConfiguredMapping(const GamepadState& pad, const std::string& prefix) const
    {
        struct NesMapInfo
        {
            const char* suffix;
            const char* fallback;
            unsigned retroId;
        };
        static const NesMapInfo maps[] = {
            {"a", "PAD_A", RETRO_DEVICE_ID_JOYPAD_A},
            {"b", "PAD_B", RETRO_DEVICE_ID_JOYPAD_B},
            {"x", "PAD_X", RETRO_DEVICE_ID_JOYPAD_X},
            {"y", "PAD_Y", RETRO_DEVICE_ID_JOYPAD_Y},
            {"up", "PAD_UP", RETRO_DEVICE_ID_JOYPAD_UP},
            {"down", "PAD_DOWN", RETRO_DEVICE_ID_JOYPAD_DOWN},
            {"left", "PAD_LEFT", RETRO_DEVICE_ID_JOYPAD_LEFT},
            {"right", "PAD_RIGHT", RETRO_DEVICE_ID_JOYPAD_RIGHT},
            {"l", "PAD_LB", RETRO_DEVICE_ID_JOYPAD_L},
            {"r", "PAD_RB", RETRO_DEVICE_ID_JOYPAD_R},
            {"l2", "PAD_LT", RETRO_DEVICE_ID_JOYPAD_L2},
            {"r2", "PAD_RT", RETRO_DEVICE_ID_JOYPAD_R2},
            {"l3", "PAD_LSB", RETRO_DEVICE_ID_JOYPAD_L3},
            {"r3", "PAD_RSB", RETRO_DEVICE_ID_JOYPAD_R3},
            {"start", "PAD_START", RETRO_DEVICE_ID_JOYPAD_START},
            {"select", "PAD_BACK", RETRO_DEVICE_ID_JOYPAD_SELECT},
            {"lstick_up", "PAD_LEFTSTICKUP", RETRO_DEVICE_ID_JOYPAD_UP},
            {"lstick_down", "PAD_LEFTSTICKDOWN", RETRO_DEVICE_ID_JOYPAD_DOWN},
            {"lstick_left", "PAD_LEFTSTICKLEFT", RETRO_DEVICE_ID_JOYPAD_LEFT},
            {"lstick_right", "PAD_LEFTSTICKRIGHT", RETRO_DEVICE_ID_JOYPAD_RIGHT},
            {"rstick_up", "PAD_RIGHTSTICKUP", RETRO_DEVICE_ID_JOYPAD_UP},
            {"rstick_down", "PAD_RIGHTSTICKDOWN", RETRO_DEVICE_ID_JOYPAD_DOWN},
            {"rstick_left", "PAD_RIGHTSTICKLEFT", RETRO_DEVICE_ID_JOYPAD_LEFT},
            {"rstick_right", "PAD_RIGHTSTICKRIGHT", RETRO_DEVICE_ID_JOYPAD_RIGHT},
        };

        uint32_t mask = 0;
        const bool playerSpecificNes = prefix == "nes.p1." || prefix == "nes.p2.";
        const unsigned platformMask = playerSpecificNes
            ? beiklive::input_mapping::kPlatformNes
            : beiklive::input_mapping::platformMaskForPrefix(prefix);
        for (const auto& entry : maps)
        {
            const bool stickMapping =
                std::strncmp(entry.suffix, "lstick_", 7) == 0 ||
                std::strncmp(entry.suffix, "rstick_", 7) == 0;
            bool supported = false;
            if (playerSpecificNes)
            {
                supported =
                    std::string(entry.suffix) == "a" ||
                    std::string(entry.suffix) == "b" ||
                    std::string(entry.suffix) == "up" ||
                    std::string(entry.suffix) == "down" ||
                    std::string(entry.suffix) == "left" ||
                    std::string(entry.suffix) == "right" ||
                    std::string(entry.suffix) == "start" ||
                    std::string(entry.suffix) == "select";
            }
            if (!supported && stickMapping)
                supported = GET_SETTING_KEY_INT("input.joystick.enabled", 1) != 0;
            if (!supported)
            {
                for (const auto& def : beiklive::input_mapping::kGameButtonDefaults)
                {
                    if (std::string(entry.suffix) == def.suffix)
                    {
                        supported = (def.platformMask & platformMask) != 0;
                        break;
                    }
                }
            }
            if (!supported)
                continue;
            const std::string key = prefix + "handle." + entry.suffix;
            const std::string value = GET_SETTING_KEY_STR(
                key.c_str(),
                beiklive::input_mapping::defaultHandleValueForPrefix(
                    prefix, entry.suffix, entry.fallback));
            if (value.empty() || value == "none")
                continue;
            auto combos = beiklive::tools::parseMultiCombo(value);
            for (const auto& combo : combos)
            {
                if (containsComboInMask(pad, combo))
                {
                    mask |= (1u << entry.retroId);
                    break;
                }
            }
        }
        return mask;
    }

} // namespace beiklive
