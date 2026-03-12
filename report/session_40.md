# 工作报告 Work Report – 游戏模式下屏蔽 UI 层键盘/手柄控制

## 会话 Session #40

---

## 任务概述

**问题：** 在 Windows 平台使用键盘操作时，进入 GameView 后方向键焦点仍在 UI 层，导致无法控制游戏内的角色方向。

**目标：** 进入 GameView 后屏蔽所有 UI 层输入控制；在使用功能键打开 UI 相关菜单或退出游戏时，才恢复 UI 层的控制。

---

## 根本原因分析

Borealis 桌面平台（GLFW/SDL）会将键盘方向键映射为虚拟手柄按键（`BUTTON_UP/DOWN/LEFT/RIGHT` 和 `BUTTON_NAV_UP/DOWN/LEFT/RIGHT`），并在每帧的 `processInput()` 循环中通过 `onControllerButtonPressed()` 触发 UI 导航（焦点移动）。

虽然 GameView 已通过 `beiklive::swallow()` 注册了这些按键的"吞噬动作"，但用户观察到焦点仍在 UI 层，说明 UI 导航并没有被完全拦截。

为彻底解决此问题，采用 `brls::Application::blockInputs()` API 显式屏蔽 UI 层的所有输入处理。

---

## 实现方案

### 核心机制

Borealis 提供基于令牌计数器（token counter）的输入屏蔽接口：

- `brls::Application::blockInputs(bool muteSounds)` — 令牌 +1，当 `blockInputsTokens > 0` 时，`processInput()` 被跳过，UI 导航/动作均不触发
- `brls::Application::unblockInputs()` — 令牌 -1，归零时恢复正常 UI 输入处理
- `brls::Application::isInputBlocks()` — 查询当前是否处于屏蔽状态

### 令牌生命周期（以打开菜单再返回为例）

```
pushActivity(GameView)
  → blockInputs()         token=1
  → giveFocus(GameView)
     → onFocusGained()
        → blockInputs()   token=2
  → unblockInputs()       token=1  ← 动画结束后
【游戏运行中，UI 完全屏蔽，token=1】

hotkey F1 → pushActivity(MenuActivity)
  → blockInputs()         token=2
  → giveFocus(Menu)
     → GameView::onFocusLost()
        → unblockInputs() token=1
  → unblockInputs()       token=0  ← 菜单可操作 ✓

popActivity(MenuActivity)
  → blockInputs()         token=1
  → giveFocus(GameView)
     → GameView::onFocusGained()
        → blockInputs()   token=2
  → unblockInputs()       token=1  ← 动画结束后
【游戏恢复，UI 再次屏蔽，token=1 ✓】
```

### 手柄输入保障

当 `blockInputs()` 生效时，borealis 的 `processInput()` 被跳过，导致全局 `controllerState` 不再更新（因为 `inputManager->updateUnifiedControllerState()` 不会被调用）。若游戏线程仍从 `getControllerState()` 读取，会得到过时的状态，手柄按键会出现"卡住"的问题。

解决方案：在 `pollInput()` 中检测 UI 屏蔽状态，如果处于屏蔽状态则**直接调用 `im->updateUnifiedControllerState()`** 获取最新手柄状态，绕过全局缓存。

```cpp
brls::ControllerState state = {};
if (im && brls::Application::isInputBlocks()) {
    im->updateUnifiedControllerState(&state);  // 直接读取，绕过缓存
} else {
    state = brls::Application::getControllerState();  // 正常路径
}
```

> 注：键盘模式（`m_useKeyboard = true`）直接通过 `im->getKeyboardKeyState()` 读取原始键盘状态，不依赖 `controllerState`，因此键盘输入不受此问题影响。

---

## 修改文件

### `include/Game/game_view.hpp`

新增成员变量：

```cpp
/// true when we have an outstanding blockInputs() token (desktop only).
bool m_uiBlocked = false;
```

### `src/Game/game_view.cpp`

#### 1. `onFocusGained()`（原为空实现，现新增逻辑）

```cpp
void GameView::onFocusGained()
{
    Box::onFocusGained();
#ifndef __SWITCH__
    if (!m_uiBlocked) {
        brls::Application::blockInputs(true); // true = 屏蔽 UI 错误音效
        m_uiBlocked = true;
    }
#endif
}
```

#### 2. `onFocusLost()`（原为空实现，现新增逻辑）

```cpp
void GameView::onFocusLost()
{
    Box::onFocusLost();
#ifndef __SWITCH__
    if (m_uiBlocked) {
        brls::Application::unblockInputs();
        m_uiBlocked = false;
    }
#endif
}
```

#### 3. `cleanup()`（新增安全释放逻辑）

在 `cleanup()` 最开始处新增安全守卫，防止析构顺序异常时令牌泄漏：

```cpp
#ifndef __SWITCH__
if (m_uiBlocked) {
    brls::Application::unblockInputs();
    m_uiBlocked = false;
}
#endif
```

#### 4. `pollInput()`（新增直接读取手柄状态逻辑）

将原来的 `const brls::ControllerState& state = brls::Application::getControllerState();` 替换为：

```cpp
brls::ControllerState state = {};
#ifndef __SWITCH__
{
    auto* platform = brls::Application::getPlatform();
    auto* im_state = platform ? platform->getInputManager() : nullptr;
    if (im_state && brls::Application::isInputBlocks()) {
        im_state->updateUnifiedControllerState(&state);
    } else {
        state = brls::Application::getControllerState();
    }
}
#else
state = brls::Application::getControllerState();
#endif
```

---

## 平台影响范围

| 平台 | 是否受影响 | 说明 |
|------|-----------|------|
| Windows (GLFW/SDL) | ✅ 修复 | 方向键不再触发 UI 导航 |
| Linux (GLFW) | ✅ 修复 | 同上 |
| macOS (GLFW) | ✅ 修复 | 同上 |
| Nintendo Switch | ❌ 不影响 | `#ifndef __SWITCH__` 保护，行为不变 |

---

## 变更文件汇总

| 文件 | 修改内容 |
|------|---------|
| `include/Game/game_view.hpp` | 新增 `m_uiBlocked` 成员变量 |
| `src/Game/game_view.cpp` | `onFocusGained()` / `onFocusLost()` / `cleanup()` / `pollInput()` |
| `report/session_40.md` | 本报告 |
