
```text
当前状态:
- third_party/melonds/            (官方源码)
- third_party/melonds-switch/     (switch平台参考工程)
- GBAStation 已有统一模拟器框架
- UI/渲染系统已存在(BeikUI/OpenGL)
- 平台: Switch + libnx
```

目标：

```text
GBAStation
├── mGBA             (libretro)
├── fceumm           (libretro)
├── snes9x           (libretro)
├── melonDS          (native)
│    ├── ARM64 JIT
│    ├── Threaded Renderer
│    ├── audren
│    ├── hid
│    └── savestate
```

melonDS 已内置 ARM64 JIT，构建系统也提供 `ENABLE_JIT` 选项。([DeepWiki][1])

下面方案中所有的目录结构和文件创建都是建议方向，具体实现需要根据项目结构进行调整。

---

# 第1步：创建核心目录

新增：

```text
src/emulator/melonds/

    MelonDSCore.h
    MelonDSCore.cpp

    MelonDSPlatform.cpp
    MelonDSPlatform.h

    MelonDSAudio.cpp
    MelonDSAudio.h

    MelonDSInput.cpp
    MelonDSInput.h

    MelonDSVideo.cpp
    MelonDSVideo.h
```

---

# 第2步：实现统一核心接口

`MelonDSCore.h`

```cpp
class MelonDSCore:
public IEmulatorCore
{
public:

    bool Initialize() override;

    bool LoadGame(
        const std::string& path
    ) override;

    void RunFrame() override;

    void Reset() override;

    void Stop() override;

    void Pause(bool) override;

    bool SaveState(
        const std::string&
    ) override;

    bool LoadState(
        const std::string&
    ) override;

    void SetButton(
        int key,
        bool pressed
    ) override;

    void SetTouch(
        int x,
        int y,
        bool down
    ) override;

    const uint32_t*
    GetFrameBuffer();

private:

    std::atomic<bool> loaded=false;

    uint32_t* framebuffer=nullptr;

    int width=256;
    int height=384;
};
```

---

# 第3步：裁剪 melonDS 源码

不要编译：

```text
third_party/melonds/src/frontend/
third_party/melonds/src/qt/
third_party/melonds/src/libui/
```

保留：

```text
third_party/melonds/src/

ARM/
ARMJIT/
Config/
DSi/
GPU/
NDS/
Platform/
Savestate/
SPU/
```

ARM64 JIT 位于 `ARMJIT`。([DeepWiki][2])

---

# 第4步：建立 Switch 平台实现

从：

```text
third_party/melonds-switch
```

参考：

```text
Platform.cpp
Thread.cpp
```

实现：

```cpp
namespace Platform
{
    FILE* OpenFile(
        const char* path,
        const char* mode
    );

    uint64_t GetTimeUs();

    void Sleep(
        uint64_t ns
    );

    void Log(
        LogLevel level,
        const char* fmt,...
    );
}
```

实现：

```cpp
FILE* Platform::OpenFile(...)
{
    return fopen(path,mode);
}

uint64_t Platform::GetTimeUs()
{
    return armGetSystemTick();
}

void Platform::Sleep(
    uint64_t ns
)
{
    svcSleepThread(ns);
}
```

---

# 第5步：添加 CMake

新增：

```cmake
add_library(
    melonds_core

    third_party/melonds/src/ARM/*.cpp
    third_party/melonds/src/ARMJIT/*.cpp
    third_party/melonds/src/GPU/*.cpp
    third_party/melonds/src/NDS/*.cpp
    third_party/melonds/src/SPU/*.cpp
    third_party/melonds/src/DSi/*.cpp
    third_party/melonds/src/Config/*.cpp
    third_party/melonds/src/Savestate/*.cpp

    src/emulator/melonds/*.cpp
)
```

不要：

```cmake
add_subdirectory(
third_party/melonds
)
```

因为会把 Qt/SDL 一起拉进来。

---

# 第6步：开启 JIT

```cmake
target_compile_definitions(
    melonds_core
    PRIVATE

    ENABLE_JIT=1
)
```

```cmake
target_compile_options(
    melonds_core
    PRIVATE

    -O3
    -mcpu=cortex-a57
    -ffast-math
    -fomit-frame-pointer
    -flto
)
```

```cmake
target_link_options(
    melonds_core
    PRIVATE
    -flto
)
```

melonDS 官方构建默认支持 ARM64 JIT。([DeepWiki][1])

运行初始化：

