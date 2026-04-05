```C++
/**
 * @brief 输入系统统一接口（核心抽象层）
 *
 * InputManager 是 Borealis 输入系统的核心抽象接口，
 * 用于屏蔽不同平台（Switch / PC / Android / Linux 等）的输入差异。
 *
 * 所有输入设备（手柄 / 键盘 / 触摸 / 鼠标 / 传感器）最终都通过该接口
 * 转换为统一的数据结构，供 UI 系统和应用逻辑使用。
 *
 * -------------------------------
 * 【设计目标】
 * -------------------------------
 * 1. 跨平台输入统一
 * 2. 支持多设备（多手柄 + 键盘 + 触摸）
 * 3. 每帧轮询（frame-based polling）
 * 4. 提供事件机制（Event）
 * 5. 支持模拟器 / 游戏场景扩展
 *
 * -------------------------------
 * 【调用时机】
 * -------------------------------
 * 每一帧主循环：
 *
 * runloopStart()
 * ├── updateControllerState()
 * ├── updateTouchStates()
 * ├── updateMouseStates()
 * └── UI / 游戏消费输入
 *
 * -------------------------------
 * 【使用模式】
 * -------------------------------
 * 平台层实现该接口，例如：
 *  - GLFWInputManager
 *  - SwitchInputManager
 *  - SDLInputManager
 *
 * 然后注册到 Borealis Application 中统一使用
 */
class InputManager
{
public:
    virtual ~InputManager() { }

    /**
     * @brief 获取当前已连接的手柄数量
     *
     * @return short 手柄数量（最大 GAMEPADS_MAX）
     *
     * @note
     * - 用于多玩家支持
     * - Switch 上通常为 Joy-Con / Pro Controller
     */
    virtual short getControllersConnectedCount() = 0;

    /**
     * @brief 更新“统一控制器状态”（聚合所有输入设备）
     *
     * @param state 输出参数，写入最终控制状态
     *
     * @note
     * - 将多个手柄 + 键盘 + 其他输入合并为一个虚拟控制器
     * - 常用于 UI 导航（方向键、确认键等）
     */
    virtual void updateUnifiedControllerState(ControllerState* state) = 0;

    /**
     * @brief 更新指定手柄的状态（逐个设备）
     *
     * @param state 输出状态
     * @param controller 手柄索引（0 ~ n）
     *
     * @note
     * - 用于游戏逻辑（多玩家）
     * - 每帧调用一次
     */
    virtual void updateControllerState(ControllerState* state, int controller) = 0;

    /**
     * @brief 获取键盘按键当前状态（是否按下）
     *
     * @param state 键码（BrlsKeyboardScancode）
     * @return true 按下
     * @return false 未按下
     */
    virtual bool getKeyboardKeyState(BrlsKeyboardScancode state) = 0;

    /**
     * @brief 更新触摸输入（原始数据）
     *
     * @param states 输出触摸点数组
     *
     * @note
     * - 仅提供“原始触摸数据”
     * - TouchPhase 由 computeTouchState 自动计算
     */
    virtual void updateTouchStates(std::vector<RawTouchState>* states) = 0;

    /**
     * @brief 更新鼠标输入（原始数据）
     *
     * @param state 鼠标状态
     *
     * @note
     * - position: 当前坐标
     * - offset: 移动增量
     * - scroll: 滚轮
     */
    virtual void updateMouseStates(RawMouseState* state) = 0;

    /**
     * @brief 发送手柄震动（Rumble）
     *
     * @param controller 手柄索引
     * @param lowFreqMotor 低频震动（一般是大马达）
     * @param highFreqMotor 高频震动（一般是小马达）
     *
     * @note
     * - 数值范围通常 0~65535
     * - 平台实现决定是否支持
     */
    virtual void sendRumble(unsigned short controller,
                            unsigned short lowFreqMotor,
                            unsigned short highFreqMotor) = 0;

    /**
     * @brief 每帧开始时调用（生命周期钩子）
     *
     * @note
     * - 用于清理上一帧状态
     * - 或重置临时缓存
     * - Borealis 内部调用
     */
    virtual void runloopStart() {}

    /**
     * @brief 绘制鼠标光标（可选）
     *
     * @param vg NanoVG 上下文
     *
     * @note
     * - 仅 PC 平台常用
     * - Switch/触摸设备一般不需要
     */
    virtual void drawCursor(NVGcontext* vg) {}

    /**
     * @brief 设置鼠标锁定（FPS/模拟器常用）
     *
     * @param lock 是否锁定
     *
     * @note
     * - true: 隐藏光标 + 相对移动
     * - false: 释放
     */
    virtual void setPointerLock(bool lock) {}

    /**
     * @brief 鼠标移动事件（偏移量）
     */
    inline Event<Point>* getMouseCusorOffsetChanged()
    {
        return &mouseCusorOffsetChanged;
    }

    /**
     * @brief 鼠标滚轮事件
     */
    inline Event<Point>* getMouseScrollOffsetChanged()
    {
        return &mouseScrollOffsetChanged;
    }

    /**
     * @brief 手柄传感器事件（陀螺仪 / 加速度）
     */
    inline Event<SensorEvent>* getControllerSensorStateChanged() {
        return &controllerSensorStateChanged;
    }

    /**
     * @brief 键盘按键变化事件（按下/释放）
     */
    inline Event<KeyState>* getKeyboardKeyStateChanged()
    {
        return &keyboardKeyStateChanged;
    }

    /**
     * @brief 计算触摸阶段（自动状态机）
     *
     * @param currentTouch 当前帧原始数据
     * @param lastFrameState 上一帧状态
     * @return TouchState
     *
     * @note 状态转换：
     * NONE -> START -> STAY -> END -> NONE
     */
    static TouchState computeTouchState(RawTouchState currentTouch,
                                        TouchState lastFrameState);

    /**
     * @brief 计算鼠标按键阶段（类似触摸）
     */
    static MouseState computeMouseState(RawMouseState currentTouch,
                                        MouseState lastFrameState);

    /**
     * @brief 控制器按键映射（平台适配）
     *
     * @param button 抽象按键
     * @return 映射后的按键
     *
     * @note
     * - 用于不同平台布局统一（如 Switch / Xbox）
     */
    static ControllerButton mapControllerState(ControllerButton button);

private:
    Event<Point> mouseCusorOffsetChanged;      ///< 鼠标移动事件
    Event<Point> mouseScrollOffsetChanged;     ///< 鼠标滚轮事件
    Event<KeyState> keyboardKeyStateChanged;   ///< 键盘事件
    Event<SensorEvent> controllerSensorStateChanged; ///< 传感器事件
};

```