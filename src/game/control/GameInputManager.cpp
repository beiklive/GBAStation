#include "GameInputManager.hpp"

namespace beiklive
{
float fsqrt_(float f) {
    int i = *(int *)&f;
    i = (i >> 1) + 0x1fbb67ae;
    float f1 = *(float *)&i;
    return 0.5F * (f1 + f / f1);

}
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
    }

    void GameInputManager::handleInput(bool ignoreTouch)
    {
        inputDropped = false;
        // static brls::ControllerState rawController;
        // static brls::ControllerState preController;
        // static brls::ControllerState controller;
        // // static brls::RawMouseState mouse;
        // preController = rawController;
        // 获取当前输入状态
        // brls::Application::getPlatform()
        //     ->getInputManager()
        //     ->updateUnifiedControllerState(&rawController);
        // // 获取鼠标状态， 以后如果需要处理鼠标输入可以使用这个接口（nds 3ds游戏）
        // brls::Application::getPlatform()->getInputManager()->updateMouseStates(
        //         &mouse);

        // 处理输入状态变化，转换为游戏逻辑需要的格式
        handleControllerInput();
    }
    void GameInputManager::handleControllerInput()
    {
        static int lastControllerCount = 0;

        // 获取所有控制器数量
        auto controllersCount = brls::Application::getPlatform()
                                    ->getInputManager()
                                    ->getControllersConnectedCount();

        // 遍历控制器获取状态并处理输入，最多GAMEPADS_MAX个控制器
        for (int i = 0; i < controllersCount; i++)
        {
            // 获取指定控制器的状态
            GamepadState gamepadState = getControllerState(i);

            // 状态有变化
            if (!gamepadState.is_equal(lastGamepadStates[i]))
            {
                lastGamepadStates[i] = gamepadState;

                if (lastControllerCount != controllersCount)
                {
                    lastControllerCount = controllersCount;

                    for (int i = 0; i < controllersCount; i++)
                    {
                        brls::Logger::debug("StreamingView: send features message for controller #{}", i);
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
        brls::Application::getPlatform()->getInputManager()->updateControllerState(
            &rawController, controllerNum);

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

        if (leftStickDeadzone > 0) {
            float magnitude = fsqrt_(std::powf(leftXAxis, 2) + std::powf(leftYAxis, 2));
            if (magnitude < leftStickDeadzone) {
                leftXAxis = 0;
                leftYAxis = 0;
            }
        }

        if (rightStickDeadzone > 0) {
            float magnitude = fsqrt_(std::powf(rightXAxis, 2) + std::powf(rightYAxis, 2));
            if (magnitude < rightStickDeadzone) {
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







        return GamepadState();
    }

    bool GameInputManager::compareControllerState(const brls::ControllerState &a, const brls::ControllerState &b)
    {
        for (int i = 0; i < brls::ControllerButton::_BUTTON_MAX; i++)
        {
            if (a.buttons[i] != b.buttons[i])
                return false;
        }
        for (int i = 0; i < brls::ControllerAxis::_AXES_MAX; i++)
        {
            if (a.axes[i] != b.axes[i])
                return false;
        }
        return true;
    }

    const char *getPadEnumString(int value)
    {
        switch (value)
        {
        case 0:
            return "BUTTON_LT";
        case 1:
            return "BUTTON_LB";
        case 2:
            return "BUTTON_LSB";
        case 3:
            return "BUTTON_UP";
        case 4:
            return "BUTTON_RIGHT";
        case 5:
            return "BUTTON_DOWN";
        case 6:
            return "BUTTON_LEFT";
        case 7:
            return "BUTTON_BACK";
        case 8:
            return "BUTTON_GUIDE";
        case 9:
            return "BUTTON_START";
        case 10:
            return "BUTTON_RSB";
        case 11:
            return "BUTTON_Y";
        case 12:
            return "BUTTON_B";
        case 13:
            return "BUTTON_A";
        case 14:
            return "BUTTON_X";
        case 15:
            return "BUTTON_RB";
        case 16:
            return "BUTTON_RT";
        case 17:
            return "BUTTON_NAV_UP";
        case 18:
            return "BUTTON_NAV_RIGHT";
        case 19:
            return "BUTTON_NAV_DOWN";
        case 20:
            return "BUTTON_NAV_LEFT";
        }
    }

    const char *getAxisEnumString(int value)
    {
        switch (value)
        {
        case 0:
            return "LEFT_X";
        case 1:
            return "LEFT_Y";
        case 2:
            return "RIGHT_X";
        case 3:
            return "RIGHT_Y";
        case 4:
            return "LEFT_Z";
        case 5:
            return "RIGHT_Z";
        }
    }

} // namespace beiklive
