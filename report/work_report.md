# GBAStation 工作汇报

## 任务概述

本次任务基于 `old/` 目录下的旧版代码，将游戏渲染、音频、计时器、信号控制等功能移植到 `src/` 目录下的新架构中，并完善 CoreMgba 核心类与 GameView 游戏视图。

---

## 任务分析

### 目标
将分散于 `old/` 目录中耦合度过高的旧代码，按照新架构规范（职责单一、命名空间 beiklive、平台兼容）移植到 `src/` 目录。

### 输入
- `old/src/Video/renderer/` - 旧版 OpenGL 渲染器
- `old/src/Audio/AudioManager.*` - 旧版音频管理器
- `old/src/Game/game_view.*` - 旧版游戏视图（参考游戏线程实现）
- `src/game/mgba/GameRun.*` - 现有 CoreMgba 骨架

### 输出
- 新的渲染子系统（`src/game/render/`）
- 完善的 CoreMgba（`src/game/mgba/GameRun.*`）
- 带游戏线程的 GameView（`src/ui/utils/GameView.*`）
- 计时器单例（`src/core/GameTimer.hpp`）
- 信号控制单例（`src/core/GameSignal.hpp`）
- 音频管理器（`src/game/audio/AudioManager.*`）

### 挑战与解决方案
- **旧代码耦合度高**：参考实现逻辑，按新架构拆分为独立类
- **跨平台音频**：通过宏定义（`__SWITCH__`/`BK_AUDIO_WINMM`/`BK_AUDIO_ALSA`/`BK_AUDIO_COREAUDIO`）选择平台后端
- **线程安全**：游戏线程与 UI 线程之间使用 mutex + atomic 通信
- **帧率控制**：使用 sleep + 自旋等待确保精确的帧间隔

---

## 完成情况

### 任务1：移植游戏渲染代码到 src/game/render/

| 文件 | 说明 |
|------|------|
| `src/game/render/GameTexture.hpp/cpp` | GL 纹理封装，支持动态分辨率调整 |
| `src/game/render/FrameUploader.hpp/cpp` | CPU 帧数据上传工具（RGBA8888 / XRGB8888，GLES/GL 兼容） |
| `src/game/render/DirectQuadRenderer.hpp/cpp` | 直接 GL 四边形渲染器（跳过 NanoVG，保存/恢复 GL 状态） |
| `src/game/render/GameRenderer.hpp/cpp` | 主渲染器，组合以上组件，提供 `uploadFrame()` 和 `drawToScreen()` |

### 任务2：完善 CoreMgba 功能

新增方法：
- `RunFrame()` - 调用 `retro_run()` 执行一帧
- `Reset()` - 重置核心
- `Cleanup()` - 保存 SRAM/RTC，卸载核心
- `Serialize() / Unserialize()` - 状态序列化（快速存读档）
- `_initConfig()` - 向核心注册 mgba 默认配置变量
- `SetButtonState() / GetVideoFrame() / DrainAudio() / SetFastForwarding()` 等接口

### 任务3：GameView 游戏线程

- 添加 `_startGameThread() / _stopGameThread() / _gameLoop()` 方法
- 游戏线程按核心目标 FPS 执行 `RunFrame()`，支持暂停/快进信号
- 视频帧通过 mutex 传递给 UI 线程，在 `draw()` 中上传 GPU
- 音频数据推送至 `AudioManager`

### 任务4：计时器单例 GameTimer

- 文件：`src/core/GameTimer.hpp`
- 继承 `Singleton<GameTimer>`
- 提供 `start()`, `stop()`, `elapsedSeconds()`, `elapsedMilliseconds()` 等方法

### 任务5：原子信号控制单例 GameSignal

- 文件：`src/core/GameSignal.hpp`
- 继承 `Singleton<GameSignal>`
- 原子信号：暂停、快进、倒带、快速存档、快速读档、重置、静音、退出、打开菜单
- UI 线程写入，游戏线程读取/消费（`consume*()` 方法自动清除信号）

### 任务6：音频管理器 AudioManager

- 文件：`src/game/audio/AudioManager.hpp/cpp`
- 继承 `Singleton<AudioManager>`
- 环形缓冲区（8192 样本）+ 后台音频线程
- 平台后端通过宏定义：
  - `__SWITCH__` → libnx audout
  - `BK_AUDIO_WINMM` → Windows WinMM waveOut
  - `BK_AUDIO_ALSA` → Linux ALSA
  - `BK_AUDIO_COREAUDIO` → macOS CoreAudio
  - 其他 → 空输出（静默丢弃）

---

## 架构说明

```
UI 线程 (draw/brls)              游戏线程 (_gameLoop)
─────────────────────────        ──────────────────────────
GameView::draw()                 CoreMgba::RunFrame()
  ├─ 消费 GameSignal 退出信号       ├─ 获取视频帧 GetVideoFrame()
  ├─ 初始化 GameRenderer（首帧）   │  └─ mutex 传入 m_pendingFrame
  ├─ _uploadPendingFrame()         ├─ 推送音频 DrainAudio()
  │   └─ GameRenderer::            │  └─ AudioManager::pushSamples()
  │      uploadFrame()             └─ 帧率控制（sleep + 自旋）
  └─ GameRenderer::drawToScreen()

GameSignal（原子信号桥梁）：
  UI 线程写入 → 游戏线程轮询读取
  pause / fastForward / rewind / quickSave / quickLoad / reset / mute
```

