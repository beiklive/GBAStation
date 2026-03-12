# BeikLiveStation 问题修复报告

## 修复概述

本次修复包含两轮改动。

### 第一轮修复

针对项目中发现的四个主要问题，对以下文件进行了改动：

- `include/Game/LibretroLoader.hpp`
- `src/Game/LibretroLoader.cpp`
- `include/UI/game_view.hpp`
- `src/UI/game_view.cpp`
- `resources/i18n/zh-Hans/beiklive.json`
- `resources/i18n/en-US/beiklive.json` (新增)

### 第二轮修复

针对以下五个问题的修复：

1. Release 编译下游戏音频拖慢爆音
2. 根据 mGBA libretro 接口设计 SettingManager
3. DisplayConfig display 相关设置项实际生效
4. 游戏 ROM 读取失败界面提示按 A 键关闭
5. 按 ZR 没有倍速效果

修改文件：
- `src/Audio/AudioManager.cpp`
- `include/Game/LibretroLoader.hpp`
- `src/Game/LibretroLoader.cpp`
- `include/UI/game_view.hpp`
- `src/UI/game_view.cpp`
- `resources/i18n/zh-Hans/beiklive.json`
- `resources/i18n/en-US/beiklive.json`
- `report/settings_report.md` (新增)

---

## 第一轮问题

### 问题一：游戏帧率被 Borealis 绘制循环限制

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

### 问题二：无法退出游戏回到主界面

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

### 问题三：缺少帧率控制器

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

### 问题四：游戏画面渲染异常

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

## 第二轮问题

### 问题一：Release 编译游戏音频拖慢爆音

**根本原因**

Switch 音频线程每次回调读取 `SWITCH_FRAMES = 1024` 帧（32768 Hz）输入后重采样到 48000 Hz 输出 1024 帧。但：

- 每次 `audoutPlayBuffer` 回调消耗 `1024 × 32768 = 32,768 输入样本/秒 × (48000/48000)` ≠
- 实际每次应消耗 `1024 × 32768/48000 ≈ 699` 帧输入

导致每秒从 ring buffer 取走 `1024 × 46.88 = 48000` 个样本，而 GBA 核心仅以 `32768` 个/秒速率写入，ring buffer 持续欠载，持续补零，造成音频断断续续爆音。

**修复方案**（`src/Audio/AudioManager.cpp`）

```cpp
double ratio = (double)m_sampleRate / SWITCH_OUT_RATE;
size_t inputFrames = (size_t)(SWITCH_FRAMES * ratio + 0.5); // ≈ 699
```

每次从 ring buffer 精确读取 `inputFrames` 帧，再通过最近邻插值扩展到 `SWITCH_FRAMES` 帧输出。这使输入消耗速率与核心产出速率精确匹配，消除爆音。

---

### 问题二：SettingManager / 设置项管理

**修改文件**：`LibretroLoader.hpp`、`LibretroLoader.cpp`、`game_view.cpp`

**实现**：

1. `LibretroLoader` 持有 `ConfigManager* m_configManager`，通过 `setConfigManager()` 注入。
2. `RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION` 返回版本 0，迫使 mgba 使用 `RETRO_ENVIRONMENT_SET_VARIABLES`（旧式接口）。
3. `RETRO_ENVIRONMENT_SET_VARIABLES`：解析 `"描述; 默认值|选项..."` 格式，调用 `ConfigManager::SetDefault()` 写入默认值。
4. `RETRO_ENVIRONMENT_GET_VARIABLE`：从 `ConfigManager` 查询键值，若存在则通过持久 `m_coreVarStorage` map 返回 `c_str()` 指针。
5. `GameView::initialize()` 在加载核心前预注册所有已知 mgba 变量的默认值。

详见 `report/settings_report.md`。

---

### 问题三：DisplayConfig 过滤模式实际生效

**修改文件**：`game_view.cpp`、`game_view.hpp`

1. `initialize()` 读取 `m_display.filterMode` 并据此设置 GL 纹理参数（`GL_NEAREST` 或 `GL_LINEAR`），同时保存到 `m_activeFilter`。
2. `nvgImageFromGLTexture()` 接受新的 `FilterMode` 参数，对 `FilterMode::Nearest` 附加 `NVG_IMAGE_NEAREST` 标志，确保 NanoVG 也使用正确的过滤模式。
3. `draw()` 在每帧检测 `m_display.filterMode != m_activeFilter`，若变化则更新 GL 参数并重建 NVG 图像句柄。

---

### 问题四：ROM 加载失败界面提示按 A 键关闭

**修改文件**：`game_view.cpp`、`i18n` 文件

1. 错误界面在错误文字下方增加一行提示（使用 i18n key `beiklive/hints/close_on_a`）。
2. 构造函数将 `beiklive::swallow(this, brls::BUTTON_A)` 改为 `registerAction()`，当 `m_coreFailed == true` 时调用 `brls::Application::popActivity()` 关闭界面。
3. 中英文 i18n 文件均新增 `"close_on_a"` 键。

---

### 问题五：ZR 按键实现倍速效果

**修改文件**：`game_view.hpp`、`game_view.cpp`

1. `game_view.hpp` 新增 `std::atomic<bool> m_fastForward{false}` 和常量 `FAST_FORWARD_MULT = 4`。
2. `pollInput()` 检测 `state.buttons[brls::BUTTON_RT]`（= ZR）并更新 `m_fastForward`。
3. `startGameThread()` 中：按住 ZR 时每次循环运行 `FAST_FORWARD_MULT`（4）帧，跳过帧率限制 sleep，并丢弃当次音频样本（防止 ring buffer 溢出）。

---

## 修改文件汇总（两轮合计）

| 文件 | 修改内容 |
|------|---------|
| `src/Audio/AudioManager.cpp` | 修正 Switch 音频重采样输入帧数计算，消除爆音 |
| `include/Game/LibretroLoader.hpp` | 引入 `ConfigManager.hpp`；添加 `m_configManager`、`m_coreVarStorage`、`setConfigManager()` |
| `src/Game/LibretroLoader.cpp` | 处理 `GET_CORE_OPTIONS_VERSION`、`SET_VARIABLES`、`GET_VARIABLE` 环境回调 |
| `include/UI/game_view.hpp` | 添加 `m_fastForward`、`m_activeFilter`、`FAST_FORWARD_MULT` |
| `src/UI/game_view.cpp` | 注册 mgba 变量默认值；GL 过滤模式应用；NVG 过滤标志；ZR 快进；A 键关闭失败界面；错误界面提示 |
| `resources/i18n/zh-Hans/beiklive.json` | 添加 `hints.close_on_a` |
| `resources/i18n/en-US/beiklive.json` | 添加 `hints.close_on_a` |
| `report/settings_report.md` | 新增：mGBA 及显示设置项完整说明文档 |

