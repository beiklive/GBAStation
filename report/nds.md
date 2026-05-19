# BeikLiveStation 接入官方 melonDS 原生核心完整实施步骤

# 最终目标

将：

```text id="j80ud4"
官方 melonDS
```

改造成：

```text id="l3jk4j"
BeikLiveStation 的内嵌 Emulator Core
```

最终结构：

```text id="zz7cqb"
Borealis UI
    ↓
GameView
    ↓
GameRenderer
    ↓
CoreMelonDS
    ↓
Official melonDS Core
```

要求：

* 不使用 libretro
* 不使用 Qt Frontend
* 不使用 SDL Window
* 不创建新的 OpenGL Context
* 不让 melonDS 管理生命周期
* 完全接入现有 OpenGL Renderer
* 最终支持：

  * 软件渲染
  * OpenGL 3D
  * 双屏布局
  * Shader
  * SaveState
  * FastForward

---

# 第一阶段：明确接入原则

---

# 必须理解的核心思想

你的程序：

```text id="yzkp0n"
是宿主（Host）
```

melonDS：

```text id="u0j7tx"
只是 Emulator Backend
```

---

# melonDS 不允许：

```text id="ut5nn8"
创建窗口
创建 OpenGL Context
直接渲染
直接播放音频
读取手柄
管理生命周期
```

---

# BeikLiveStation 负责：

```text id="n6l34v"
UI
Renderer
Audio
Input
线程
生命周期
Shader
Layout
```

---

# 第二阶段：整理目录结构

---

# 删除旧废案

删除：

```text id="szfg4n"
src/game/melonds/PlatformMelonDS.cpp
```

---

# 新目录结构

建立：

```text id="eqo26x"
src/game/melonds/
│
├── CoreMelonDS.hpp
├── CoreMelonDS.cpp
│
├── MelonDSInstance.hpp
├── MelonDSInstance.cpp
│
├── video/
│   ├── MelonDSVideo.hpp
│   ├── MelonDSVideo.cpp
│   ├── FrameComposer.hpp
│   └── FrameComposer.cpp
│
├── audio/
│   ├── MelonDSAudio.hpp
│   └── MelonDSAudio.cpp
│
├── input/
│   ├── MelonDSInput.hpp
│   └── MelonDSInput.cpp
│
├── save/
│   ├── MelonDSSave.hpp
│   └── MelonDSSave.cpp
│
├── config/
│   ├── MelonDSConfig.hpp
│   └── MelonDSConfig.cpp
│
└── platform/
    ├── PlatformSwitch.cpp
    ├── PlatformCommon.cpp
    └── PlatformFilesystem.cpp
```

---

# 第三阶段：整理 third_party/melonDS

---

# 当前状态

你已经：

```text id="v02f4o"
third_party/melonDS
```

放入官方仓库。

---

# 非常重要

不要：

```cmake id="6v5cxp"
add_subdirectory(third_party/melonDS)
```

---

因为官方 CMake 会：

```text id="bjlc8h"
编译 frontend
编译 SDL
编译 Qt
创建窗口
创建 OpenGL Context
接管生命周期
```

---

# 正确做法

新增：

```text id="ulc5mr"
third_party/melonds_core/
```

---

目录：

```text id="vynb8c"
third_party/
├── melonDS/
└── melonds_core/
    ├── CMakeLists.txt
    └── sources.cmake
```

---

# 第四阶段：建立 melonds_core 静态库

---

# third_party/melonds_core/CMakeLists.txt

建立：

```cmake id="j5z8sz"
add_library(melonds_core STATIC ...)
```

---

# 只编译核心

不要编译：

```text id="rphn42"
frontend
qt
android
windows
macos
sdl frontend
```

---

# 只保留：

```text id="f2lg81"
CPU
GPU
SPU
RTC
DMA
NDS
ARM
Savestate
```

---

# 第五阶段：建立 sources.cmake

---

新增：

```text id="l3ecpo"
third_party/melonds_core/sources.cmake
```

---

# 用于维护源码列表

例如：