---

## 任务三：移植倒带/快进、优化帧率控制、动画类、整理游戏主循环

### 任务分析

**任务目标**：
1. 移植 `old` 版本的倒带和快进功能到 `GameView` 游戏线程
2. 使用严格时钟累加模式解决非快进状态下帧率不足55fps的问题
3. 制作 `AnimationHelper` 动画类，实现 View 显隐和 Activity 的出场/入场动画
4. 整理 `_gameLoop` 代码，提取封装多个辅助方法防止代码过长

**输入**：
- `old/src/Game/game_view.cpp` - 旧版倒带/快进逻辑参考
- 现有 `src/ui/utils/GameView.cpp` - 需要改进的游戏主循环
- `third_party/borealis` - borealis 动画系统（Animatable/Ticking）

**挑战与解决方案**：
- **倒带状态缓冲区**：使用 `std::deque<std::vector<uint8_t>>` 环形缓冲区存储历史状态，最多600帧（约10秒），每帧序列化+存储
- **帧率漂移**：原来使用 `frameStart = Clock::now()` 每帧都以实际时钟重置，导致 `sleep_for` 超时误差积累。改用 `nextFrameTarget += frameDurNs` 累加模式，将超时误差限制在单帧内
- **动画生命周期**：`brls::Animatable` 作为成员变量的堆分配对象在 `endCallback` 中使用 `delete this` 自删除；通过分析 borealis `time.cpp` 的 `updateTickings()` 实现确认此模式安全

### 变更说明

| 文件 | 变更内容 |
|------|---------|
| `src/ui/utils/GameView.hpp` | 新增常量（REWIND_BUFFER_SIZE/REWIND_STEP/FF_MULTIPLIER）、倒带缓冲区成员、8个辅助方法声明 |
| `src/ui/utils/GameView.cpp` | 重构游戏主循环：提取 8 个辅助方法；实现倒带/快进帧逻辑；改用 `nextFrameTarget` 累加帧率控制 |
| `src/ui/utils/AnimationHelper.hpp` | 新建动画辅助类，提供 `fadeIn/fadeOut/slideInFromBottom/slideOutToBottom/pushActivity/popActivity` |
| `src/ui/utils/AnimationHelper.cpp` | 实现动画方法，使用堆分配的 FadeAnim/SlideAnim 结构体管理 Animatable 生命周期 |
| `src/ui/page/GamePage.cpp` | 引入 AnimationHelper，菜单显隐改为带动画效果（滑入/淡出） |

### 架构说明（更新）

```
游戏线程 _gameLoop（已整理）：
  ├─ 暂停：sleep 16ms，推进计时基准
  ├─ 倒带模式（_stepRewind）：
  │   ├─ 从 m_rewindBuffer 弹出 REWIND_STEP 帧
  │   └─ Unserialize 状态 + RunFrame 刷新视频
  ├─ 快进/正常模式（_stepFrame）：
  │   ├─ _saveRewindState：序列化并存入环形缓冲区
  │   └─ RunFrame × FF_MULTIPLIER（快进）或 × 1（正常）
  ├─ _captureVideoFrame：取出视频帧，mutex 传 UI 线程
  ├─ _pushFrameAudio：推送音频（快进时限量）
  ├─ _updateFpsStats：每秒更新 FPS 显示
  ├─ _updatePlayTime：每3分钟保存游戏时长
  └─ _throttleFrameRate：
      ├─ nextTarget += frameDurNs（累加目标时间）
      ├─ 若 nextTarget < now → 重置（帧超时防护）
      ├─ sleep_for(coarse)：粗粒度睡眠
      └─ spin wait：精确等待至 nextTarget

AnimationHelper（新建）：
  ├─ fadeIn(view)：setVisibility(VISIBLE) + alpha 0→1
  ├─ fadeOut(view)：alpha 1→0，完成后可设 GONE
  ├─ slideInFromBottom(view)：translateY distance→0
  ├─ slideOutToBottom(view)：translateY 0→distance
  ├─ pushActivity(activity, transition)：封装 brls 推入
  └─ popActivity(transition, cb)：封装 brls 弹出
```


### 任务分析

**任务目标**：在现有 GameView 框架基础上，完善以下七项功能：
1. 完整按键绑定（EMU_A~EMU_SELECT 映射到 brls 系统按键）
2. 画面缩放模式（整数倍、自由缩放、FILL、比例）
3. 游戏运行时长自动记录（每3分钟保存）
4. 状态覆盖层绘制（FPS/快进/倒带/暂停/静音提示）
5. 初步 GameMenuView（返回游戏/退出游戏）
6. EMU_OPEN_MENU 热键打开 GameMenuView
7. onFocusGained/Lost 控制游戏暂停/继续

**输入**：现有 `src/` 代码架构、`old/src/Game/game_view.cpp` 参考实现

**输出**：
- 完善的 `GameView` 按键绑定与游戏循环
- `ScreenMode` 枚举和 `computeDisplayRect()` 工具函数
- `GameOverlayRenderer` 覆盖层绘制工具类
- 功能完整的 `GameMenuView`

