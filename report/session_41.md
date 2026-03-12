# Session 41 – Arrow Keys Not Working in GameView (Root Cause Fix)

## Problem

方向键（↑↓←→）在 GameView 中按下后游戏没有响应。

## Root Cause Analysis

问题位于 `src/Game/game_view.cpp` 的 `GameView::pollInput()` 函数，具体是键盘模式（`m_useKeyboard == true`）下的游戏按键处理循环。

### 按键映射表结构

`InputMappingConfig::loadButtonMaps()`（`src/Control/InputMapping.cpp`）构建了一个 `m_gameButtonMap`，其中每个条目包含：

- `retroId`：libretro 虚手柄按钮 ID（如 `RETRO_DEVICE_ID_JOYPAD_UP`）
- `padButton`：Borealis 手柄按钮枚举值（如 `brls::BUTTON_UP`）
- `kbdScancode`：键盘扫描码（如 `brls::BRLS_KBD_KEY_UP = 265`，若无映射则为 `-1`）

在主循环之后，代码还为 D-pad 方向键添加了 **NAV 别名条目**（`BUTTON_NAV_UP/DOWN/LEFT/RIGHT`）：

```cpp
// Also emit entries for NAV buttons (dpad aliases, gamepad only).
m_gameButtonMap.push_back({ RETRO_DEVICE_ID_JOYPAD_UP,
    static_cast<int>(brls::BUTTON_NAV_UP),    -1 });  // kbdScancode = -1 !
m_gameButtonMap.push_back({ RETRO_DEVICE_ID_JOYPAD_DOWN,
    static_cast<int>(brls::BUTTON_NAV_DOWN),  -1 });
m_gameButtonMap.push_back({ RETRO_DEVICE_ID_JOYPAD_LEFT,
    static_cast<int>(brls::BUTTON_NAV_LEFT),  -1 });
m_gameButtonMap.push_back({ RETRO_DEVICE_ID_JOYPAD_RIGHT,
    static_cast<int>(brls::BUTTON_NAV_RIGHT), -1 });
```

### 错误执行流程（修复前）

以按下"↑"键为例，`pollInput()` 键盘模式循环依次执行：

| 顺序 | 条目 | retroId | kbdScancode | pressed | 结果 |
|------|------|---------|-------------|---------|------|
| 1 | 主条目 | `JOYPAD_UP` | `BRLS_KBD_KEY_UP`=265 | **true** | `m_buttons[JOYPAD_UP] = true` ✓ |
| 2 | NAV 别名 | `JOYPAD_UP` | **-1**（无键盘绑定） | **false** | `m_buttons[JOYPAD_UP] = false` ✗ |

NAV 别名条目的 `kbdScancode = -1`，导致 `pressed` 始终为 `false`，并用 `setButtonState()` 把刚才第 1 步设置的 `true` **覆盖为 `false`**。游戏核心永远读不到"↑键被按下"的状态。

### 为什么手柄模式不受影响

手柄模式中读取的是 `state.buttons[padButton]`。`updateUnifiedControllerState()` 会将 `BUTTON_UP` 的状态同步到 `BUTTON_NAV_UP`，因此两个条目都返回 `true`，不存在覆盖问题。

## 修复方案

在 `pollInput()` 键盘模式循环中，对 `kbdScancode < 0` 的条目直接 `continue` 跳过，不调用 `setButtonState()`：

```cpp
// src/Game/game_view.cpp – pollInput()
for (const auto& entry : btnMap) {
    if (entry.kbdScancode < 0) continue;   // ← 新增：无键盘绑定则跳过
    bool pressed = false;
#ifndef __SWITCH__
    if (im)
        pressed = im->getKeyboardKeyState(
            static_cast<brls::BrlsKeyboardScancode>(entry.kbdScancode));
#endif
    m_core.setButtonState(entry.retroId, pressed);
}
```

### 修复正确性验证

- 主条目（`kbdScancode >= 0`）：正常处理，按键按下/释放均正确上报。
- NAV 别名条目（`kbdScancode == -1`）：跳过，不再覆盖主条目已设置的状态。
- 无默认键盘绑定的按键（如 `JOYPAD_L3`/`JOYPAD_R3`，`kbdScancode == -1`）：跳过，状态保持初始化时的 `false`，行为正确。

## 修改文件

- `src/Game/game_view.cpp`：`pollInput()` 函数，键盘模式游戏按键循环，约第 782–792 行。
