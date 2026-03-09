# BeikLiveStation 问题修复报告

## 修复概述

本次修复针对项目中发现的四个主要问题，对以下文件进行了改动：

- `include/Game/LibretroLoader.hpp`
- `src/Game/LibretroLoader.cpp`
- `include/UI/game_view.hpp`
- `src/UI/game_view.cpp`
- `resources/i18n/zh-Hans/beiklive.json`
- `resources/i18n/en-US/beiklive.json` (新增)

---

## 问题一：游戏帧率被 Borealis 绘制循环限制

### 问题描述

原来的实现将 `retro_run()` 放在 `GameView::draw()` 中执行，导致模拟器帧率完全受 Borealis 渲染帧率的限制。Borealis 主循环还会执行 UI 布局、动画等操作，可能降低实际帧率。

### 修复方案

在 `GameView` 中引入独立的游戏线程：

1. **新增成员变量**（`game_view.hpp`）：
   - `std::thread m_gameThread` — 游戏模拟线程
   - `std::atomic<bool> m_running{false}` — 线程运行标志

2. **新增方法**：
   - `startGameThread()` — 在 `initialize()` 完成后启动线程
   - `stopGameThread()` — 在 `cleanup()` 或退出时停止并等待线程结束

3. **职责分离**：
   - 游戏线程：负责 `pollInput()` + `m_core.run()` + 音频 drain
   - 绘制线程（`draw()`）：仅负责从 `LibretroLoader` 取得最新帧数据、上传到 GL 纹理、用 NanoVG 渲染

---

## 问题二：无法退出游戏回到主界面

### 问题描述

进入游戏后，所有按键均被 `beiklive::swallow()` 吞掉或映射给模拟器，导致没有任何方式能退出游戏界面返回主界面。

### 修复方案

将 **BUTTON_X** 设置为退出键：

1. 从按键映射表 `k_buttonMap` 中**移除** `BUTTON_X → RETRO_DEVICE_ID_JOYPAD_X` 条目。
2. 在 `GameView` 构造函数中，**不再** `swallow(BUTTON_X)`，而是通过 `registerAction()` 注册退出动作：

```cpp
registerAction("beiklive/hints/exit_game"_i18n, brls::BUTTON_X,
    [this](brls::View*) {
        bklog::info("GameView: exit requested via BUTTON_X");
        stopGameThread();
        brls::Application::popActivity();
        return true;
    }, false, false, brls::SOUND_CLICK);
```

3. 新增 i18n 条目 `"exit_game"` 到 `zh-Hans/beiklive.json` 和 `en-US/beiklive.json`。

> **注意**：GBA 本身没有 X 键，移除该映射不影响游戏可玩性。

---

## 问题三：缺少帧率控制器

### 问题描述

原实现无帧率控制，模拟器运行速度取决于硬件性能，可能导致游戏加速或受 Borealis 帧率约束。

### 修复方案

在游戏线程中使用 `std::chrono::steady_clock` 实现精确帧率控制：

```cpp
double fps = m_core.fps();          // 从核心获取目标帧率（GBA ≈ 59.73 fps）
if (fps <= 0.0 || fps > 240.0) fps = 60.0;
const Duration frameDuration(1.0 / fps);

while (m_running) {
    auto frameStart = Clock::now();

    pollInput();
    m_core.run();
    drainAudio();

    // 帧率控制：休眠剩余时间
    auto elapsed = Clock::now() - frameStart;
    if (elapsed < frameDuration)
        std::this_thread::sleep_for(frameDuration - elapsed);
}
```

目标帧率从 `m_core.fps()` 读取（使用 libretro 核心上报的实际帧率），超出合理范围时回落到 60fps。

---

## 问题四：游戏画面渲染异常

### 问题描述

- 屏幕并排出现两个相同的游戏画面
- 画面被纵向拉伸
- 颜色基本只有白色、绿色和深色

### 根本原因分析