**挑战**：
- 游戏按键需 HOLD+RELEASE 双注册以正确维护按下/松开状态
- 跨线程安全：使用 GameSignal 原子位掩码传递按键状态
- 菜单与游戏视图的焦点切换需配合 onFocusGained/Lost 实现暂停控制

### 变更说明

| 文件 | 变更内容 |
|------|---------|
| `src/core/enums.h` | 新增 `ScreenMode` 枚举、`DisplayRect` 结构体、`computeDisplayRect()` 内联函数 |
| `src/core/GameSignal.hpp` | 新增游戏按键位掩码（`m_gameButtonMask`）及操作方法 |
| `src/game/mgba/GameRun.hpp` | 新增 `SetButtonsFromSignal()` 批量更新按键状态 |
| `src/ui/utils/GameOverlayRenderer.hpp` | 新增通用覆盖层绘制工具类（FPS/快进/倒带/暂停/静音） |
| `src/ui/utils/GameView.hpp` | 新增 `m_screenMode`、FPS 统计、`m_gameMenuView` 成员及 `_drawOverlays()` |
| `src/ui/utils/GameView.cpp` | 完善按键绑定、游戏循环（计时+FPS）、draw（画面模式+覆盖层+菜单打开） |
| `src/ui/utils/GameMenuView.hpp` | 完善类定义，添加按钮成员和回调接口 |
| `src/ui/utils/GameMenuView.cpp` | 实现两按钮菜单（返回游戏/退出游戏）含半透明覆盖层 |
| `src/ui/page/GamePage.cpp` | 启用 GameMenuInitialize，注入回调，关联 GameView 与 GameMenuView |


---

## 任务四：移植渲染链及 GLSL/GLSLP 解析代码

### 任务分析

#### 目标
将 `old/src/Video/` 目录下的渲染链代码（`RenderChain`）和 GLSL/GLSLP 解析代码移植到 `src/game/render/` 目录，并集成到 `GameRenderer` 与 `GameView` 中，使游戏运行时支持 RetroArch 风格的多通道着色器预设。

#### 输入
- `old/src/Video/renderer/FullscreenQuad.hpp/cpp` - 全屏四边形渲染辅助类
- `old/src/Video/renderer/GLSLPParser.hpp/cpp` - .glslp 预设和 .glsl 文件解析器
- `old/src/Video/renderer/ShaderCompiler.hpp/cpp` - RetroArch 风格 GLSL 着色器编译器
- `old/src/Video/renderer/RetroShaderPipeline.hpp/cpp` - 多通道着色器管线
- `old/src/Video/RenderChain.hpp/cpp` - 渲染链（管线+直接渲染器整合）
- `src/game/render/GameRenderer.hpp/cpp` - 现有渲染器（需升级）
- `src/ui/utils/GameView.cpp` - 现有游戏视图（需传入着色器路径）

#### 输出
- `src/game/render/FullscreenQuad.hpp/cpp` - 全屏四边形渲染辅助类
- `src/game/render/GLSLPParser.hpp/cpp` - .glslp/.glsl 解析器及相关结构体
- `src/game/render/ShaderCompiler.hpp/cpp` - RetroArch GLSL 着色器编译器
- `src/game/render/RetroShaderPipeline.hpp/cpp` - 多通道着色器管线
- `src/game/render/RenderChain.hpp/cpp` - 渲染链
- 更新后的 `src/game/render/GameRenderer.hpp/cpp` - 集成 RenderChain
- 更新后的 `src/ui/utils/GameView.cpp` - 传入 GameEntry.shaderPath 初始化渲染器

#### 挑战与解决方案
- **Include 路径变更**：旧代码使用 `"Video/renderer/XXX.hpp"` 风格路径，移植时统一改为 `"game/render/XXX.hpp"`
- **stb_image 依赖**：通过 borealis 的 nanovg include 目录自动解析，可直接使用 `#include "stb_image.h"`
- **GameRenderer 接口升级**：用 `RenderChain`（含着色器管线和直接渲染器）替换原有 `DirectQuadRenderer`，新增 `shaderPath` 初始化参数和着色器管理接口
- **视口尺寸传递**：`drawToScreen()` 中将 `virtW * windowScale` 和 `virtH * windowScale` 作为视口物理尺寸传给 `RenderChain::run()`，供 viewport 缩放类型的着色器通道使用

### 变更说明

| 文件 | 说明 |
|------|------|
| `src/game/render/FullscreenQuad.hpp/cpp` | 新建：全屏四边形 VAO/VBO，供 RetroArch 着色器通道使用 |
| `src/game/render/GLSLPParser.hpp/cpp` | 新建：.glslp 预设解析器（支持多通道、外部纹理、参数覆盖）；定义 ShaderPassDesc/ShaderParamInfo 等结构体 |
| `src/game/render/ShaderCompiler.hpp/cpp` | 新建：合并 .glsl 文件编译器，自动注入 #version/VERTEX/FRAGMENT 宏，兼容 GL3/GLES3/GL2/GLES2 |
| `src/game/render/RetroShaderPipeline.hpp/cpp` | 新建：多通道着色器管线，支持外部纹理加载、#pragma parameter 解析、OrigInputSize 等 RetroArch 标准 uniform |
| `src/game/render/RenderChain.hpp/cpp` | 新建：渲染链，整合 RetroShaderPipeline 和 DirectQuadRenderer，提供统一的 run()/drawToScreen() 接口 |
| `src/game/render/GameRenderer.hpp` | 更新：将 DirectQuadRenderer 替换为 RenderChain；新增 setShader/hasShader/getShaderParams/setShaderParam 接口 |
| `src/game/render/GameRenderer.cpp` | 更新：init() 新增 shaderPath 参数；drawToScreen() 改为通过渲染链处理帧后绘制 |
| `src/ui/utils/GameView.cpp` | 更新：初始化渲染器时根据 GameEntry.shaderEnabled 和 shaderPath 传入着色器路径 |

