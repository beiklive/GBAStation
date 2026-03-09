# BeikLiveStation
基于 libretro 核心 + borealis UI框架的模拟器


你的任务是：
1. 参考项目根目录的CMakeLists.txt, 编写 extern 目录下的CMakeLists.txt, 用于构建 extern 目录下的borealis库。
2. extern 目录下的CMakeLists.txt 需要对 mGBA库 进行libretro编译
3. 修改根目录的CMakeLists.txt, 将 extern 目录下的CMakeLists.txt 添加到构建过程中
4. 以上所有构建都要兼容 Windows 、 APPLE平台 、Linux平台、Switch平台, 并且需要使用 CMake 进行构建管理。可以参考各仓库的官方文档和示例来实现跨平台构建。

---

## 任务汇报 – libretro 加载、游戏渲染与音频管理器实现

### 概述

本次实现了将 `mgba_libretro` 核心与 borealis/NanoVG 前端对接所需的三个核心模块，并同步完成了构建系统的配套调整。

### 新增文件

| 文件 | 说明 |
|------|------|
| `include/Game/LibretroLoader.hpp` | libretro 动态库加载器接口 |
| `src/Game/LibretroLoader.cpp`     | 跨平台动态加载实现 |
| `include/Audio/AudioManager.hpp`  | 通用音频管理器接口 |
| `src/Audio/AudioManager.cpp`      | 多平台音频后端实现 |

### 修改文件

| 文件 | 变更内容 |
|------|----------|
| `include/UI/game_view.hpp` | 移除 `#ifdef __SWITCH__` 限制，新增 GL 纹理 / NVG 图像成员 |
| `src/UI/game_view.cpp`     | 完整实现：加载核心、运行帧、渲染画面、轮询输入、推送音频 |
| `src/XMLUI/StartPageView.cpp` | 移除 `#ifdef __SWITCH__` 限制，所有平台均可启动游戏 |
| `CMakeLists.txt`           | 新增音频库链接、mGBA 头文件路径、libretro 后处理拷贝步骤 |

---

### 模块详解

#### 1. LibretroLoader（`include/Game/LibretroLoader.hpp` / `src/Game/LibretroLoader.cpp`）

- 使用 `dlopen`/`dlsym`（Linux/macOS/Switch）或 `LoadLibrary`/`GetProcAddress`（Windows）动态加载 `mgba_libretro.so/.dylib/.dll`
- 解析所有必要的 libretro API 函数指针（`retro_init`、`retro_run`、`retro_load_game` 等）
- 注册 5 个静态 C 回调：
  - `video_refresh`：将 XRGB8888 视频帧存入受互斥锁保护的缓冲区
  - `audio_sample_batch`：将立体声 int16_t 样本写入音频环形缓冲区
  - `audio_sample`：单样本包装，同上
  - `input_poll`：空实现（主线程已在 `retro_run()` 前完成状态更新）
  - `input_state`：从按键状态数组查询 RETRO_DEVICE_JOYPAD 按钮状态
- `environment_callback` 处理核心所需的最低限度环境命令（`GET_CAN_DUPE`、`SET_PIXEL_FORMAT`、`GET_SYSTEM_DIRECTORY` 等）

#### 2. AudioManager（`include/Audio/AudioManager.hpp` / `src/Audio/AudioManager.cpp`）

单例 + 线程安全环形缓冲区（容量 ≈ 1 秒 32768 Hz 立体声）。

| 平台 | 音频后端 |
|------|---------|
| Linux   | ALSA (`libasound`) — 阻塞式 `snd_pcm_writei` |
| Windows | WinMM (`waveOut`) — 双缓冲 WAVEHDR 回调 |
| macOS   | CoreAudio (`AudioUnit`) — 拉取渲染回调 |
| Switch  | libnx `audout` — 双缓冲 + 最近邻重采样至 48 kHz |
| 其他    | 静默 fallback（样本丢弃） |

`pushSamples(data, frames)` 可从任意线程安全调用（libretro 音频回调）。

#### 3. GameView（`include/UI/game_view.hpp` / `src/UI/game_view.cpp`）

- **跨平台**：移除 `#ifdef __SWITCH__` 限制，在 Windows / macOS / Linux / Switch 上均可运行
- **延迟初始化**：第一次 `draw()` 时在有效 GL 上下文中完成初始化
- **渲染流程（每帧）**：
  1. `pollInput()` — 将 borealis `ControllerState` 映射为 retro joypad 按键
  2. `m_core.run()` — 执行 `retro_run()` 推进一帧模拟
  3. 排空音频样本 → `AudioManager::pushSamples()`
  4. `uploadFrame()` — 将 XRGB8888 像素上传至 GL 纹理（`GL_BGRA` 格式）
  5. `nvglCreateImageFromHandleGL3/GL2/GLES2/GLES3` — 将 GL 纹理包装为 NanoVG 图像句柄
  6. `nvgImagePattern` + `nvgFillPaint` — 通过 NanoVG 绘制游戏画面
- **DisplayConfig** 支持 `Fit / Fill / Original / IntegerScale / Custom` 五种缩放模式
- **按键映射**（桌面键盘 GLFW 模式）：通过 borealis `ControllerState` 统一处理

#### 4. CMakeLists.txt 变更

- 暴露 `${CMAKE_CURRENT_SOURCE_DIR}` 为 include 路径，使 `LibretroLoader.hpp` 可引用 libretro.h
- 平台自动检测音频后端并链接对应系统库
- 新增 POST_BUILD 步骤：将 `mgba_libretro.so/.dll/.dylib` 拷贝至可执行文件同目录，供运行时相对路径加载

---

### 构建方式

```bash
# Linux
./linuxbuild.sh

# 产物目录 build_linux/
#   BKStation           —— 主程序
#   mgba_libretro.so    —— libretro 核心（自动拷贝至同目录）
```

```bat
rem Windows (MSYS2 MinGW64)
windowsbuild.bat

rem 产物目录 build_windows\
rem   BKStation.exe
rem   mgba_libretro.dll
```

---

### 已知限制 / 后续工作

- 重采样为最近邻算法，Switch 平台 48 kHz 输出质量有限；后续可引入线性插值重采样
- 桌面平台按键映射基于 borealis 抽象层；GLFW 模式下键盘映射需依赖 borealis 的键盘→手柄映射逻辑
- 存档（serialize/unserialize）接口已暴露于 `LibretroLoader`，UI 层尚未集成