```cmake id="n6ee0g"
set(MELONDS_CORE_SOURCES
    ${MELONDS_DIR}/src/NDS.cpp
    ${MELONDS_DIR}/src/ARM.cpp
    ${MELONDS_DIR}/src/GPU.cpp
    ${MELONDS_DIR}/src/SPU.cpp
)
```

---

# 初期原则

不要一次性全加。

---

# 第一目标

```text id="1o5njh"
最小可运行核心
```

---

# 第六阶段：接入根 CMake

---

# third_party/CMakeLists.txt

添加：

```cmake id="ep9gln"
add_subdirectory(melonds_core)
```

---

# 不允许

```cmake id="l0wwqz"
add_subdirectory(melonDS)
```

---

# 第七阶段：配置 include path

---

# melonds_core/CMakeLists.txt

```cmake id="2d5lmo"
set(MELONDS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../melonDS)

target_include_directories(melonds_core PUBLIC
    ${MELONDS_DIR}/src
)
```

---

# 第八阶段：建立平台宏

---

```cmake id="5u7z2n"
target_compile_definitions(melonds_core PUBLIC
    __SWITCH__
    MELONDS_EMBEDDED
)
```

---

# 用于：

```text id="h8v4h5"
屏蔽 frontend
屏蔽 SDL
屏蔽 Qt
```

---

# 第九阶段：建立 Emulator 抽象层

---

新增：

```text id="2vg33x"
src/game/core/IEmulatorCore.hpp
```

---

# 接口

```cpp id="9uh6n9"
class IEmulatorCore
{
public:
    virtual ~IEmulatorCore() = default;

    virtual bool LoadRom(const std::string& path) = 0;

    virtual void Reset() = 0;

    virtual void RunFrame() = 0;

    virtual void Stop() = 0;

    virtual bool IsRunning() const = 0;

    virtual const uint32_t* GetTopScreen() = 0;

    virtual const uint32_t* GetBottomScreen() = 0;

    virtual int GetScreenWidth() const = 0;

    virtual int GetScreenHeight() const = 0;

    virtual void PushInput(int key, bool pressed) = 0;

    virtual void SaveState(const std::string&) = 0;

    virtual void LoadState(const std::string&) = 0;
};
```

---

# 第十阶段：实现 CoreMelonDS

---

```text id="s2qjlwm"
CoreMelonDS : public IEmulatorCore
```

---

# CoreMelonDS 负责

```text id="2p8fwq"
LoadRom
RunFrame
GetFramebuffer
Input
SaveState
生命周期
```

---

# 不允许 UI 直接碰 melonDS API

---

# 第十一阶段：建立 MelonDSInstance

---

# 作用

封装：

```text id="i6z3gm"
官方 melonDS 全局状态
```

---

# 官方 melonDS 有大量：

```text id="h6if4r"
global
static
singleton
```

---

# 所以：

必须建立：

```text id="mjlwmc"
MelonDSInstance
```

---

# 负责：

```text id="e0jcn0"
Init
Shutdown
LoadROM
RunFrame
Reset
```

---

# 第十二阶段：先接软件渲染

---

# 非常重要

不要先搞 OpenGL Renderer。

---

# 第一目标

```text id="fgm44u"
显示第一帧 NDS 图像
```

---

# 使用 Software Renderer

即：

```text id="c0k4a8"
GPU framebuffer
```

---

# framebuffer

通常：

```text id="lkb22j"
256x192
RGBA8888
```

---

# 分别：

```text id="z3xpjf"
Top Screen
Bottom Screen
```

---

# 第十三阶段：接入你的渲染系统

---

# 正确流程

```text id="bj6k7m"
melonDS
    ↓
CPU framebuffer
    ↓
FrameUploader
    ↓
OpenGL Texture
    ↓
RenderChain
    ↓
Borealis Overlay
```

---

# 不允许

```text id="nwv90z"
melonDS 直接 glDraw
```

---

# 你已有：

```text id="mgd30t"
FrameUploader
GameTexture
RenderChain
```

已经足够。

---

# 第十四阶段：实现 FrameComposer

---

新增：

```text id="j6kibx"
FrameComposer
```

---

# 输入

```text id="xv8p29"
top framebuffer
bottom framebuffer
```

---

# 输出

```text id="pmst4x"
最终组合 framebuffer
```

---

# 支持布局