### 渲染链架构

```
游戏帧 GPU 纹理 (m_texture)
        │
        ▼
RenderChain::run()
  ├─ [有着色器] RetroShaderPipeline::process()
  │     ├─ Pass0：绑定 FBO0，运行着色器0，输出纹理A
  │     ├─ Pass1：绑定 FBO1，运行着色器1，输出纹理B
  │     └─ ...  → 最终输出纹理
  └─ [无着色器] 直通，返回原始纹理
        │
        ▼
RenderChain::drawToScreen()
  └─ DirectQuadRenderer::render()  → OpenGL 直接绘制到屏幕
```

---

## 任务二：移植设置界面和关于界面

### 任务分析

#### 目标
将 `old/` 目录下的 `SettingPage`（设置界面）和 `AboutPage`（关于界面）移植到 `src/` 目录，代码风格与现有 `src` 代码一致。

#### 输入
- `old/include/UI/Pages/SettingPage.hpp` / `old/src/UI/Pages/SettingPage.cpp` - 旧版设置页面
- `old/include/UI/Pages/AboutPage.hpp` / `old/src/UI/Pages/AboutPage.cpp` - 旧版关于页面

#### 输出
- `src/ui/page/SettingPage.hpp/.cpp` - 新版设置页面
- `src/ui/page/AboutPage.hpp/.cpp` - 新版关于页面
- `src/core/constexpr.h` - 补充缺失的配置键常量
- `src/ui/page/StartPage.hpp/.cpp` - 集成新页面

#### 挑战与解决方案
- **基类变更**：`beiklive::UI::BBox` → `beiklive::Box`，使用 `getContentBox()` 添加内容
- **配置API差异**：旧版使用 `cfgGetBool/cfgSetBool` 自由函数，新版使用 `GET_SETTING_KEY_*` 宏；在文件内定义本地辅助函数进行适配
- **FileListPage接口差异**：旧版使用 `setFilter`/`setDefaultFileCallback`，新版使用 `setFliter`/`onFileSelected`；封装 `openFilePicker` 通用辅助函数
- **InputMappingConfig依赖**：简化为直接使用配置键字符串，避免引入旧版复杂依赖
- **缺失配置键**：将旧版 `common.hpp` 中的配置键常量（背景图、XMB、音频、调试）添加到 `src/core/constexpr.h`

### 变更说明

| 文件 | 变更内容 |
|------|---------|
| `src/core/constexpr.h` | 添加 UI背景、XMB、缩略图、音频、调试等配置键常量 |
| `src/ui/page/AboutPage.hpp` | 新增关于页面类定义，继承 `beiklive::Box` |
| `src/ui/page/AboutPage.cpp` | 实现关于页面：页头、项目描述文字、作者信息（头像+链接） |
| `src/ui/page/SettingPage.hpp` | 新增设置页面类定义，使用 TabFrame 多标签页 |
| `src/ui/page/SettingPage.cpp` | 实现6个标签页：界面/游戏/画面/音频/按键绑定/调试 |
| `src/ui/page/StartPage.hpp` | 新增对 SettingPage/AboutPage 的引用和私有方法 |
| `src/ui/page/StartPage.cpp` | 补充 `_openSettings()` 和 `_openAbout()` 实现，连接 Layout 回调 |

---

## 任务：修复#158画面闪烁 + 系统架构分析

### 任务目标
1. 修复 PR#158（BKAudioPlayer移植）引入的画面频繁闪烁问题
2. 阅读 src 代码，绘制系统架构图，并提出设计遗漏和可优化的地方

---

### 任务分析

#### 问题描述
PR#158 为非Switch平台添加了 `BKAudioPlayer`（UI音效播放器），注册到 borealis 框架后，游戏画面出现频繁闪烁（频繁黑帧/背景色帧）。

#### 根本原因分析（三个并发问题）

**问题1：`GameView::draw()` 中的提前 `return`（最直接原因）**

```cpp
// 原代码：消费信号后直接 return，导致当帧不绘制游戏画面
if (GameSignal::instance().consumeExit()) {
    brls::sync([this](){ brls::Application::popActivity(); });
    return;  // ← 这帧不绘制游戏，出现黑帧！
}
if (GameSignal::instance().consumeOpenMenu()) {
    brls::sync([this](){ /* 打开菜单 */ });
    return;  // ← 这帧不绘制游戏，出现黑帧！
}
```

