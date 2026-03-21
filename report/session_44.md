# Session 44 - PSP XMB Color Control, SettingPage Overhaul & Combo Key Support

## 任务完成情况

### 任务 1：PSP XMB 背景颜色控制（UI.pspxmb.color）

**已完成。**

#### 变更文件：
- `include/UI/Utils/ProImage.hpp`
  - 新增 `m_xmbBgR / m_xmbBgG / m_xmbBgB` 成员变量（默认深蓝色）
  - 新增 `m_xmbUBgColor` GL uniform 位置变量
  - 新增公开方法 `setXmbBgColor(float r, float g, float b)`

- `src/UI/Utils/ProImage.cpp`
  - GLSL fragment shader 新增 `uniform vec3 uBgColor;`，替换原先硬编码颜色 `vec3(0.74, 0.86, 0.14)`
  - `initXmbShader()` 中获取 `uBgColor` uniform 位置
  - `drawPspXmbShader()` 中每帧设置颜色 uniform
  - 实现 `setXmbBgColor()` 方法

- `include/common.hpp`
  - 新增常量宏：`KEY_UI_SHOW_BG_IMAGE`, `KEY_UI_BG_IMAGE_PATH`, `KEY_UI_SHOW_XMB_BG`, `KEY_UI_PSPXMB_COLOR`, `KEY_UI_TEXT_COLOR`, `KEY_AUDIO_BUTTON_SFX`, `KEY_DEBUG_LOG_LEVEL`, `KEY_DEBUG_LOG_FILE`, `KEY_DEBUG_LOG_OVERLAY`
  - 声明 `ApplyXmbColor(ProImage*)` 函数

- `src/common.cpp`
  - 新增 8 种预设颜色表 `k_xmbColorPresets`：深蓝、紫色、绿色、橙色、红色、青色、黑色、原版
  - 实现 `ApplyXmbColor()`：读取 `UI.pspxmb.color` 配置项，应用对应颜色到 ProImage
  - `InsertBackground()` 现在调用 `ApplyXmbColor()` 应用用户颜色

#### 预设颜色列表（配置值 → RGB）：

| 配置值     | 显示名 | R    | G    | B    |
|-----------|--------|------|------|------|
| blue      | 深蓝   | 0.05 | 0.10 | 0.25 |
| purple    | 紫色   | 0.15 | 0.05 | 0.30 |
| green     | 绿色   | 0.05 | 0.20 | 0.10 |
| orange    | 橙色   | 0.30 | 0.15 | 0.05 |
| red       | 红色   | 0.25 | 0.05 | 0.05 |
| cyan      | 青色   | 0.05 | 0.20 | 0.25 |
| black     | 黑色   | 0.02 | 0.02 | 0.05 |
| original  | 原版   | 0.74 | 0.86 | 0.14 |

---

### 任务 2：控制层组合键输入支持

**已存在，无需修改。**

`InputMappingConfig::parseKeyCombo()` 已支持 `CTRL+F5`、`SHIFT+TAB` 等组合键格式。`game_view.cpp` 中的 `pollInput()` / `isHotkeyPressed()` 已完整检查修饰键（CTRL/SHIFT/ALT）状态。

---

### 任务 3：完善 SettingPage 的 Tab 界面

**已完成。** 完全重写 `src/UI/Pages/SettingPage.cpp`，实现 6 个子页面，每个页面使用 `brls::ScrollingFrame` 包裹内容，各节用 `brls::Header` 分隔，底部有保存按钮。

#### Tab 1：界面设置

| 节标题          | 设置项               | 组件类型         | 配置键                  |
|----------------|---------------------|-----------------|------------------------|
| 背景图片        | 显示背景图片         | BooleanCell     | UI.showBgImage         |
| 背景图片        | 背景图片路径 (PNG)   | DetailCell      | UI.bgImagePath         |
| PSP XMB 风格背景 | 显示 XMB 背景       | BooleanCell     | UI.showXmbBg           |
| PSP XMB 风格背景 | 颜色设置            | SelectorCell    | UI.pspxmb.color        |
| 文字            | 文字颜色             | SelectorCell    | UI.textColor           |

背景图片路径点击后会推入一个 `FileListPage`（PNG 白名单过滤器）。

#### Tab 2：游戏设置

