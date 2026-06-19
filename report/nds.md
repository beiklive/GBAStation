
# GBAStation — melonDS(libretro) 静态集成实施方案

目标：

将：

```text
third_party/melonds-ds
```

中的 libretro 核心静态集成到 GBAStation。

要求：

* 不依赖 RetroArch
* 静态链接
* 接入现有 IEmulatorCore
* OpenGL继续作为统一渲染器
* 支持 Switch/libnx 和 Windows
* 支持 DS 双屏布局
* 支持存档/即时存档
* 支持触摸
* 后期支持金手指
* 参考本方案，但仍以项目本体架构为主进行修正

---

# 一、目录结构

建议项目新增(优先以项目当前结构为主)：

```text
GBAStation/

├── src/emulator/
│   │
│   ├── melonds/
│   │   │
│   │   ├── CoreMelonDS.h
│   │   ├── CoreMelonDS.cpp
│   │   │
│   │   ├── MelonDSAdapter.h
│   │   ├── MelonDSAdapter.cpp
│   │   │
│   │   ├── MelonDSCallbacks.cpp
│   │   ├── MelonDSCallbacks.h
│   │   │
│   │   ├── MelonDSVideo.cpp
│   │   ├── MelonDSAudio.cpp
│   │   └── MelonDSInput.cpp
│
├── third_party/
│   │
│   └── melonds-ds/
```

---

# 二、统一接口

已有：

```cpp
class IEmulatorCore
{
public:

    virtual bool LoadGame(
        const std::string& path)=0;

    virtual void RunFrame()=0;

    virtual void Reset()=0;

    virtual void Shutdown()=0;

    virtual bool SaveState(
        int slot)=0;

    virtual bool LoadState(
        int slot)=0;

    virtual TextureHandle GetTexture()=0;

    virtual void SetInput(
        const InputState&
    )=0;
};
```

实现：

```cpp
class CoreMelonDS :
    public IEmulatorCore
{
public:

    bool LoadGame(
        const std::string&
    ) override;

    void RunFrame() override;

    void Reset() override;

    void Shutdown() override;

    bool SaveState(
        int slot
    ) override;

    bool LoadState(
        int slot
    ) override;

    TextureHandle GetTexture()
    override;

private:

    MelonDSAdapter m_adapter;

};
```

---

# 三、libretro符号隔离

多个静态核心会冲突：

错误：

```cpp
retro_init()
retro_run()
retro_load_game()
```

改：

新增：

```cpp
cores/melonds/MelonDSRename.h
```

内容：

```cpp
#define retro_init melonds_retro_init
#define retro_deinit melonds_retro_deinit

#define retro_load_game \
    melonds_retro_load_game

#define retro_unload_game \
    melonds_retro_unload_game

#define retro_run \
    melonds_retro_run

#define retro_serialize \
    melonds_retro_serialize

#define retro_unserialize \
    melonds_retro_unserialize

#define retro_get_memory_data \
    melonds_retro_get_memory_data

#define retro_get_memory_size \
    melonds_retro_get_memory_size
```

在核心源码前：

```cpp
#include "MelonDSRename.h"

#include <libretro.c>
```

必须全部重命名。

---

# 四、回调绑定

初始化：

```cpp
void MelonDSAdapter::Initialize()
{
    melonds_retro_set_video_refresh(
        VideoCallback
    );

    melonds_retro_set_audio_sample_batch(
        AudioCallback
    );

    melonds_retro_set_input_poll(
        InputPoll
    );

    melonds_retro_set_input_state(
        InputState
    );

    melonds_retro_init();
}
```

---

# 五、视频系统

DS：

```text
上屏:

256×192

下屏:

256×192
```

组合：

```text
256×384
```

缓冲：

```cpp
static uint32_t g_frameBuffer[
    256*384
];
```

视频回调：

```cpp
void VideoCallback(
    const void* data,
    unsigned width,
    unsigned height,
    size_t pitch
)
{
    memcpy(
        g_frameBuffer,
        data,
        width*height*4
    );
}
```

---

OpenGL上传：

```cpp
glBindTexture(
    GL_TEXTURE_2D,
    mTexture
);

glTexSubImage2D(
    GL_TEXTURE_2D,
    0,
    0,
    0,
    256,
    384,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    g_frameBuffer
);
```

---

# 六、双屏布局