borealis 每帧先调用 `videoContext->clear()` 清除帧缓冲（背景色），再调用所有视图的 `draw()`。若 `draw()` 提前 return，该帧只有背景色，造成闪烁。BKAudioPlayer 引入后，`giveFocus()` 等操作触发的音效回调可能间接触发信号，使此问题更频繁暴露。

**问题2：`BKAudioPlayer::play()` 在UI线程中进行文件 I/O**

```cpp
// 原代码：首次播放时在UI线程（draw调用链中）同步加载WAV文件
if (!m_sounds[idx].loaded)
    load(sound);  // ← 在UI渲染线程中执行文件 I/O！
```

文件 I/O 阻塞渲染线程（每个WAV文件约1-5ms），导致帧时间超过16ms（60fps），帧率下降、画面抖动。

**问题3：`DirectQuadRenderer::render()` GL纹理单元恢复逻辑错误**

```cpp
// 原代码：prevTex 保存的是 prevActive 单元的纹理，但恢复时绑定到了 GL_TEXTURE0
glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);  // 保存当前活跃单元的纹理
// ...渲染时使用 GL_TEXTURE0...
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, prevTex);  // 错误！prevTex 可能来自其他单元
```

当 borealis/NanoVG 的活跃纹理单元不是 GL_TEXTURE0 时，GL_TEXTURE0 的原始绑定被错误恢复，导致 NanoVG 后续渲染出现纹理混乱，在某些情况下表现为画面闪烁。

---

#### 修复方案

| 修复项 | 文件 | 修复内容 |
|--------|------|---------|
| 消除信号处理提前返回 | `src/ui/utils/GameView.cpp` | 消费信号后不再 `return`，继续绘制当帧游戏画面 |
| 音效加载移至后台线程 | `src/ui/audio/BKAudioPlayer.cpp` | `play()` 只队列信号，`playbackThread` 负责按需加载和播放 |
| 修复GL纹理状态恢复 | `src/game/render/DirectQuadRenderer.cpp` | 明确保存/恢复 GL_TEXTURE0 的纹理绑定，正确处理活跃纹理单元 |

---

### 系统架构图

已生成 draw.io 格式架构图：`report/system_architecture.drawio`

#### 架构层次说明

```
入口层:   main.cpp
   ↓
UI 层:    MyActivity → StartPage → GamePage → GameView/GameMenuView
                                 → SettingPage / AboutPage
   ↓
游戏层:   GameInputManager | GameSignal | AudioManager | GameTimer
          GameRenderer → RenderChain → DirectQuadRenderer / RetroShaderPipeline
          GameTexture / FrameUploader
   ↓
核心层:   CoreMgba → LibretroLoader → third_party/mgba
          ConfigManager / GameDB | third_party/borealis
```

---

### 设计遗漏与可优化项

#### 1. 单帧缓冲区设计（`m_pendingFrame`）

**现状**：游戏线程与UI线程之间只有一个 `m_pendingFrame`（单缓冲）。游戏线程新帧覆盖旧帧时，UI线程可能在当帧没有可用的新数据，仍显示旧帧。

**建议**：改为双缓冲（两个 `VideoFrame` 槽），游戏线程写入空闲槽，UI线程从就绪槽读取，减少等待时间。

#### 2. `GameView` 职责过重

**现状**：`GameView` 同时负责：游戏线程管理、帧缓冲传递、渲染器初始化、输入处理、信号消费、覆盖层绘制。违反单一职责原则，代码超过 400 行。

**建议**：抽出 `GameSession` 类专门管理游戏线程和帧缓冲，`GameView` 只负责渲染调用和信号响应。

#### 3. `AudioManager` 的 `pushSamples()` 会阻塞游戏线程

**现状**：音频缓冲区满时，`pushSamples()` 调用 `m_spaceCV.wait()` 阻塞游戏线程，造成帧率不稳定。

**建议**：改为非阻塞写入（丢弃最旧的音频帧），或适当增大缓冲区，减少等待概率。

#### 4. `BKAudioPlayer` 每次播放重新打开 ALSA/CoreAudio 设备

**现状**（Linux ALSA）：每次调用 `playSoundDirect()` 都执行 `snd_pcm_open → 配置 → 写入 → drain → close`，效率低，且与 `AudioManager` 共用 `"default"` 设备可能产生竞争。

**建议**：持久持有 PCM 句柄，仅在配置变化时重新初始化；使用独立的 ALSA 设备名（如 `"plug:default"`）避免冲突。

#### 5. 缺少着色器热切换和出错恢复机制

**现状**：`RenderChain::setShader()` 若加载失败，直接打印日志返回，渲染链可能处于部分初始化状态。

**建议**：失败时回退至直通模式（fallback），确保游戏画面始终可见。

#### 6. 缺少游戏 ROM 校验和错误提示

**现状**：`LibretroLoader::loadRom()` 失败时仅返回 false，`CoreMgba::SetupGame()` 不产生用户可见的错误提示。

**建议**：向 `GamePage` 传递失败原因，弹出 borealis notify 告知用户。

#### 7. `GameDB` 缺少事务支持

**现状**：`GamePage` 在初始化和游戏结束时各调用一次 `db->upsert()` + `db->flush()`，若进程崩溃则数据可能丢失。

**建议**：使用 SQLite（或原子文件写入）代替纯 JSON 存储，确保数据原子性。