| 节标题                      | 设置项       | 组件类型     | 配置键                        |
|----------------------------|-------------|-------------|------------------------------|
| 加速（最大速度依赖机器性能）| 启用加速     | BooleanCell | fastforward.enabled          |
| 加速                        | 按键模式     | SelectorCell | fastforward.mode             |
| 加速                        | 加速倍率     | SelectorCell | fastforward.multiplier       |
| 倒带                        | 启用倒带     | BooleanCell | rewind.enabled               |
| 倒带                        | 按键模式     | SelectorCell | rewind.mode                  |
| 倒带                        | 倒带缓存数量 | SelectorCell | rewind.bufferSize            |
| 倒带                        | 倒带步数     | SelectorCell | rewind.step (1–5)            |
| GBA/GBC 游戏               | GB 机型     | SelectorCell | core.mgba_gb_model           |
| GBA/GBC 游戏               | 使用 BIOS   | BooleanCell | core.mgba_use_bios           |
| GBA/GBC 游戏               | 跳过 BIOS   | BooleanCell | core.mgba_skip_bios          |
| GBA/GBC 游戏               | GB 调色板   | SelectorCell | core.mgba_gb_colors          |
| GBA/GBC 游戏               | 空闲优化    | SelectorCell | core.mgba_idle_optimization  |

#### Tab 3：画面设置

| 节标题             | 设置项        | 组件类型     | 配置键                     |
|------------------|--------------|-------------|---------------------------|
| 画面显示          | 显示模式      | SelectorCell | display.mode              |
| 画面显示          | 纹理过滤      | SelectorCell | display.filter            |
| 画面显示          | 显示帧率(FPS) | BooleanCell | display.showFps           |
| 画面显示          | 显示快进提示  | BooleanCell | display.showFfOverlay     |
| 画面显示          | 显示倒带提示  | BooleanCell | display.showRewindOverlay |

#### Tab 4：声音设置

| 节标题      | 设置项          | 组件类型     | 配置键                              |
|-----------|----------------|-------------|-------------------------------------|
| 模拟器声音 | 模拟器按键音效  | BooleanCell | audio.buttonSfx                     |
| 游戏声音   | 快进时静音      | BooleanCell | fastforward.mute                    |
| 游戏声音   | 倒带时静音      | BooleanCell | rewind.mute                         |
| 游戏声音   | 低通滤波器      | SelectorCell | core.mgba_audio_low_pass_filter     |

#### Tab 5：按键预设

- **手柄**节：列出全部 14 个游戏按键（handle.*）+ 全部快捷键手柄绑定（hotkey.*.pad）
- **键盘**节（Switch 端通过 `#ifndef __SWITCH__` 隐藏）：列出全部 14 个游戏按键（keyboard.*）+ 全部快捷键键盘绑定（hotkey.*.kbd）

**按键监听窗口（KeyCaptureView）：**
- 基于 `brls::Dialog` 内嵌 `KeyCaptureView`（继承 `brls::Box`）
- 通过 `draw()` 每帧轮询输入，无需额外线程
- 5 秒倒计时，超时自动关闭
- 键盘模式：检测 CTRL/SHIFT/ALT 修饰键 + 主键，生成组合键字符串（如 `CTRL+F5`）；最多支持 2 键组合
- 手柄模式：检测单个按键（如 `RT`、`LB`）
- 检测到按键后自动关闭对话框，更新 DetailCell 显示

#### Tab 6：调试工具

| 节标题   | 设置项      | 组件类型     | 配置键                  |
|---------|-----------|-------------|------------------------|
| 日志设置 | 日志等级   | SelectorCell | debug.logLevel         |
| 日志设置 | 日志文件写入 | BooleanCell | debug.logFile          |
| 日志设置 | 日志浮窗   | BooleanCell | debug.logOverlay       |

日志等级变更立即调用 `brls::Logger::setLogLevel()`；日志文件写入使用静态 FILE* 跟踪，避免资源泄漏。

---

### 新增 SettingManager 默认值

#### `main.cpp::RunnerInit()`
- `UI.showBgImage` = "false"
- `UI.bgImagePath` = ""
- `UI.showXmbBg` = "false"
- `UI.pspxmb.color` = "blue"
- `UI.textColor` = "white"
- `audio.buttonSfx` = "false"
- `debug.logLevel` = "info"
- `debug.logFile` = "false"
- `debug.logOverlay` = "false"

#### `src/Control/InputMapping.cpp::setDefaults()`
- `fastforward.enabled` = "true"（新增）

---

## 安全摘要

- 所有新增代码经过 CodeQL 扫描，未发现安全漏洞
- `dispModeIds`、`logLevelIds` 均声明为 `static const char*`，避免了 lambda 捕获栈变量导致的未定义行为
- 日志文件 FILE* 使用 `static` 追踪并在重新打开前正确关闭，避免资源泄漏
- 无新增外部依赖

---

## 编译验证

Linux 平台（`PLATFORM_DESKTOP=ON`）编译成功：`[100%] Built target GBAStation`