```text id="29c2r4"
Vertical
Horizontal
Single
Hybrid
```

---

# 布局必须由前端控制

---

# 第十五阶段：建立模拟线程

---

# 不允许：

```text id="jlwmnm"
UI线程 RunFrame
```

---

# 正确结构

```text id="5smb5m"
Emu Thread
    ↓
RunFrame()
```

---

# 主线程

```text id="0jzlb7"
上传纹理
渲染 UI
```

---

# 推荐：

```cpp id="ucjcv9"
while (running)
{
    core->RunFrame();

    SwapFramebuffer();

    SleepUntilNextFrame();
}
```

---

# 第十六阶段：线程同步

---

# 不允许

```text id="93esv7"
模拟线程直接 GL
```

---

# 正确方式

```text id="h7y75x"
模拟线程：
写 CPU framebuffer

渲染线程：
上传 GPU texture
```

---

# 推荐：

```text id="w0p6r0"
双缓冲
```

---

# frame结束

```text id="f8p3y8"
atomic swap
```

---

# 第十七阶段：输入系统

---

# 不让 melonDS 读取 HID

---

# 输入流程

```text id="cy5lpi"
GameInputManager
    ↓
CoreMelonDS::PushInput()
    ↓
melonDS key matrix
```

---

# 支持：

```text id="d4p4k1"
A B X Y
L R
DPad
Start Select
Touch
```

---

# 第十八阶段：音频系统

---

# 不让 melonDS 输出设备音频

---

# 正确流程

```text id="v8u4l1"
melonDS SPU
    ↓
PCM samples
    ↓
AudioManager
```

---

# 推荐：

```text id="l2zj2m"
RingBuffer
```

---

# 第十九阶段：SaveState

---

# 使用官方 SaveState API

---

# 不要自己序列化

---

# 包装：

```text id="9m9f8o"
SaveState(path)
LoadState(path)
```

---

# 第二十阶段：生命周期管理

---

# GamePage

统一：

```text id="5nq6l3"
Start
Pause
Resume
Stop
Destroy
```

---

# Stop 时必须：

```text id="3gh19z"
停止模拟线程
停止音频
释放 GPU 资源
关闭 ROM
```

---

# 第二十一阶段：资源释放

---

# 官方 melonDS 有大量全局对象

---

# 必须：

```cpp id="m71e48"
Shutdown()
```

---

# 否则：

```text id="r83cvd"
二次启动崩溃
```

---

# 第二十二阶段：接入 OpenGL 3D Renderer

---

# 软件渲染稳定后再做

---

# 非常重要

melonDS OpenGL Renderer：

默认会：

```text id="ji7nn8"
创建自己的 GL Context
```

---

# 必须修改

---

# 正确方式

```text id="d0n9rw"
复用 Borealis 的 GL Context
```

---

# 不允许

```text id="b8cwzy"
SDL_GL_CreateContext
```

---

# 目标

melonDS：

```text id="26iv0c"
只负责发 GL Draw Call
```

---

# Context 由：

```text id="o5k42t"
BeikLiveStation 管理
```

---

# 第二十三阶段：GL 状态隔离

---

# 非常重要

melonDS 会污染：

```text id="ok5o3v"
Framebuffer
Viewport
Shader
Blend
Depth
Texture
```

---

# 必须：

渲染前：

```cpp id="jlwmav"
SaveGLState();
```

渲染后：

```cpp id="1ubys9"
RestoreGLState();
```

---

# 否则：

```text id="2h8d1g"
UI 黑屏
shader 错乱
布局错乱
```

---

# 第二十四阶段：最终 OpenGL 3D 流程

---

# 正确流程

```text id="d3o0wc"
melonDS 3D
    ↓
Render To Texture
    ↓
RenderChain
```

---

# 不允许

```text id="xjlwm3"
melonDS SwapBuffers
```

---

# 第二十五阶段：配置系统

---

新增：

```text id="1mpt9f"
MelonDSConfig
```

---

# 管理：

```text id="u6jqx2"
RendererType
AudioLatency
ScreenLayout
FrameSkip
ThreadedRendering
JIT
ResolutionScale
```

---

# 第二十六阶段：推荐实施顺序