#### 8. 画面模式（ScreenMode）和自定义缩放未与着色器联动

**现状**：`computeDisplayRect()` 计算的显示矩形传给 `drawToScreen()`，但着色器管线的 viewport 缩放类型（`source`/`viewport`）是独立计算的。二者可能不一致。

**建议**：在 `RenderChain::drawToScreen()` 内部统一计算 viewport，避免二次计算偏差。

---

### 变更文件列表

| 文件 | 变更内容 |
|------|---------|
| `src/ui/utils/GameView.cpp` | 移除 consumeExit/consumeOpenMenu 的提前 return，避免当帧跳过游戏画面绘制 |
| `src/ui/audio/BKAudioPlayer.cpp` | play() 不再在UI线程加载音效；由 playbackThread 后台按需加载后再播放 |
| `src/game/render/DirectQuadRenderer.cpp` | 修复 GL_TEXTURE0 纹理状态保存/恢复逻辑，防止恢复到错误的纹理单元 |
| `report/system_architecture.drawio` | 新增系统架构图（drawio格式） |
| `report/work_report.md` | 更新工作汇报，记录问题分析、修复方案、设计建议 |

---

## 任务：GridItem控件、存档功能、GameMenuView完善、游戏库页面、文件列表优化

### 任务目标
1. 制作 GridItem 通用网格控件（支持 GAME_LIBRARY 和 SAVE_STATE 两种模式）
2. 移植保存/读取状态功能到 GameView
3. 完善 GameMenuView 的 6 个菜单按钮
4. 制作游戏菜单的保存/读取状态页面
5. 制作游戏库页面 GameLibraryPage
6. 文件列表排序优化

### 任务分析

#### 输入
- `old/src/Game/game_view.cpp` - 旧版存档路径计算和存取逻辑
- `old/src/UI/Utils/GameMenu.cpp` - 旧版菜单按钮创建逻辑
- `src/ui/utils/GameMenuView.*` - 现有菜单视图骨架
- `src/ui/utils/GridBox.*` - 现有 GridBox 控件
- `src/core/GameSignal.hpp` - GameSignal 存档信号接口

#### 输出
- `src/ui/utils/GridItem.hpp/cpp` - 新建网格子项控件
- `src/ui/utils/GameView.hpp/cpp` - 新增存档路径计算和存取方法
- `src/ui/utils/GameMenuView.hpp/cpp` - 完善的 6 按钮菜单 + 存档面板
- `src/ui/page/GamePage.cpp` - 注入存档回调和槽位信息回调
- `src/ui/page/GameLibraryPage.hpp/cpp` - 新建游戏库页面
- `src/ui/page/FileListPage.cpp` - 文件排序优化

#### 关键设计决策

**GridItem 设计**
- 两种模式通过 `GridItemMode` 枚举区分
- SAVE_STATE 模式：隐藏徽标框和第三行；空状态居中显示槽位名
- GAME_LIBRARY 模式：显示平台徽标（GBA紫/GBC蓝/GB绿）、游戏名、上次游玩时间、游玩时长
- 所有标签使用 `setSingleLine(true) + setAnimated(true)` 实现单行滚动

**存档路径规则**（与旧版一致）
- 文件名：`{romStem}.ss{slot}`（slot=0 为自动存档）
- 缩略图：`{statePath}.png`
- 目录：`GameEntry.savePath` 非空时使用，否则用全局 saves 目录

**GameMenuView 按钮逻辑**
- `_createMenuButton(text, onClick, sonPanel)` 统一创建
- 有 sonPanel：focus 时调用 `_hideAllPanels()` 再显示 sonPanel 并设为可 focus；click 时 `giveFocus(sonPanel)`
- 无 sonPanel：focus 时 `_hideAllPanels()`；click 时执行 onClick 回调
- 保存/读取状态面板：focus 时额外触发 `_refreshStatePanel(isSave)` 异步扫描

**文件列表排序**
- 分别收集目录和文件到两个 vector
- 按名称不区分大小写排序后，目录在前、文件在后写入结果
- 使用 RawEntry 临时结构体减少字符串拷贝

#### 挑战与解决方案
- **GridBox/LazyCell 焦点剥夺**：GridItem 内部的 onItemClicked 由 LazyCell 的 `onClicked` 通过 GridBox 的 `onItemClicked` 路由触发，不依赖 GridItem 自身的 focusable 属性
- **Borealis Dropdown API**：borealis 的 Dropdown 不是静态方法，需要 `new brls::Dropdown(...)` + `pushActivity`
- **ASYNC_RETAIN/RELEASE 跨帧异步**：使用 borealis 提供的 ASYNC_RETAIN/RELEASE 宏确保异步操作期间 View 不被销毁
- **预存在编译错误**：修复了 `GameInputManager.cpp` 中 `std::powf` 不存在的问题（改为 `std::pow`）

### 结果
所有 6 个子任务均已完成，构建成功（`[100%] Built target GBAStation`）。

---

## 任务二：多Bug修复与功能增强（2026-04-03）

### 任务分析

#### 目标
修复7个已知问题并实现相关功能增强。