**原因 1：GL_BGRA 在 OpenGL ES 上不兼容**

原代码在上传纹理时使用 `GL_BGRA` 格式：
```cpp
glTexImage2D(..., GL_BGRA, GL_UNSIGNED_BYTE, ...);
```
OpenGL ES 3.0 核心规范不包含 `GL_BGRA`（需要 `GL_EXT_texture_format_BGRA8888` 扩展）。若驱动不支持该扩展，`glTexImage2D` 会产生 `GL_INVALID_ENUM` 错误，纹理数据无法正确上传，导致颜色异常。

**原因 2：像素格式未做通道转换**

libretro XRGB8888 格式在小端内存中为 `[B, G, R, X]`（4 字节），原代码直接拷贝到 GL 纹理但使用 `GL_RGBA` 解读，导致红/蓝通道对调（R↔B）。
结合 Alpha 通道为 0xFF 的 X 字节，白色（R=G=B=255）、绿色（G 不受影响）、深色（RGB 均低）等颜色在通道对调后视觉上改变不大，其他颜色则严重失真。

**原因 3：多个 NANOVG_GL 变体同时定义**

原代码在包含 `nanovg_gl.h` 前定义了所有四个变体（`NANOVG_GL2/GL3/GLES2/GLES3`）。`nanovg_gl.h` 内部使用 `#if ... #elif` 链，导致只有 `NANOVG_GL2` 的实现被编译，可能与 Borealis 实际使用的 GL 后端不匹配。

### 修复方案

**1. 像素格式转换：XRGB8888 → RGBA8888（LibretroLoader）**

- `LibretroLoader` 增加 `m_pixelFormat` 成员，记录核心请求的像素格式
- `s_videoRefreshCallback` 中将像素数据转换为统一的 RGBA8888 格式：
  - XRGB8888：`[B,G,R,X] → [R,G,B,0xFF]`（交换 R/B 通道，设 Alpha=0xFF）
  - RGB565：展开为 RGBA8888
- `VideoFrame::pixels` 语义变为 RGBA8888

**2. 纹理上传改用 GL_RGBA（game_view.cpp）**

将所有 `glTexImage2D` / `glTexSubImage2D` 调用从 `GL_BGRA` 改为 `GL_RGBA`，在 OpenGL 和 OpenGL ES 上均支持。

**3. 修正 NanoVG GL 后端包含方式**

按照参考代码，根据 Borealis 实际使用的 GL 后端选择性定义一个变体：

```cpp
#ifdef BOREALIS_USE_OPENGL
#  ifdef USE_GLES3
#    define NANOVG_GLES3
#  elif defined(USE_GLES2)
#    define NANOVG_GLES2
#  elif defined(USE_GL2)
#    define NANOVG_GL2
#  else
#    define NANOVG_GL3
#  endif
#  include <nanovg/nanovg_gl.h>
#endif
```

---

## 修改文件汇总

| 文件 | 修改内容 |
|------|---------|
| `include/Game/LibretroLoader.hpp` | 添加 `m_pixelFormat` 字段；`VideoFrame::pixels` 语义改为 RGBA8888；移除 `pitch` 字段 |
| `src/Game/LibretroLoader.cpp` | 跟踪核心像素格式；`s_videoRefreshCallback` 中执行 XRGB8888/RGB565→RGBA8888 转换 |
| `include/UI/game_view.hpp` | 添加 `m_gameThread`、`m_running`、`startGameThread()`、`stopGameThread()` |
| `src/UI/game_view.cpp` | 修正 NanoVG 后端检测；BUTTON_X 注册为退出键；游戏逻辑移至独立线程；纹理上传改用 GL_RGBA |
| `resources/i18n/zh-Hans/beiklive.json` | 添加 `hints.exit_game` 中文翻译 |
| `resources/i18n/en-US/beiklive.json` | 新增英文翻译文件（含 `hints.exit_game`） |