---

# Phase 1

```text id="ywz50s"
编译 melonds_core.a
```

---

# Phase 2

```text id="go3z1l"
显示第一帧
```

---

# Phase 3

```text id="l3n4f6"
输入
音频
稳定运行
```

---

# Phase 4

```text id="8ehx6f"
SaveState
FastForward
布局
```

---

# Phase 5

```text id="bd9m0n"
OpenGL 3D
```

---

# Phase 6

```text id="k7o1qq"
Shader
ResolutionScale
```

---

# Phase 7

```text id="c7ub7g"
Touch
Mic
RTC
Wifi
```

---

# 第二十七阶段：当前绝对不要做的事情

---

不要：

```text id="jlwm0n"
Qt
SDL Window
多 OpenGL Context
立即接 OpenGL Renderer
```

---

# 否则：

```text id="8nd0m5"
平台层地狱
```

---

# 第二十八阶段：当前唯一目标

---

当前唯一目标：

```text id="5a8n9v"
官方 melonDS Core
输出 framebuffer
到你的 RenderChain
```

---

# 一旦这一步成功

后面：

```text id="jlwmq2"
输入
音频
shader
3D
```

都会变简单。

---

# 最终结果

---

最终：

```text id="jlwmqw"
BeikLiveStation
```

完全掌控：

```text id="jlwmww"
UI
Renderer
Shader
Audio
Input
Layout
Lifecycle
```

---

而：

```text id="jlwm55"
官方 melonDS
```

只是：

```text id="jlwm66"
NDS 执行核心
```







# 官方 melonDS 接入时必须阅读的源码说明

# 目标

本说明用于指导：

```text id="2j1tqs"
其他大模型
```

在接入官方 melonDS 时：

```text id="7jlwmc"
优先阅读哪些源码
哪些不要碰
哪些必须修改
```

---

# 一、必须理解的核心思想

---

当前项目：

```text id="jlwm1a"
BeikLiveStation
```

是：

```text id="jlwm2a"
宿主程序（Host）
```

---

官方：

```text id="jlwm3a"
melonDS
```

只作为：

```text id="jlwm4a"
Emulator Core
```

---

因此：

# 不允许直接运行官方 frontend

---

必须：

```text id="jlwm5a"
只提取 Emulator Core
```

---

# 二、阅读源码的优先级

---

# 第一优先级（必须阅读）

这些是：

```text id="jlwm6a"
真正的模拟器核心
```

---

# 1. NDS.cpp / NDS.h

路径：

```text id="jlwm7a"
third_party/melonDS/src/NDS.*
```

---

# 必须理解：

```text id="jlwm8a"
NDS::Init()
NDS::LoadROM()
NDS::RunFrame()
NDS::Reset()
NDS::Stop()
```

---

# 这是：

```text id="jlwm9a"
整个模拟器入口
```

---

# 所有生命周期：

```text id="jlwm0a"
都从这里开始
```

---

# 2. GPU.cpp / GPU.h

路径：

```text id="jlwmab"
third_party/melonDS/src/GPU.*
```

---

# 必须理解：

```text id="jlwmbb"
framebuffer 输出
screen framebuffer
screen swap
```

---

# 重点：

找到：

```text id="jlwmcb"
Top Screen Buffer
Bottom Screen Buffer
```

---

# 当前阶段：

```text id="jwlvdb"
只接 Software Renderer
```

---

# 3. SPU.cpp / SPU.h

路径：

```text id="jlwmeb"
third_party/melonDS/src/SPU.*
```

---

# 必须理解：

```text id="jlwmfb"
PCM sample 输出
audio callback
buffer queue
```

---

# 最终：

```text id="jlwmgb"
接入 AudioManager
```

---

# 4. Savestate.cpp

路径：

```text id="jlwmhb"
third_party/melonDS/src/Savestate.*
```

---

# 必须理解：

```text id="jlwmib"
SaveState API
LoadState API
```

---

# 不允许：

```text id="jlwmjb"
自己实现序列化
```

---

# 5. Input 相关

查找：

```text id="jlwmkb"
KeyInput
Input
Keypad
```

---

# 必须理解：

```text id="jwlvlb"
按键矩阵
按键状态
触摸输入
```