#### 输入
- `src/ui/utils/GridBox.cpp` - GridBox布局实现
- `src/ui/page/GameLibraryPage.cpp` - 游戏库页面
- `src/core/Tools.cpp` - 工具函数
- `src/core/enums.h` - 枚举定义
- `src/game/control/GameInputManager.*` - 输入管理器
- `src/ui/utils/GameView.cpp` - 游戏视图
- `src/ui/page/SettingPage.cpp` - 设置页面
- `src/core/common.cpp` - 通用初始化
- `src/ui/page/GamePage.cpp` - 游戏页面

#### 可能的挑战与解决方案
- **GridBox占位符方案**：最后一行不满时需要添加等宽透明占位符，避免最后一个item拉伸
- **摇杆方向区分**：processStick原来不区分轴方向，需要修改签名传入4个方向ID
- **路径初始化时机**：GamePage有两种构造路径（DirListData和GameEntry），都需要初始化路径

### 完成工作

#### 1. GridBox最后一行拉伸Bug修复
**文件**: `src/ui/utils/GridBox.cpp`
- 在 `rebuild()` 中，当最后一行不满列数时，为剩余位置添加透明的占位Box
- 每个 LazyCell 设置 `setGrow(1.0f)` 确保等宽分配

#### 2. Y键排序后焦点崩溃Bug修复
**文件**: `src/ui/page/GameLibraryPage.cpp`
- 在 `_showSortDropdown()` 的排序回调中，`_rebuildGrid()` 后添加 `brls::Application::giveFocus(m_grid)`

#### 3. 日期格式和游玩时间修复
**文件**: `src/core/Tools.cpp`, `src/ui/page/GameLibraryPage.cpp`
- `getTimestampString()` 格式改为 `%y-%m-%d %H时%M分`（不含秒）
- `_formatPlayTime()` 去掉秒的显示，不足1分钟显示"不到1分钟"

#### 4. EmuFunctionKey增加左右摇杆映射
**文件**: `src/core/enums.h`, `src/game/control/GameInputManager.hpp/.cpp`, `src/ui/utils/GameView.cpp`
- `GameInputPad` 枚举添加8个摇杆方向值（UP/DOWN/LEFT/RIGHT × 左右摇杆）
- `processStick()` 修改签名为接受4个方向参数，区分正负方向推入不同枚举值
- `EmuFunctionKey` 枚举添加 `EMU_LEFT_STICK_*` 和 `EMU_RIGHT_STICK_*`（共8个）
- `GameView::_registerGameInput()` 注册摇杆方向映射为RETRO方向键（UP/DOWN/LEFT/RIGHT）

#### 5. SettingPage按键映射添加摇杆
**文件**: `src/ui/page/SettingPage.cpp`
- `k_gameBtns` 数组添加左右摇杆的8个方向映射项

#### 6. ConfigureInit预设所有默认值
**文件**: `src/core/common.cpp`
- 使用 `SettingManager->SetDefault()` 为所有设置项预设默认值
- 涵盖UI、遮罩、着色器、调试、快进、倒带、核心、画面、存档、截图、金手指等所有设置分组

#### 7. GameEntryInitialize路径初始化
**文件**: `src/ui/page/GamePage.cpp/.hpp`
- 新增 `_initGameEntryPaths()` 方法，统一初始化5个路径字段：
  - `savePath`：从 `save.sramDir` 设置读取，为空时使用全局saves目录
  - `cheatPath`：从 `cheat.dir` 设置读取，构建为 `<目录>/<游戏名>.cht`（使用std::filesystem::path）
  - `overlayPath`：根据平台从对应KEY_DISPLAY_OVERLAY_*读取
  - `logoPath`：使用平台默认封面图标
  - `screenShotPath`：使用全局screenshots目录
- 两种GamePage构造方式（DirListData和GameEntry）都调用此方法

---

## 2026-04-03 修复PR#162：旧代码混入src的逻辑混乱问题

### 任务分析

**任务目标**：PR #162在旧代码路径（`old/` 风格：`src/Control/`、`src/Game/`、`src/UI/Pages/`）实现了功能，而非新代码路径（`src/ui/`、`src/game/`、`src/core/`），导致新代码中的设置界面和游戏输入映射存在逻辑问题：设置保存后不生效、硬编码按键无法更改、摇杆斜向输入不支持配置。

**输入**：新代码中GameView的`_registerGameInput()`硬编码按键，SettingPage的A键覆盖而非追加，无摇杆设置UI，processStick不支持斜向模式。

**输出**：按键映射从配置动态读取、A键追加combo、X键清空、新增摇杆开关与斜向开关、`processStick`支持斜向模式配置。

### 实现内容

#### 1. 按键字符串解析工具（Tools.hpp / Tools.cpp）
- 新增 `parsePadCombo(string)` → `vector<int>`：将"LB+START"解析为按键ID列表（大小写不敏感）
- 新增 `parseMultiCombo(string)` → `vector<vector<int>>`：将"A,LB+A"解析为多组combo（逗号分隔）
- 利用 `k_gameInputNames` 实现名称→ID映射

#### 2. 配置默认值（common.cpp）
- 新增 `handle.a/b/x/y/up/down/left/right/l/r/l2/r2/l3/r3/start/select` 默认值
- 新增摇杆方向键默认值（lstick_*/rstick_*）
- 新增功能热键默认值（fastforward/rewind/hotkey.menu.pad等）
- 新增 `input.joystick.enabled=1`、`input.joystick.diagonal=1` 默认值

