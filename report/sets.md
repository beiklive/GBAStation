# GBAStation 设置项整理报告

> 生成时间：2026-04-03
> 说明：本文档整理了 GBAStation 所有设置项的当前状态（已生效 / 未生效）及待新增的设置项。

---

## 一、已生效的设置项

### 1. 界面设置（buildUITab）

| 配置键 | 描述 | 默认值 | 读取位置 |
|--------|------|--------|----------|
| `save.sramDir` | SRAM 存档目录（空=ROM所在目录，非空=模拟器目录） | `""` | `GamePage._initGameEntryPaths()` |
| `cheat.dir` | 金手指文件目录（空=ROM所在目录，非空=模拟器目录） | `""` | `GamePage._initGameEntryPaths()` |

### 2. 游戏设置（buildGameTab）

| 配置键 | 描述 | 默认值 | 读取位置 |
|--------|------|--------|----------|
| `core.mgba_gb_model` | GB 机型（Autodetect/Game Boy/GBC/GBA等） | `Autodetect` | mGBA 核心配置 |
| `core.mgba_use_bios` | 是否使用 BIOS | `ON` | mGBA 核心配置 |
| `core.mgba_skip_bios` | 是否跳过 BIOS 动画 | `OFF` | mGBA 核心配置 |
| `core.mgba_gb_colors` | GB 配色方案 | `Grayscale` | mGBA 核心配置 |
| `core.mgba_idle_optimization` | 空闲循环优化策略 | `Remove Known` | mGBA 核心配置 |

### 3. 画面设置（buildDisplayTab）

| 配置键 | 描述 | 默认值 | 读取位置 |
|--------|------|--------|----------|
| `display.overlay.gbaPath` | GBA 全局遮罩 PNG 路径 | `""` | `GamePage._initGameEntryPaths()` |
| `display.overlay.gbcPath` | GBC 全局遮罩 PNG 路径 | `""` | `GamePage._initGameEntryPaths()` |
| `display.overlay.gbPath` | GB 全局遮罩 PNG 路径 | `""` | `GamePage._initGameEntryPaths()` |

> **注意**：遮罩路径已被读取并写入 `m_gameEntry.overlayPath`，但遮罩渲染功能尚未在 GameView 中实现，因此路径设置已生效但视觉效果暂时无法显示。

### 4. 音频设置（buildAudioTab）

| 配置键 | 描述 | 默认值 | 读取位置 |
|--------|------|--------|----------|
| `core.mgba_audio_low_pass_filter` | GBA 音频低通滤波器（enabled/disabled） | `disabled` | mGBA 核心配置 |
| `audio.buttonSfx` | 按钮 UI 音效开关 | `1`（开） | `BKAudioPlayer.isButtonSfxEnabled()` |

### 5. 按键绑定（buildKeyBindTab）

| 配置键 | 描述 | 默认值 | 读取位置 |
|--------|------|--------|----------|
| `handle.a` | GBA A 键映射 | `none` | `GameView._registerHotkeys()` |
| `handle.b` | GBA B 键映射 | `none` | `GameView._registerHotkeys()` |
| `handle.x` | GBA X 键映射 | `none` | `GameView._registerHotkeys()` |
| `handle.y` | GBA Y 键映射 | `none` | `GameView._registerHotkeys()` |
| `handle.up` | GBA 上方向键映射 | `none` | `GameView._registerHotkeys()` |
| `handle.down` | GBA 下方向键映射 | `none` | `GameView._registerHotkeys()` |
| `handle.left` | GBA 左方向键映射 | `none` | `GameView._registerHotkeys()` |
| `handle.right` | GBA 右方向键映射 | `none` | `GameView._registerHotkeys()` |
| `handle.l` | GBA L1 键映射 | `none` | `GameView._registerHotkeys()` |
| `handle.r` | GBA R1 键映射 | `none` | `GameView._registerHotkeys()` |
| `handle.l2` | GBA L2 键映射 | `none` | `GameView._registerHotkeys()` |
| `handle.r2` | GBA R2 键映射 | `none` | `GameView._registerHotkeys()` |
| `handle.start` | GBA START 键映射 | `none` | `GameView._registerHotkeys()` |
| `handle.select` | GBA SELECT 键映射 | `none` | `GameView._registerHotkeys()` |
| `handle.lstick_up/down/left/right` | 左摇杆方向映射 | `none` | `GameView._registerHotkeys()` |
| `handle.rstick_up/down/left/right` | 右摇杆方向映射 | `none` | `GameView._registerHotkeys()` |
| `handle.fastforward` | 快进触发按键 | `LSB` | `GameView._registerHotkeys()` |
| `handle.rewind` | 倒带触发按键 | `RSB` | `GameView._registerHotkeys()` |
| `hotkey.quicksave.pad` | 快速保存热键 | `none` | `GameView._registerHotkeys()` |
| `hotkey.quickload.pad` | 快速读取热键 | `none` | `GameView._registerHotkeys()` |
| `hotkey.menu.pad` | 打开游戏菜单热键 | `LT+RT` | `GameView._registerHotkeys()` |
| `hotkey.mute.pad` | 静音切换热键 | `none` | `GameView._registerHotkeys()` |
| `input.joystick.enabled` | 左摇杆方向键输入开关 | `1`（开） | `GameView._registerHotkeys()` |
| `input.joystick.diagonal` | 允许摇杆斜向输入 | `1`（开） | `GameView._registerHotkeys()` |