---

# 最终：

```text id="jwlvmb"
由 GameInputManager 驱动
```

---

# 三、第二优先级（后期阅读）

---

这些是：

```text id="jwlvnb"
后期高级功能
```

---

# 1. OpenGL Renderer

查找：

```text id="jwlvob"
GLRenderer
OpenGLRenderer
```

---

# 当前阶段不要碰

---

# 只有：

```text id="jwlvpb"
软件渲染稳定后
```

再阅读。

---

# 必须修改：

```text id="jwlvqb"
GL Context 创建
```

---

# 不允许：

```text id="jwlvrb"
SDL_GL_CreateContext
```

---

# 2. JIT / Dynarec

查找：

```text id="jwlvsb"
JIT
Dynarec
ARMJIT
```

---

# 当前阶段：

```text id="jwlvtb"
不要编译
```

---

# 后期再启用。

---

# 3. Wifi

查找：

```text id="jwlvub"
Wifi
LAN
Socket
```

---

# 当前阶段：

```text id="jwlvvb"
完全不要碰
```

---

# 四、绝对不要阅读的部分

---

# 以下内容：

```text id="jwlvwb"
不要研究
不要修改
不要参考
```

---

# 1. Qt Frontend

路径：

```text id="jwlvxb"
frontend/qt
```

---

# 原因：

```text id="jwlvyb"
与你的架构完全无关
```

---

# 2. SDL Window

查找：

```text id="jwlvzb"
SDL_CreateWindow
SDL_GL_CreateContext
```

---

# 原因：

你已经有：

```text id="jwlv0b"
Borealis + OpenGL
```

---

# 不允许：

```text id="jwlv1b"
melonDS 创建窗口
```

---

# 3. 平台 frontend

路径：

```text id="jwlv2b"
android/
windows/
macos/
```

---

# 全部不要碰

---

# 五、必须重点修改的地方

---

# 1. 生命周期

官方：

```text id="jwlv3b"
frontend 管理生命周期
```

---

你必须改成：

```text id="jwlv4b"
CoreMelonDS 管理生命周期
```

---

# 2. framebuffer 输出

官方：

```text id="jwlv5b"
直接渲染
```

---

你必须改成：

```text id="jwlv6b"
输出 CPU framebuffer
```

---

# 3. 音频输出

官方：

```text id="jwlv7b"
直接播放设备音频
```

---

你必须改成：

```text id="jwlv8b"
输出 PCM samples
```

---

# 4. 输入

官方：

```text id="jwlv9b"
自己读取 HID
```

---

你必须改成：

```text id="jwlvab"
由 GameInputManager 注入
```

---

# 六、当前阶段唯一目标

---

# 不要想：

```text id="jwlvbb"
OpenGL
Shader
JIT
Wifi
RTC
```

---

# 当前唯一目标：

```text id="jwlvcb"
RunFrame()
输出 framebuffer
显示第一帧
```

---

# 只要：

```text id="jwlvdb"
第一帧显示成功
```

---

说明：

```text id="jwlveb"
核心已经接入成功
```

---

# 七、正确实施顺序

---

# Step 1

阅读：

```text id="jwlvfb"
NDS.cpp
```

---

# Step 2

找到：

```text id="jwlvgb"
RunFrame()
```

---

# Step 3

找到：

```text id="jwlvhb"
framebuffer 输出
```

---

# Step 4

建立：

```text id="jwlvib"
CoreMelonDS
```

---

# Step 5

让：

```text id="jwlvjb"
FrameUploader
```

上传 framebuffer。

---

# Step 6

显示第一帧。

---

# 只有完成以上步骤后

才允许：

```text id="jwlvkb"
输入
音频
SaveState
OpenGL
```

---

# 八、最终目标

---

最终：

```text id="jwlvlb"
melonDS
```

必须变成：

```text id="jwlvmb"
无 UI
无窗口
无平台层
无 Context
```

---

只剩：

```text id="jwlvnb"
Emulator Core
```

---

然后：

```text id="jwlvob"
BeikLiveStation
```

负责：

```text id="jwlvpb"
UI
Renderer
Audio
Input
Lifecycle
Shader
Layout
```