#### 3. GameInputManager斜向模式（GameInputManager.hpp / .cpp）
- 新增 `m_diagonalMode` 成员变量（默认true）
- 新增 `setDiagonalMode(bool)` 公开接口
- `processStick()` 根据 `m_diagonalMode` 决定是否同时激活X和Y轴

#### 4. GameView从配置读取按键映射（GameView.cpp）
- 移除硬编码的 `gameBtnMaps[]` 和 `stickBtnMaps[]`
- `_registerGameInput()` 从 `handle.xxx` 配置动态读取多combo按键，注册HOLD/RELEASE回调
- 摇杆映射根据 `input.joystick.enabled` 开关决定是否注册
- 功能热键（menu/fastforward/rewind/quicksave/quickload/mute）从配置读取多combo

#### 5. SettingPage按键UI重制（SettingPage.cpp）
- 添加 `<sstream>` include
- `buildKeyBindTab()`：重构为局部辅助lambda `registerKeyBindActions`：A键追加combo（逗号分隔，去重），X键清空
- 新增"功能热键绑定"分区标题（原"热键绑定（手柄）"）
- 新增"摇杆设置"分区：启用左摇杆输入开关、允许斜向输入开关

---

## 任务三：BKAudioPlayer Switch 支持 + SettingPage 清理（2026-04-03）

### 任务分析

#### 目标
1. 为 `BKAudioPlayer` 添加 Nintendo Switch 平台支持，接管 borealis 原有 `SwitchAudioPlayer` 的音效
2. 移除 `SettingPage` 中尚未实际生效的设置项
3. 整理所有设置项状态到 `report/sets.md`

#### 挑战与解决方案
- **Switch 音频接管**：需要使用 libpulsar API 加载 qlaunch BFSAR 音效库，与非Switch平台的WAV加载路径完全不同
- **C++ goto 跨初始化跳转**：将 Switch 初始化逻辑提取为单独的 `_initSwitch()` 私有方法，避免 goto 问题
- **`playSoundDirect` 签名变更**：添加 `soundIdx` 参数供 Switch 平台查找 pulsar 句柄，非Switch平台忽略此参数
- **设置项生效判断**：通过代码搜索确认每个配置键在 SettingPage 之外是否被实际读取

### 实现内容

#### 1. BKAudioPlayer Switch 平台支持（BKAudioPlayer.hpp / .cpp）
- 头文件新增 `#ifdef __SWITCH__` 段落，引入 `pulsar.h` 并声明 Switch 专属成员
  - `m_switchInit`：初始化成功标志
  - `m_qlaunchBfsar`：qlaunch BFSAR 句柄
  - `m_switchSounds[]`：各音效的 `PLSR_PlayerSoundId` 数组
  - `_initSwitch()`：私有初始化方法
- 新增 Switch 音效名称映射表 `SWITCH_SOUND_NAMES[]`，与 borealis `SwitchAudioPlayer` 保持一致
- `BKAudioPlayer()` 构造函数中对 Switch 调用 `_initSwitch()`，挂载 qlaunch ROMFS 并打开 BFSAR
- `~BKAudioPlayer()` 析构函数中释放所有音效句柄并调用 `plsrPlayerExit()`
- `load()` 方法新增 Switch 分支：通过 `plsrPlayerLoadSoundByName()` 加载官方音效
- `playSoundDirect()` 签名增加 `int soundIdx` 参数，Switch 分支调用 `plsrPlayerSetPitch()` + `plsrPlayerPlay()`
- 平台判断顺序改为：`__SWITCH__` → `BK_AUDIO_ALSA` → `BK_AUDIO_WINMM` → `BK_AUDIO_COREAUDIO` → 空实现

#### 2. SettingPage 清理（SettingPage.cpp）
- **界面Tab** 移除：背景图片节、XMB风格背景节、自动存档/自动读档/即时存档目录/存档截图缩略图、截图目录、金手指启用开关
- **游戏Tab** 移除：整个快进节（enabled/mode/multiplier）、整个倒带节（enabled/mode/bufferSize/step）
- **画面Tab** 移除：显示模式/整数倍缩放/纹理过滤、状态显示节（showFps/showFfOverlay/showRewindOverlay/showMuteOverlay）、遮罩启用开关、整个着色器节
- **音频Tab** 移除：快进时静音、倒带时静音（低通滤波器保留）
- **按键Tab** 移除：暂停热键（`hotkey.pause.pad`）、截屏热键（`hotkey.screenshot.pad`）
- 移除无用的局部常量（`ffRateVals`、`k_bufSizeInts`、`k_xmbColorIds/Labels/Count`）
- 移除无用的辅助函数 `cfgGetFloat()`、`cfgGetInt()`
- 移除无用的 `#include <algorithm>` 和 `#include <cmath>`

#### 3. 设置项整理文档（report/sets.md）
- 整理了所有已生效设置项（含读取位置）
- 整理了所有已移除无效设置项（含移除原因）
- 整理了待新增设置项（含涉及修改位置）
- 整理了 BKAudioPlayer 各平台实现状态和 Switch 音效名称映射