新增：

```cpp
enum class DSLayout
{
    Vertical,
    Horizontal,
    SingleTop,
    SingleBottom
};
```

配置：

```text
sdmc:/GBAStation/config/nds.json
```

默认：

```json
{
    "layout":"Vertical"
}
```

渲染：

```cpp
switch(layout)
{
case Vertical:

    DrawTop();

    DrawBottom();

break;

case Horizontal:

    DrawLeft();

    DrawRight();

break;

case SingleTop:

break;

case SingleBottom:

break;
}
```

---

# 七、BIOS路径

固定：

```text
sdmc:/GBAStation/bios/nds/

bios7.bin
bios9.bin
firmware.bin
```

加载：

```cpp
retro_variable bios[]=
{
{
"melonds_ds_bios_path",
"sdmc:/GBAStation/bios/nds/"
}
};
```

启动前检查：

```cpp
if(!Exists(
    BIOS7))
{
    return false;
}
```

缺失提示：

```text
Missing BIOS:

bios7.bin
bios9.bin
firmware.bin
```

---

# 八、ROM路径

支持：

```text
.nds
.zip
.7z
```

加载：

```cpp
retro_game_info game={};

game.path=
    path.c_str();

melonds_retro_load_game(
    &game
);
```

---

# 九、存档

路径：

```text
sdmc:/GBAStation/saves/nds/
```

格式：

```text
Pokemon.sav
Mario.sav
```

回调：

```cpp
RETRO_ENVIRONMENT_SET_SAVE_DIRECTORY
```

返回：

```cpp
"sdmc:/GBAStation/saves/nds/"
```

---

# 十、即时存档

路径：

```text
sdmc:/GBAStation/states/nds/
```

格式：

```text
Pokemon.slot1.state
Pokemon.slot2.state
```

保存：

```cpp
size_t size=
melonds_retro_serialize_size();

melonds_retro_serialize(
    buffer,
    size
);
```

---

# 十一、音频

回调：

```cpp
size_t AudioCallback(
    const int16_t* data,
    size_t frames
)
{
    AudioManager::Push(
        data,
        frames
    );

    return frames;
}
```

统一：

```text
48000hz

stereo
```

---

# 十二、输入映射

Switch：

```cpp
A → NDS A
B → NDS B
X → NDS X
Y → NDS Y

L → L
R → R

Plus → START
Minus → SELECT
```

摇杆：

```cpp
LeftStick

↓

DPad
```

---

# 十三、触摸

新增：

```cpp
struct TouchState
{
    bool pressed;

    float x;

    float y;
};
```

转换：

```cpp
switchPosToNDS(
    touchX,
    touchY
);
```

DS：

```text
256×192
```

映射：

```cpp
ndsX=
touchX*256/screenW;

ndsY=
touchY*192/screenH;
```

---

# 十四、性能

禁止：

```cpp
malloc()

free()
```

在：

```cpp
RunFrame()
```

中调用。

必须：

初始化一次：

```cpp
framebuffer

audiobuffer

savestatebuffer
```

---

# 十五、主循环

```cpp
while(appletMainLoop())
{
    Input.Update();

    core->SetInput();

    core->RunFrame();

    renderer.Render();

    audio.Update();
}
```

---

# 十六、CMake

```cmake
add_library(core_melonds

    cores/melonds/CoreMelonDS.cpp
    cores/melonds/MelonDSAdapter.cpp
    cores/melonds/MelonDSCallbacks.cpp

    third_party/melonds-ds/libretro/libretro.cpp
)

target_include_directories(
    core_melonds
    PUBLIC

    cores/melonds
    third_party/melonds-ds
)

target_link_libraries(
    core_melonds
    PRIVATE
    nx
)
```

---

# 十七、Codex实施要求

```text
1. 不依赖RetroArch

2. 不编译动态库

3. 静态编译

4. 重命名全部retro符号

5. 使用统一IEmulatorCore接口

6. OpenGL仅负责显示FrameBuffer

7. 视频输出统一RGBA8888

8. BIOS:
sdmc:/GBAStation/bios/nds/

9. Save:
sdmc:/GBAStation/saves/nds/

10. State:
sdmc:/GBAStation/states/nds/

11. 支持DS双屏布局

12. 支持触摸

13. 不在RunFrame进行动态内存分配

14. 支持Switch和Windows
```

