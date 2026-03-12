# 工作报告 Work Report – 按键映射剥离 & 模拟器热键模块

## 会话 Session #39

---

## 任务概述

将 `GameView` 中的所有按键映射逻辑剥离到专用的 `Control/InputMapping` 模块中，并为模拟器系统功能键（快进、倒带、快速保存、截屏等）提供完整的配置接口。

---

## 一、新增文件

### `include/Control/InputMapping.hpp`

定义 `beiklive::InputMappingConfig` 类，包含：

#### 数据类型

| 类型 | 说明 |
|------|------|
| `Hotkey` 枚举 | 13 个模拟器系统功能键（见下表） |
| `KeyCombo` 结构 | 键盘按键 + Ctrl/Shift/Alt 修饰键（组合键支持） |
| `GameButtonEntry` 结构 | 一个游戏按钮：Retro Joypad ID + 手柄键 + 键盘键 |
| `HotkeyBinding` 结构 | 一个模拟器热键：`KeyCombo` + 手柄键 |

#### 模拟器系统功能键枚举（`Hotkey`）

| 枚举值 | 功能 | 默认键盘键 | 默认手柄键 |
|-------|------|----------|----------|
| `FastForwardToggle` | 快进（切换） | none | none |
| `FastForwardHold`   | 快进（保持） | TAB  | RT   |
| `RewindToggle`      | 倒带（切换） | none | none |
| `RewindHold`        | 倒带（保持） | GRAVE | LT  |
| `QuickSave`         | 快速保存    | F5   | none |
| `QuickLoad`         | 快速读取    | F8   | none |
| `OpenMenu`          | 打开菜单    | F1   | none |
| `Mute`              | 静音        | F9   | none |
| `Pause`             | 暂停        | F10  | none |
| `OpenCheatMenu`     | 打开金手指菜单 | F2 | none |
| `OpenShaderMenu`    | 打开着色器菜单 | F3 | none |
| `Screenshot`        | 截屏        | F12  | none |
| `ExitGame`          | 退出游戏    | ESC  | none |

#### 配置 I/O 接口

- `setDefaults(ConfigManager&)` – 写入所有配置键的默认值（不覆盖已有值）
- `load(const ConfigManager&)` – 从配置读取全部绑定及快进/倒带设置
- `gameButtonMap()` – 获取游戏按键映射表（只读引用）
- `hotkeyBinding(Hotkey)` – 获取指定功能键的绑定（只读引用）

#### 公共静态解析器（供未来设置 UI 复用）

- `parseKeyboardScancode(string)` – 名称（"X", "TAB"）或数字字符串 → 扫描码
- `parseGamepadButton(string)` – 名称（"A", "RT"）或数字字符串 → 手柄键 ID
- `parseKeyCombo(string)` – 解析组合键字符串："CTRL+F5"、"SHIFT+Z"、"F12"、"none"

#### 热键元数据（供设置 UI 枚举用）

- `hotkeyKbdConfigKey(Hotkey)` – 返回该热键的键盘配置键名（如 `"hotkey.quicksave.kbd"`）
- `hotkeyPadConfigKey(Hotkey)` – 返回该热键的手柄配置键名（如 `"hotkey.quicksave.pad"`）
- `hotkeyDisplayName(Hotkey)` – 返回 UTF-8 人可读名称（如 `"快速保存"`）

---

### `src/Control/InputMapping.cpp`

从 `src/Game/game_view.cpp` 迁移并整理的所有查找表和解析逻辑：

- `k_kbdKeyNames[]` – 键盘按键名称 ↔ `BrlsKeyboardScancode` 映射（含修饰键 LCTRL/RSHIFT 等）
- `k_brlsBtnNames[]` – 手柄按键名称 ↔ `brls::ControllerButton` 映射
- `k_retroNames[]` – Retro 按钮名称 ↔ `RETRO_DEVICE_ID_JOYPAD_*` 映射
- `k_defaultButtonMap[]` – 手柄默认按键映射（不含 BUTTON_X，保留其作为退出键）
- `k_defaultKbMap[]` – 键盘默认按键映射（与原代码一致：X→A, Z→B, A→Y…）
- `k_hotkeyMeta[]` – 所有模拟器热键的配置键名和默认值

---

## 二、修改文件

### `include/Game/game_view.hpp`

**移除的旧成员（13 个）：**
`m_ffButton`、`m_rewindButton`、`m_kbExitKey`、`m_ffMultiplier`、`m_ffMute`、`m_ffToggleMode`、
`m_rewindEnabled`、`m_rewindBufSize`、`m_rewindStep`、`m_rewindMute`、`m_rewindToggleMode`、
`BtnMap` / `m_buttonMap`、`KbMap` / `m_kbButtonMap`

**新增成员：**
- `beiklive::InputMappingConfig m_inputMap` – 统一管理所有输入配置
- `m_ffTogglePrevKey` – 快进切换键的上一帧状态（用于上升沿检测）
- `m_rewindTogglePrevKey` – 倒带切换键的上一帧状态

**移除的方法：**
- `loadButtonMaps()` – 现由 `InputMappingConfig::load()` 替代

---

### `src/Game/game_view.cpp`

- **删除**：所有静态查找表（`k_defaultButtonMap`、`k_kbdKeyNames` 等）及解析函数（`parseKbdScancode()`、`parseBrlsButton()`）
- **删除**：`loadButtonMaps()` 函数体
- **简化** `initialize()`：
  - 手动设置 ~30 个配置默认值的代码 → 替换为 `m_inputMap.setDefaults(*cfg)` 一行
  - 读取 FF/Rewind 各配置项的代码 → 替换为 `m_inputMap.load(*cfg)` 一行
