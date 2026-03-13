# Bug Fix Report – Session 47

## 问题列表与解决方案

---

### 问题 1：XMB 背景设置为关闭并保存后，并没有关闭

**根本原因：**  
`InsertBackground()` 在创建 `ProImage` 时始终将 shader 动画设为 `PSP_XMB_RIPPLE`，并调用 `ApplyXmbColor()`，从未根据 `UI.showXmbBg` 配置项来决定是否显示或隐藏背景组件。`addSaveButton()` 里只调用了 `ApplyXmbColorToAll()`，该函数也只会设置颜色，不处理可见性。

**修复方案：**  
- 新增 `ApplyXmbBg(ProImage*)` 函数，同时处理：
  - 读取 `UI.showXmbBg` → 控制 shader 动画开关
  - 读取 `UI.showBgImage` + `UI.bgImagePath` → 设置背景图片  
  - 两者都关闭时，将组件设为 `GONE`（完全隐藏）
- `ApplyXmbColorToAll()` 改为调用 `ApplyXmbBg()`
- `InsertBackground()` 改为调用 `ApplyXmbBg()` 替代旧的 `ApplyXmbColor()`

---

### 问题 2：XMB 背景颜色在设置后立刻就切换了，应在点击保存后才生效

**根本原因：**  
`buildUITab()` 中颜色选择器 (`SelectorCell`) 的回调在每次选项变更时就直接调用了 `beiklive::ApplyXmbColorToAll()`，导致颜色即时生效。

**修复方案：**  
移除颜色选择器回调中的 `ApplyXmbColorToAll()` 调用。颜色仅写入到 `SettingManager`（内存），等用户点击"保存"按钮时，`addSaveButton()` 的 `Save()` + `ApplyXmbColorToAll()` 才会真正将颜色应用到显示。

---

### 问题 3：背景图片设置打开并选择了图片后，并没有图片显示出来

**根本原因：**  
配置 `UI.bgImagePath` 被写入，但没有任何地方在保存后将图片路径实际应用到已存在的 `ProImage` 实例上。原 `InsertBackground()` 只创建空白 `ProImage`，不关联背景图。

**修复方案：**  
在 `ApplyXmbBg()` 中，当 `UI.showBgImage == true` 且 `UI.bgImagePath` 非空时，调用 `img->setImageFromFile(bgPath)` 将图片加载到 `ProImage`。点击保存后 `ApplyXmbColorToAll()` → `ApplyXmbBg()` 触发图片加载。

---

### 问题 4：模拟器按键音效设置关闭但是仍然有声音

**根本原因：**  
`BKAudioPlayer::play()` 未检查 `audio.buttonSfx` 配置项，始终播放声音。

**修复方案：**  
在 `BKAudioPlayer::play()` 最开始处读取 `SettingManager` 中的 `audio.buttonSfx`（即 `KEY_AUDIO_BUTTON_SFX`），若值为 false/0/no，则直接返回 `true`（静默丢弃，不报错）。

---

### 问题 5：画面设置中要添加缩放倍数设置，在显示模式设置为整数倍的时候生效

**根本原因：**  
`buildDisplayTab()` 中没有 `display.integer_scale_mult` 的 UI 设置项，用户无法从界面修改整数倍缩放倍率。

**修复方案：**  
在"显示模式"选择器下方新增"整数倍缩放倍率（整数倍模式下生效）"选择器，提供：
- `自动 (Auto)`：值 0（自动最大整数倍）
- `1x` ~ `6x`：固定倍率 1~6
  
选中后写入 `display.integer_scale_mult`，保存后生效。

---

### 问题 6：按键映射界面，开启映射弹窗后映射A键时，映射完毕也无法关闭

**根本原因：**  
`KeyCaptureView` 使用 `draw()` 轮询手柄按键状态，但没有为 `BUTTON_A` 等按钮注册 borealis action handler。`brls::Dialog` 打开时，borealis 的焦点在 Dialog 内，按下 A 键时：
1. Dialog 的"确认关闭"action 被触发，导致 Dialog 关闭，但
2. 同一帧 `draw()` 的按键轮询也检测到了 A 键按下，触发了 `finish("A")`，  
导致原来那个 `DialogCell` 的 A action 又被重新触发，重新打开一个新的对话框。

**修复方案：**  
对手柄捕获模式 (`!isKeyboard`)，在 `KeyCaptureView` 构造时为所有 `k_capPadKeys` 中的按钮注册 borealis action handlers，这样：
1. 按键被 `KeyCaptureView` 消费（返回 `true`），不再向上传播到父 `DetailCell`
2. 映射结果通过 borealis action 系统传递（每次按键触发一次，不是每帧轮询）
3. A 键的映射与 borealis "确认" 逻辑完全解耦

---

### 问题 7：游戏界面仍然无法获取按键，debugview 视图对按键操作也没有显示相应的 log

**根本原因：**  
`pollInput()` 在独立的游戏线程中被调用，但直接调用了 GLFW API（通过 `updateUnifiedControllerState()` 和 `getKeyboardKeyState()`）。GLFW 明确规定这些函数必须从主线程调用。从非主线程调用这些函数属于未定义行为（UB），在某些系统/驱动上会返回始终为 false/0 的状态，导致所有按键输入永远检测不到。

另外，`brls::Application::isInputBlocks()` 为 true 时，borealis 主循环跳过 `processInput()`，所以 `getControllerState()` 返回的是上一帧的陈旧状态。

**修复方案：**  
引入主线程输入快照机制：
1. 在 `GameView` 中新增 `InputSnapshot` 结构体（包含 `ControllerState` + 键盘按键状态 map）和对应的互斥锁 `m_inputSnapMutex`
2. 新增 `refreshInputSnapshot()` 方法，在主线程的 `draw()` 中每帧调用，使用 GLFW/borealis API 采集所有需要的输入状态并写入 `m_inputSnap`
3. `pollInput()`（游戏线程）改为在互斥锁保护下读取 `m_inputSnap` 快照，完全不再直接调用 GLFW 函数
4. 在 `pollInput()` 的 DEBUG 模式下新增按键事件日志，方便 debugview 追踪

---

## 修改文件列表

| 文件 | 修改内容 |
|------|---------|
| `include/common.hpp` | 新增 `ApplyXmbBg()` 函数声明 |
| `src/common.cpp` | 实现 `ApplyXmbBg()`；更新 `InsertBackground()` 和 `ApplyXmbColorToAll()` |
| `include/Game/game_view.hpp` | 新增 `InputSnapshot` 结构体、`m_inputSnap`、`m_inputSnapMutex`；新增 `refreshInputSnapshot()` |
| `src/Game/game_view.cpp` | 实现 `refreshInputSnapshot()`；重构 `pollInput()` 使用快照；`draw()` 每帧调用 `refreshInputSnapshot()` |
| `src/Audio/BKAudioPlayer.cpp` | `play()` 中检查 `KEY_AUDIO_BUTTON_SFX` 配置 |
| `src/UI/Pages/SettingPage.cpp` | 移除颜色选择器即时应用；新增整数倍缩放倍率设置；`KeyCaptureView` 手柄模式注册 action handlers |
