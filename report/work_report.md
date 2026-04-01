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

## 任务二：游戏输入、画面模式、计时、覆盖层、菜单功能完善

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

