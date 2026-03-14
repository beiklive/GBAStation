# Session 65 – GameView 控制系统重写报告

## 需求分析

原始需求如下：

> 重写 gameview 控制系统：
> 1. gameview 禁用 borealis 的输入系统（提供一键控制开关的接口），使用底层
>    `Application::getPlatform()->getInputManager()` 来获取按键，并构造按下、抬起、
>    长按、短按和组合键系统，减少与 borealis 的交互冲突。
> 2. 控制系统不需要兼容键盘，只要维护手柄部分即可。

经过代码库分析，梳理出以下完整需求清单：

| # | 需求点 | 现状 | 本次处理 |
|---|--------|------|----------|
| 1 | 一键开关 borealis 输入系统的公开接口 | 无公开 API，仅隐式地在 `onFocusGained/Lost` 中操作 | 新增 `setGameInputEnabled(bool)` |
| 2 | 使用 `getPlatform()->getInputManager()` 获取手柄状态 | 已在 `refreshInputSnapshot` 中完成 | 保留并精简注释 |
| 3 | 按下（Press）事件 | 已有 | 保留 |
| 4 | 抬起（Release）事件 | 已有 | 保留 |
| 5 | 短按（ShortPress）事件 | **缺失** | 新增 |
| 6 | 长按（LongPress）事件 | 已有 | 保留 |
| 7 | 组合键（Combo）支持 | 已有（多按钮同时按下） | 保留 |
| 8 | 暂停/恢复整个控制器的开关 | **缺失** | 新增 `setEnabled(bool)` |
| 9 | 移除键盘相关的控制路径 | 大量键盘代码混入 `pollInput` 和 `refreshInputSnapshot` | 全部移除 |
| 10 | 快进/倒带的 hold 模式与 toggle 模式使用正确事件触发 | 使用 `Press` 做 toggle（语义不明确） | 改用 `ShortPress` 做 toggle，`Press/Release` 做 hold |

---

## 具体修改

### 1. `include/Control/GameInputController.hpp`

- `KeyEvent` 枚举新增 `ShortPress`：
  - `Press`      – 上升沿（按下）
  - `Release`    – 下降沿（抬起）
  - `ShortPress` – 在 `Release` 时、本次按压未产生 `LongPress` 时触发
  - `LongPress`  – 按住达到 500 ms 时触发一次

- 新增 `setEnabled(bool enabled)` 方法：
  - `false`：`update()` 变为空操作，并将所有 Action 的 `active/longPressFired` 重置，
    防止悬空状态导致下次恢复时产生虚假事件。
  - `true`：恢复正常处理，已注册的 Action 不会被清除。

- 新增 `isEnabled() const`。

### 2. `src/Control/GameInputController.cpp`

- `update()` 开头加 `if (!m_enabled) return;`。
- 下降沿处理改为：先判断是否需要发 `ShortPress`（`!longPressFired`），再发 `Release`：
  ```cpp
  action.active = false;
  if (!action.longPressFired)
      action.callback(KeyEvent::ShortPress);
  action.longPressFired = false;
  action.callback(KeyEvent::Release);
  ```
- 实现 `setEnabled(bool)` 及重置逻辑。

### 3. `include/Game/game_view.hpp`

- 新增公开方法 `void setGameInputEnabled(bool enabled)`。
- 删除已无用的键盘专属成员变量：
  - `m_useKeyboard`
  - `m_ffKbdHeld`, `m_ffKbdHoldPrev`, `m_ffKbdTogglePrev`
  - `m_rewKbdHeld`, `m_rewKbdHoldPrev`, `m_rewKbdTogglePrev`
  - `m_kbdQsSavePrev`, `m_kbdQlLoadPrev`
- `InputSnapshot` 结构去除 `kbdState`（`unordered_map<int,bool>`），仅保留 `ctrlState`。
- 移除 `#include <unordered_map>`。

### 4. `src/Game/game_view.cpp`

#### `setGameInputEnabled(bool enabled)` – 新方法
```cpp
void GameView::setGameInputEnabled(bool enabled)
{
#ifndef __SWITCH__
    if (enabled && !m_uiBlocked)
    {
        brls::Application::blockInputs(true);
        m_uiBlocked = true;
    }
    else if (!enabled && m_uiBlocked)
    {
        brls::Application::unblockInputs();
        m_uiBlocked = false;
    }
#endif
    m_inputCtrl.setEnabled(enabled);
}
```

#### `onFocusGained() / onFocusLost()`
从直接调用 `blockInputs/unblockInputs` 改为委托给新 API：
```cpp
void GameView::onFocusGained() { Box::onFocusGained(); setGameInputEnabled(true);  }
void GameView::onFocusLost()   { Box::onFocusLost();   setGameInputEnabled(false); }
```

#### `refreshInputSnapshot()`
- 移除全部键盘部分（`#ifndef __SWITCH__` 块：`kbdState`、修饰键、导航键、热键扫码读取）。
- 仅保留 `im->updateUnifiedControllerState(&snap.ctrlState)` 一行核心逻辑。

#### `pollInput()`
- 移除 `kbdState` Lambda。
- 移除键盘模式切换（`m_useKeyboard` 检测逻辑）。
- 移除键盘快进、倒带、退出、快存/读的内联处理。
- 游戏按键映射仅保留手柄路径，去除 `m_useKeyboard` 分支。
- 调试日志也相应简化。

#### `registerGamepadHotkeys()`
- **Hold 模式**：改为 `Press` 事件置为 held、`Release` 事件清除 held（语义更清晰）。
- **Toggle 模式 / 专用 Toggle 键 / 退出 / 快存 / 快读**：改用 `ShortPress` 触发（避免长按误触发）。
- FF/Rewind 相关的 `m_ffKbdHeld` 和 `m_rewKbdHeld` 已从表达式中移除。

---

## 事件触发时序说明

```
按下  ──────────── Press
       (< 500 ms)
抬起  ────────────── ShortPress → Release

按下  ──────────── Press
       (>= 500 ms) LongPress（触发一次）
抬起  ────────────── Release（无 ShortPress）
```

---

## 未涉及范围

- **键盘游戏按键映射**（`handle.*` / `keyboard.*` 配置项）：
  `InputMappingConfig` 中仍保留 `kbdScancode` 字段，但 `pollInput()` 不再读取它。
  如需完全移除，可在后续版本清理 `InputMappingConfig::GameButtonEntry.kbdScancode`。

- **热键键盘绑定**（`hotkey.*.kbd`）：
  `InputMappingConfig` 中仍可配置，但不再在运行时产生效果。

---

## 影响范围

| 文件 | 修改类型 |
|------|----------|
| `include/Control/GameInputController.hpp` | 增强（新增枚举值 + 方法） |
| `src/Control/GameInputController.cpp`     | 增强（实现 ShortPress + setEnabled） |
| `include/Game/game_view.hpp`              | 清理（删除键盘成员 + 新增 setGameInputEnabled） |
| `src/Game/game_view.cpp`                  | 重写 pollInput/refreshInputSnapshot，新增 setGameInputEnabled |