```cpp
Config::JIT_Enable=true;

NDS::SetJITArgs(
    true
);
```

验证：

```cpp
printf(
    "JIT=%d\n",
    NDS::IsJITEnabled()
);
```

---

# 第7步：初始化模拟器

`MelonDSCore.cpp`

```cpp
bool MelonDSCore::Initialize()
{
    NDS::Init();

    GPU::Init();

    SPU::Init();

    Config::JIT_Enable=true;

    GPU::SetThreaded3D(
        true
    );

    return true;
}
```

线程化 GPU 在部分场景可带来额外收益。([DeepWiki][3])

---

# 第8步：加载游戏

```cpp
bool MelonDSCore::LoadGame(
const std::string& path)
{
    loaded=
    NDS::LoadROM(
        path.c_str()
    );

    return loaded;
}
```

BIOS：

```text
sdmc:/GBAStation/bios/nds/

bios7.bin
bios9.bin
firmware.bin
```

Switch 社区实践也使用该文件组合。([Reddit][4])

---

# 第9步：执行帧

```cpp
void MelonDSCore::RunFrame()
{
    if(!loaded)
        return;

    NDS::RunFrame();

    framebuffer=
        GPU::GetFrameBuffer();
}
```

不要：

```cpp
memcpy(
temp,
framebuffer,
size
);
```

直接：

```cpp
renderer->UpdateTexture(
    framebuffer
);
```

避免：

```text
melonDS
↓

临时buffer

↓

GPU
```

---

# 第10步：视频输出

初始化：

```cpp
ndsTexture.Create(
    256,
    384,
    TextureFormat::RGBA8
);
```

每帧：

```cpp
ndsTexture.Update(
    framebuffer
);
```

显示：

```cpp
DrawTexture(
    ndsTexture,
    rect
);
```

---

# 第11步：音频

创建：

```cpp
class MelonDSAudio
{
    RingBuffer<int16_t> audio;
};
```

初始化：

```cpp
audio.Init(
16384
);
```

读取：

```cpp
SPU::ReadOutput(
buffer,
samples
);

audio.Push(
buffer,
samples
);
```

输出：

```cpp
audrenPlay(
audio.GetReadPtr()
);
```

不要：

```cpp
while(full)
{
    wait();
}
```

---

# 第12步：输入

映射：

```cpp
KEY_A      -> Input_A
KEY_B      -> Input_B
KEY_X      -> Input_X
KEY_Y      -> Input_Y
KEY_PLUS   -> Input_Start
KEY_MINUS  -> Input_Select
```

触摸：

```cpp
touchPosition pos;

hidTouchRead(
&pos,
0
);

nds->TouchScreen(
    pos.px*256/uiWidth,
    pos.py*192/uiHeight
);
```

---

# 第13步：存档

目录：

```text
sdmc:/GBAStation/saves/nds/

Pokemon.nds
Pokemon.sav
Pokemon.state
```

实现：

```cpp
SaveSRAM()

LoadSRAM()

SaveState()

LoadState()
```

调用：

```cpp
NDS::DoSavestate();
```

---

# 第14步：启动流程

```text
UI

↓
EmulatorManager

↓
MelonDSCore::Initialize()

↓
LoadGame()

↓
RunFrame()

↓
UpdateTexture()

↓
AudioSubmit()
```

---

预计收益（相对之前的 libretro 方案）：

```text
ARM64 JIT            +20~40%
去掉libretro层       +10~15%
Threaded Renderer    +5~15%
减少拷贝             +5~10%

总收益:

≈30~60%
```

在 Switch 上，独立版开启 ARM64 JIT 被大量用户反馈能让多数游戏达到接近满速。([Reddit][4])

[1]: https://deepwiki.com/melonDS-emu/melonDS/1.2-building-melonds?utm_source=chatgpt.com "Building melonDS | melonDS-emu/melonDS | DeepWiki"
[2]: https://deepwiki.com/melonDS-emu/melonDS/2.2-arm-cpu-emulation?utm_source=chatgpt.com "ARM CPU Emulation | melonDS-emu/melonDS | DeepWiki"
[3]: https://deepwiki.com/melonDS-emu/melonDS/2-core-emulation?utm_source=chatgpt.com "Core Emulation | melonDS-emu/melonDS | DeepWiki"
[4]: https://www.reddit.com/r/SwitchHacks/comments/dhtten/melonds_alpha_with_arm64_jit_for_switch/?utm_source=chatgpt.com "MelonDS Alpha with ARM64 JIT for Switch"