### 6. 调试工具（buildDebugTab）

| 配置键 | 描述 | 默认值 | 读取位置 |
|--------|------|--------|----------|
| `debug.logLevel` | 日志输出级别（debug/info/warning/error） | `info` | `brls::Logger::setLogLevel()` |
| `debug.logFile` | 是否将日志输出到文件 | `0`（关） | `brls::Logger::setLogOutput()` |
| `debug.logOverlay` | 是否显示调试信息覆盖层 | `0`（关） | `brls::Application::enableDebuggingView()` |

---

## 二、已移除的无效设置项

以下设置项由于对应功能尚未实现或代码中未读取，已从 SettingPage 中移除：

### 界面 Tab 移除项

| 配置键 | 描述 | 移除原因 |
|--------|------|----------|
| `UI.showBgImage` | 显示背景图片 | Box.cpp 未读取，背景渲染未接入设置 |
| `UI.bgImagePath` | 背景图片路径 | 同上 |
| `UI.bgBlurEnabled` | 背景模糊开关 | 同上 |
| `UI.bgBlurRadius` | 背景模糊半径 | 同上 |
| `UI.showXmbBg` | 显示 XMB 风格背景 | XMB 背景渲染未接入设置 |
| `UI.pspxmb.color` | XMB 颜色预设 | 同上 |
| `save.autoSaveState` | 自动存档开关 | 游戏循环未读取此设置 |
| `save.autoSaveInterval` | 自动存档间隔 | 同上 |
| `save.autoLoadState0` | 启动时自动读取存档0 | 游戏初始化未读取此设置 |
| `save.stateDir` | 即时存档目录 | GameView 使用 `m_gameEntry.savePath`（来自 sramDir） |
| `UI.useSavestateThumbnail` | 无封面时使用存档截图 | 缩略图加载逻辑未读取此设置 |
| `screenshot.dir` | 截图保存目录 | GamePage 使用 `screenshotPath()` 固定路径 |
| `cheat.enabled` | 启用金手指 | 游戏核心初始化未读取此开关 |

### 游戏 Tab 移除项

| 配置键 | 描述 | 移除原因 |
|--------|------|----------|
| `fastforward.enabled` | 启用快进 | GameView 未检查此开关，快进始终可用 |
| `fastforward.mode` | 快进触发方式 | GameView 使用固定切换逻辑，未读取 |
| `fastforward.multiplier` | 快进倍率 | 使用硬编码的 `FF_MULTIPLIER` |
| `rewind.enabled` | 启用倒带 | GameView 未检查此开关，倒带始终可用 |
| `rewind.mode` | 倒带触发方式 | GameView 使用固定切换逻辑，未读取 |
| `rewind.bufferSize` | 倒带缓冲大小 | 使用硬编码的 `REWIND_BUFFER_SIZE` |
| `rewind.step` | 倒带步长 | 使用硬编码的 `REWIND_STEP` |

### 画面 Tab 移除项

| 配置键 | 描述 | 移除原因 |
|--------|------|----------|
| `display.mode` | 全局显示模式 | GameView 从 GameEntry 读取，GamePage 未从全局设置填充 |
| `display.integer_scale_mult` | 整数倍缩放 | 同上 |
| `display.filter` | 纹理过滤模式 | 同上 |
| `display.showFps` | 显示帧率 | GameView 始终显示 FPS，未读取此开关 |
| `display.showFfOverlay` | 显示快进状态 | GameView 始终显示，未读取此开关 |
| `display.showRewindOverlay` | 显示倒带状态 | 同上 |
| `display.showMuteOverlay` | 显示静音状态 | 同上 |
| `display.overlay.enabled` | 遮罩总开关 | GamePage 只读取路径，不检查此开关 |
| `display.shaderEnabled` | 着色器总开关 | GamePage 未从全局设置填充 shaderEnabled |
| `display.shader` / `display.shader.gba/gbc/gb` | 着色器路径 | GamePage 未读取全局着色器设置 |

### 音频 Tab 移除项

| 配置键 | 描述 | 移除原因 |
|--------|------|----------|
| `fastforward.mute` | 快进时静音 | 游戏循环未读取此设置 |
| `rewind.mute` | 倒带时静音 | 游戏循环未读取此设置 |

### 按键 Tab 移除项

| 配置键 | 描述 | 移除原因 |
|--------|------|----------|
| `hotkey.pause.pad` | 暂停热键 | GameView 未注册此热键（暂停通过进入菜单实现） |
| `hotkey.screenshot.pad` | 截屏热键 | GameView 未注册此热键，截图功能未实现 |

---

## 三、待新增的设置项

以下设置项对应已有代码框架但尚未接入设置系统，建议后续实现并添加到 SettingPage：

