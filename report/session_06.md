# BeikLiveStation 功能扩展报告

## 概述

本次修改实现了以下六项功能扩展：

1. 音频改进（已由上一轮修复解决 Switch 端）
2. 键盘/手柄双输入源支持及按键映射配置化
3. 加速倍率/静音/模式（按住/切换）可配置
4. 倒带功能（libretro serialize/unserialize，3600帧缓存，2步回退，ZL 默认键）
5. 游戏界面 FPS 显示
6. 倒带与加速均支持按住与切换模式

---

## 修改文件

| 文件 | 修改内容 |
|------|---------|
| `include/UI/game_view.hpp` | 新增倒带缓冲区、FPS 计数、快进/倒带配置字段、按键映射成员 |
| `src/UI/game_view.cpp` | 实现所有新功能（见下方各节） |
| `report/feature_report.md` | 本报告（新增） |

---

## 1. 按键映射配置化（keyboard. / handle. 前缀）

### 实现原理

- **handle.** 前缀：配置 gamepad/controller `ControllerButton` 枚举值 → retro 按键的映射
- **keyboard.** 前缀：配置 PC 键盘 `BrlsKeyboardScancode` 值 → retro 按键的映射
- **自动检测**：`pollInput()` 通过 `brls::Application::getPlatform()->getInputManager()->getKeyboardKeyState()` 采样若干常用键，若检测到键盘活动则切换为键盘模式；若检测到手柄按键则切回手柄模式

### 默认键盘映射

| 配置键 | 默认值（BrlsKeyboardScancode） | 对应 retro 功能 |
|--------|-------------------------------|-----------------|
| `keyboard.a` | X 键 (88) | GBA A 键 |
| `keyboard.b` | Z 键 (90) | GBA B 键 |
| `keyboard.y` | A 键 (65) | GBA Y（扩展） |
| `keyboard.up/down/left/right` | 方向键 | 方向键 |
| `keyboard.start` | Enter | Start |
| `keyboard.select` | S 键 | Select |
| `keyboard.l` | Q 键 | L 键 |
| `keyboard.r` | W 键 | R 键 |
| `keyboard.l2` | E 键 | L2 |
| `keyboard.r2` | R 键 | R2 |
| `keyboard.fastforward` | Tab (258) | 快进 |
| `keyboard.rewind` | `` ` `` (96) | 倒带 |

### 默认手柄映射

| 配置键 | 默认值（ControllerButton） | 对应 retro 功能 |
|--------|--------------------------|-----------------|
| `handle.a` | 13 (BUTTON_A) | GBA A 键 |
| `handle.b` | 12 (BUTTON_B) | GBA B 键 |
| `handle.y` | 11 (BUTTON_Y) | GBA Y（扩展） |
| `handle.up/down/left/right` | 3/5/6/4 | 方向键 |
| `handle.start` | 9 (BUTTON_START) | Start |
| `handle.select` | 7 (BUTTON_BACK) | Select |
| `handle.l` | 1 (BUTTON_LB) | L |
| `handle.r` | 15 (BUTTON_RB) | R |
| `handle.l2` | 0 (BUTTON_LT) | L2/ZL |
| `handle.r2` | 16 (BUTTON_RT) | R2/ZR |
| `handle.fastforward` | 16 (BUTTON_RT/ZR) | 快进 |
| `handle.rewind` | 0 (BUTTON_LT/ZL) | 倒带 |

---

## 2. 加速（快进）功能配置化

### 配置项

| 配置键 | 说明 | 默认值 |
|--------|------|--------|
| `fastforward.multiplier` | 加速倍率（支持浮点，如 0.5、0.1、4.0） | `4.0` |
| `fastforward.mute` | 加速时是否静音 | `true` |
| `fastforward.mode` | 按键模式：`hold`（按住）/ `toggle`（切换） | `hold` |

### 实现说明

- `multiplier >= 1.0`：计算 `round(multiplier)` 帧/迭代，不限速（尽可能快）
- `multiplier < 1.0`（如 0.5x 慢速）：每迭代仍运行 1 帧，但休眠 `1/(fps*multiplier)` 时长
- `mute = true` 时：每帧音频样本不送入 AudioManager，防止 ring buffer 溢出

---

## 3. 倒带功能

### 实现原理

使用 libretro `retro_serialize` / `retro_unserialize` 接口：
- 正常游戏时，每帧在运行 `retro_run()` 之前，调用 `m_core.serialize()` 保存状态到 `m_rewindBuffer`（`std::deque<std::vector<uint8_t>>`）
- `m_rewindBuffer` 前端为最新状态，后端为最旧状态
- 当倒带键被按下时，循环 `rewind.step` 次从前端弹出状态并调用 `m_core.unserialize()` 还原

### 配置项

| 配置键 | 说明 | 默认值 |
|--------|------|--------|
| `rewind.enabled` | 是否启用倒带 | `false` |
| `rewind.bufferSize` | 最大缓存帧数 | `3600` |
| `rewind.step` | 每次倒带回退的帧数 | `2` |
| `rewind.mute` | 倒带时是否静音 | `false` |
| `rewind.mode` | 按键模式：`hold` / `toggle` | `hold` |

> **注意**：启用倒带功能会显著增加内存占用（3600 帧 × 每帧状态大小）。GBA 每帧状态约 ~300KB，3600 帧约需 ~1GB，建议将 `rewind.bufferSize` 调整为合适值。

### 默认倒带键

- 手柄：ZL（`handle.rewind = 0 = BUTTON_LT`）
- 键盘：`` ` ``（Grave Accent，`keyboard.rewind = 96`）

---

## 4. FPS 显示

### 配置项

| 配置键 | 说明 | 默认值 |
|--------|------|--------|
| `display.showFps` | 是否在游戏画面角落显示 FPS | `false` |

### 实现说明

- 游戏线程每秒统计一次帧率（与渲染线程通过 `m_fpsMutex` 同步）
- `draw()` 调用时读取 `m_currentFps`，在屏幕左上角绘制半透明背景 + 绿色文字 `FPS: XX.X`
- 倒带中时额外在顶部中央显示 `<<< REWIND` 黄色提示

---

## 5. 倒带与加速的切换/按住模式

- `fastforward.mode = hold`：按住快进键生效，松开恢复
- `fastforward.mode = toggle`：按下快进键切换状态（再次按下取消）
- `rewind.mode = hold`：按住倒带键持续倒带
- `rewind.mode = toggle`：按下倒带键切换倒带状态

---

## 配置文件示例

```ini
; 加速配置
fastforward.multiplier = 4.0
fastforward.mute = true
fastforward.mode = hold

; 倒带配置（默认关闭，需手动开启）
rewind.enabled = false
rewind.bufferSize = 600
rewind.step = 2
rewind.mute = false
rewind.mode = hold

; 显示配置
display.showFps = false

; 手柄按键映射（值为 brls::ControllerButton 枚举整数）
handle.a = 13
handle.b = 12
handle.fastforward = 16
handle.rewind = 0

; 键盘按键映射（值为 BrlsKeyboardScancode 整数）
keyboard.a = 88
keyboard.b = 90
keyboard.fastforward = 258
keyboard.rewind = 96
```
