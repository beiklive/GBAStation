# Session 46 - #47 之后发现的三个问题修复

## 问题描述

PR #47 合并后发现以下三个问题：

1. **SettingPage XMB 调色器无即时生效**：`UI.pspxmb.color` 修改并保存后，现有 activity 的 `ProImage` 背景不会更新颜色。
2. **按键映射弹窗瞬间关闭**：点击某按钮打开映射弹窗时，按下按钮（用于打开弹窗的 A 键）被立即捕获为新映射，弹窗瞬间消失。另外，如果映射的是已有按键，弹窗会立即以已映射的键触发并关闭。
3. **游戏运行时按键全部失灵**：`pollInput()` 的键盘模式检测仅检查7个固定键（UP/DOWN/LEFT/RIGHT/X/Z/ENTER），若用户首次按下的是其他已映射键（如 Q/W/E/R/A/S），键盘模式无法激活，这些键在手柄模式下也无对应映射，导致实际失灵。

---

## 修复方案

### 问题 1：XMB 颜色修改未作用于所有 activity 背景

**根因**：`InsertBackground()` 创建的 `ProImage` 实例在各 `BBox` 中独立存在，没有集中管理。颜色选择器只更新 `SettingManager`（内存中的 config），但不会通知已存在的 `ProImage` 实例。

**修复**：

- **`src/common.cpp`**：新增全局注册表 `s_xmbBackgrounds`（`std::set<ProImage*>`）
  - `RegisterXmbBackground(img)`：注册
  - `UnregisterXmbBackground(img)`：注销
  - `ApplyXmbColorToAll()`：遍历注册表，对所有实例调用 `ApplyXmbColor()`
  - `InsertBackground()` 现在在创建 `ProImage` 后调用 `RegisterXmbBackground()`

- **`include/common.hpp`**：声明三个新函数

- **`src/UI/Utils/ProImage.cpp`**：
  - 引入 `common.hpp`
  - 析构函数中调用 `UnregisterXmbBackground(this)`，确保 ProImage 销毁时自动注销

- **`src/UI/Pages/SettingPage.cpp`**：
  - XMB 颜色选择回调中，颜色变更后立即调用 `beiklive::ApplyXmbColorToAll()`
  - 保存按钮回调中也调用 `beiklive::ApplyXmbColorToAll()`

**效果**：用户在 SettingPage 中选择颜色后，所有已显示的 activity 背景立即更新为新颜色；点击保存时同步更新。

---

### 问题 2：按键映射弹窗瞬间关闭

**根因**：`KeyCaptureView::pollGamepad()` 和 `pollKeyboard()` 从第一帧就开始检测按键。打开弹窗时，A 按键（或用于触发打开的任意键）仍处于按下状态，被立即捕获为新映射。

**修复**：

- **`src/UI/Pages/SettingPage.cpp`** 中的 `KeyCaptureView` 类：
  - 新增成员变量 `bool m_waitingForRelease = true`
  - 新增 `checkGamepadRelease()` 方法：检测所有追踪按键是否已全部释放，若是则将 `m_waitingForRelease` 置为 `false`
  - 新增 `checkKeyboardRelease()` 方法：同上，同时检测 CTRL/SHIFT/ALT 修饰键
  - `draw()` 方法中：若 `m_waitingForRelease` 为 `true`，调用释放检测而非正式捕获；同时重置倒计时起始时间，确保用户得到完整的 5 秒捕获窗口

**效果**：弹窗打开后等待所有按键释放，再开始监听新按键，完全消除误触发。

---

### 问题 3：游戏运行时自定义映射按键失灵

**根因**：`pollInput()` 中用于切换键盘模式的检测集固定为 7 个键（UP/DOWN/LEFT/RIGHT/X/Z/ENTER）。若用户的第一次按键是其他已配置的键（如 Q→JOYPAD_L、A→JOYPAD_Y 等），`keyboardActive = false`，键盘模式不激活，而这些键在手柄模式下（`state.buttons`）也无对应映射，因此完全无效。

**修复**：

- **`src/Game/game_view.cpp`** 的 `pollInput()` 函数：
  - 将 `const auto& btnMap = m_inputMap.gameButtonMap()` 提前到函数作用域顶部（供键盘检测和下方游戏按键循环共用）
  - 替换固定检测集为遍历完整 `btnMap`：对所有 `kbdScancode >= 0` 的条目调用 `im->getKeyboardKeyState()`，任意一个键被按下即激活键盘模式
  - 同时遍历所有热键绑定（快进/倒带等），若有键盘快捷键被按下也触发键盘模式

**效果**：无论用户配置了何种键盘映射，只要按下任意已映射的键，即可自动切换到键盘模式，所有自定义映射均生效。

---

## 变更文件清单

| 文件 | 变更说明 |
|------|----------|
| `include/common.hpp` | 新增 `RegisterXmbBackground` / `UnregisterXmbBackground` / `ApplyXmbColorToAll` 声明 |
| `src/common.cpp` | 新增 XMB 背景注册表及三个注册管理函数；`InsertBackground()` 注册新 ProImage |
| `src/UI/Utils/ProImage.cpp` | 引入 `common.hpp`；析构函数中注销 XMB 背景注册 |
| `src/UI/Pages/SettingPage.cpp` | XMB 颜色回调和保存按钮调用 `ApplyXmbColorToAll()`；`KeyCaptureView` 新增释放等待机制 |
| `src/Game/game_view.cpp` | `pollInput()` 中键盘模式检测改为遍历完整按键映射表 |

---

## 验证

构建命令：
```bash
mkdir -p build_linux && cd build_linux
cmake .. -DPLATFORM_DESKTOP=ON -DCMAKE_BUILD_TYPE=Release
cmake --build . -j $(nproc)
```

编译通过，无新增错误或警告。