### 快进 / 倒带控制

| 配置键（建议） | 描述 | 涉及修改位置 |
|----------------|------|--------------|
| `fastforward.multiplier` | 快进倍率（2x/3x/4x/6x/8x） | `GameView::FF_MULTIPLIER` 改为读取配置 |
| `rewind.bufferSize` | 倒带缓冲大小（秒） | `GameView::REWIND_BUFFER_SIZE` 改为读取配置 |
| `rewind.step` | 倒带步长（帧数） | `GameView::REWIND_STEP` 改为读取配置 |
| `fastforward.mute` | 快进时静音 | 在 `GameView::_pushFrameAudio()` 中增加判断 |
| `rewind.mute` | 倒带时静音 | 在 `GameView::_pushFrameAudio()` 中增加判断 |

### 画面显示控制

| 配置键（建议） | 描述 | 涉及修改位置 |
|----------------|------|--------------|
| `display.mode` | 全局默认显示模式 | `GamePage._initGameEntryPaths()` 添加全局回落 |
| `display.filter` | 全局默认纹理过滤 | 同上 |
| `display.showFps` | 控制 FPS 覆盖层显示 | `GameView::_drawOverlays()` 检查此设置 |
| `display.showFfOverlay` | 控制快进覆盖层显示 | 同上 |
| `display.showRewindOverlay` | 控制倒带覆盖层显示 | 同上 |
| `display.showMuteOverlay` | 控制静音覆盖层显示 | 同上 |
| `display.overlay.enabled` | 遮罩总开关 | `GamePage._initGameEntryPaths()` 增加开关检查 |
| `display.shaderEnabled` | 全局着色器开关 | `GamePage._initGameEntryPaths()` 增加着色器回落 |
| `display.shader.gba/gbc/gb` | 平台对应全局着色器路径 | 同上 |

### 存档 / 功能控制

| 配置键（建议） | 描述 | 涉及修改位置 |
|----------------|------|--------------|
| `save.stateDir` | 即时存档独立目录 | `GameView::getStatePath()` 单独读取此配置 |
| `save.autoLoadState0` | 启动时自动读取槽0 | `GameView._registerGameRuntime()` 后添加 `_doLoadState(0)` |
| `save.autoSaveState` | 游戏退出时自动存档 | `GameView` 析构时判断并调用 `_doSaveState()` |
| `cheat.enabled` | 金手指全局开关 | `GameRun::SetupGame()` 中检查此开关决定是否加载金手指 |
| `screenshot.dir` | 截图保存目录 | `GameView` 截图功能实现后读取此配置 |
| `hotkey.pause.pad` | 暂停热键 | `GameView._registerHotkeys()` 中注册 |
| `hotkey.screenshot.pad` | 截屏热键 | `GameView._registerHotkeys()` 中注册并实现截图逻辑 |

### UI 主题 / 背景

| 配置键（建议） | 描述 | 涉及修改位置 |
|----------------|------|--------------|
| `UI.showBgImage` | 显示自定义背景图片 | `Box::setupBackgroundLayer()` 接入设置 |
| `UI.bgImagePath` | 自定义背景图片路径 | 同上 |
| `UI.bgBlurEnabled` | 背景图片模糊 | 同上 |
| `UI.bgBlurRadius` | 模糊半径 | 同上 |
| `UI.showXmbBg` | 显示 XMB 风格动态背景 | XMB 渲染器接入设置 |
| `UI.pspxmb.color` | XMB 背景颜色主题 | 同上 |
| `UI.useSavestateThumbnail` | 无封面时使用存档截图 | 游戏列表缩略图加载逻辑接入设置 |

---

## 四、按平台区分的音频实现

### BKAudioPlayer 各平台实现状态

| 平台 | 实现方式 | 状态 |
|------|----------|------|
| Nintendo Switch | libpulsar + qlaunch BFSAR 官方音效 | ✅ 已实现 |
| Linux | ALSA（加载 WAV 文件） | ✅ 已实现 |
| Windows | WinMM（加载 WAV 文件） | ✅ 已实现 |
| macOS | CoreAudio（加载 WAV 文件） | ✅ 已实现 |
| 其他 | 空实现（静默） | ✅ 已实现 |

### Switch 音效名称映射

| 枚举值 | Nintendo 官方音效名称 |
|--------|----------------------|
| SOUND_FOCUS_CHANGE | SeGiftReceive |
| SOUND_FOCUS_ERROR | SeKeyErrorCursor |
| SOUND_CLICK | SeBtnDecide |
| SOUND_BACK | SeFooterDecideFinish |
| SOUND_FOCUS_SIDEBAR | SeNaviFocus |
| SOUND_CLICK_ERROR | SeKeyError |
| SOUND_HONK | SeUnlockKeyZR |
| SOUND_CLICK_SIDEBAR | SeNaviDecide |
| SOUND_TOUCH_UNFOCUS | SeTouchUnfocus |
| SOUND_TOUCH | SeTouch |
| SOUND_SLIDER_TICK | SeSliderTickOver |
| SOUND_SLIDER_RELEASE | SeSliderRelease |