- **更新** `startGameThread()`：`m_ffMultiplier` → `m_inputMap.ffMultiplier`，以此类推
- **重写** `pollInput()`：
  - 统一通过 `isHotkeyPressed()` lambda 读取热键状态
  - 正确分离 `FastForwardHold`（受 `ffToggleMode` 影响）和 `FastForwardToggle`（始终边缘检测切换）
  - 同样处理 `RewindHold` 和 `RewindToggle`
  - 游戏按键读取改为遍历 `m_inputMap.gameButtonMap()` 表
  - 退出键改为使用 `Hotkey::ExitGame` 热键绑定

---

### `src/Retro/LibretroLoader.cpp`

修正了一处遗留的错误 include 路径：`Game/LibretroLoader.hpp` → `Retro/LibretroLoader.hpp`

---

## 三、配置键说明

### 游戏按键（向后兼容，格式不变）

```ini
# 手柄游戏键映射
handle.a = A         # 可用名称：A, B, X, Y, UP, DOWN, LEFT, RIGHT,
handle.b = B         #           LB, RB, LT, RT, START, BACK, LSB, RSB
...

# 键盘游戏键映射
keyboard.a = X       # 可用名称：A-Z, 0-9, F1-F12, ESC, ENTER, SPACE,
keyboard.b = Z       #           TAB, UP, DOWN, LEFT, RIGHT, GRAVE 等
...
```

### 模拟器系统热键（新增）

```ini
# 格式：hotkey.<功能>.kbd = <按键或组合键>
#       hotkey.<功能>.pad = <手柄键名或 none>
#
# 组合键格式示例：CTRL+F5, SHIFT+Z, F12, none

hotkey.quicksave.kbd = F5          # 快速保存
hotkey.quicksave.pad = none
hotkey.quickload.kbd = F8          # 快速读取
hotkey.quickload.pad = none
hotkey.menu.kbd = F1               # 打开菜单
hotkey.cheat_menu.kbd = F2         # 打开金手指菜单
hotkey.shader_menu.kbd = F3        # 打开着色器菜单
hotkey.mute.kbd = F9               # 静音
hotkey.pause.kbd = F10             # 暂停
hotkey.screenshot.kbd = F12        # 截屏
hotkey.fastforward_toggle.kbd = none  # 快进（切换，独立于 hold 键）

# 以下继承旧配置键名（向后兼容）
keyboard.fastforward = TAB         # 快进（保持）键盘键
handle.fastforward = RT            # 快进（保持）手柄键
keyboard.rewind = GRAVE            # 倒带（保持）键盘键
handle.rewind = LT                 # 倒带（保持）手柄键
keyboard.exit = ESC                # 退出游戏键盘键
```

---

## 四、设计说明

### 组合键支持

`KeyCombo` 结构体保存主键 + `ctrl/shift/alt` 标志位。配置文件中使用 `+` 分隔：

```ini
hotkey.quicksave.kbd = CTRL+F5    # Ctrl+F5 组合键
hotkey.mute.kbd = ALT+M           # Alt+M 组合键
```

`pollInput()` 中通过 `getKeyboardKeyState()` 同时检查主键和修饰键，只有全部满足时才触发。

### 快进/倒带的双模式设计

每个操作（快进/倒带）都有两个独立热键：
- **Hold 键**（`FastForwardHold`）：默认 RT/TAB；行为受 `fastforward.mode = hold/toggle` 配置影响
  - `hold` 模式：按住激活
  - `toggle` 模式：按一次切换（向后兼容）
- **Toggle 键**（`FastForwardToggle`）：默认未绑定；始终为边缘检测切换模式

两个热键的状态通过 OR 合并（`ffHoldKey || m_ffToggled`），互不干扰。

### BUTTON_X 保留策略

手柄 X 键（`BUTTON_X`）仍保留作为游戏内 Borealis 动作（退出游戏）。`k_defaultButtonMap` 和 `k_defaultKbMap` 中均不包含 `JOYPAD_X` 的默认映射，与原代码行为一致。用户如需使用 JOYPAD_X，可通过 `handle.x = <其他键>` 自行配置。

---

## 五、后续工作（待实施）

1. **键盘映射界面**：`InputMappingConfig` 的三个静态解析器及 `hotkeyKbdConfigKey/hotkeyDisplayName` 接口已为设置 UI 预留，可直接调用
2. **功能键实现**：`QuickSave`、`QuickLoad`、`OpenMenu`、`Mute`、`Pause`、`OpenCheatMenu`、`OpenShaderMenu`、`Screenshot` 的热键绑定已完成，`GameView::pollInput()` 检测到对应按键后调用对应逻辑即可（当前仅配置接口，功能实现待下一 session）

---

## 六、变更文件汇总

### 新增文件

| 文件 | 说明 |
|------|------|
| `include/Control/InputMapping.hpp` | 输入映射配置类定义 |
| `src/Control/InputMapping.cpp` | 输入映射配置类实现（查找表 + 解析器 + 配置 I/O） |
| `report/session_39.md` | 本报告 |

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `include/Game/game_view.hpp` | 用 `m_inputMap` 替换旧按键映射成员 |
| `src/Game/game_view.cpp` | 移除旧查找表/解析函数，改用 `InputMappingConfig` |
| `src/Retro/LibretroLoader.cpp` | 修正 include 路径 |
