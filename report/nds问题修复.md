你这次整理已经很完整了，基本能把问题收敛到几个关键点。我把它进一步抽象成“线程/数据流/瓶颈点”三个层面，并给出优先级和具体改法。现在最大的不是 melonDS 模拟速度本身，而是“线程调度 + 同步阻塞 + 错误的数据路径”。

# 一、当前实际运行架构（梳理后的视图）

当前在 Switch 上实际上是：

```text
core0
├─ UI主线程
│   ├─ Borealis/UI
│   ├─ OpenGL绘制
│   └─ NDS纹理复制

core1
├─ 游戏线程(GameLoop)
│   ├─ NDS::RunFrame()
│   │   ├─ ARM9
│   │   ├─ ARM7
│   │   ├─ DMA
│   │   ├─ Timer
│   │   ├─ Scheduler
│   │   ├─ GPU事件
│   │   └─ SPU
│   ├─ 输入
│   ├─ rewind
│   ├─ save检测
│   └─ Audio push

core2
├─ Audio线程
│   └─ audren输出

???
├─ SoftRenderer线程
│   └─ Platform::Thread_Create()
```

目前问题：

```text
SoftRenderer线程未绑核

↓

Switch scheduler随机调度

↓

高概率落到core2

↓

Audio线程 + SoftRenderer竞争

↓

Audio来不及取样

↓

补0

↓

爆音
```

所以你看到：

```text
普通场景:
core2≈80%

观景台:
core2≈100%
音频爆音
掉帧
```

逻辑上完全成立。

---

# 二、性能问题优先级排序

按收益排序：

|                问题 | 影响 | 优先级 |
| ----------------: | -: | --: |
| SoftRenderer线程未绑核 | 极高 |  P0 |
|   同步savestate/PNG | 极高 |  P0 |
|            快进逻辑错误 |  高 |  P0 |
|     GL Readback回退 |  高 |  P1 |
|    SRAM CRC每秒全量扫描 |  中 |  P1 |
|     m_ndsMutex锁竞争 |  中 |  P1 |
|      FastMemory关闭 |  中 |  P2 |

---

# 三、修复方案

## P0：给SoftRenderer线程绑定独立核

这是最先做的。

现在：

```cpp
Platform::Thread_Create(...)
{
    return new std::thread(func);
}
```

改：

```cpp
void ThreadEntry(...)
{
    svcSetThreadCoreMask(
        CUR_THREAD_HANDLE,
        0x8,   // preferred core3
        0x8
    );

    func();
}
```

然后：

```cpp
std::thread(ThreadEntry,...)
```

Switch实际有：

```text
core0
core1
core2
core3
```

建议：

```text
core0 → UI
core1 → 游戏
core2 → 音频
core3 → SoftRenderer
```

变成：

```text
core0
UI

core1
Game

core2
Audio

core3
SoftRenderer
```

收益：

复杂3D：

```text
+15~30%
```

并且爆音会大幅减少。

---

## P0：彻底异步存档

现在：

```cpp
saveSram()

Serialize()

stbi_write_png()
```

全部阻塞游戏线程。

应该改：

```cpp
GameThread
    ↓

SaveJobQueue.push()

    ↓

SaveWorkerThread(core3)
    ↓

Serialize
Write
PNG
```

结构：

```cpp
class SaveManager
{
    LockFreeQueue<SaveTask> queue;

    std::thread worker;
};
```

游戏线程：

```cpp
enqueueSave();
```

后台：

```cpp
while(running)
{
    task=queue.pop();

    serialize();
    write();
    writeThumbnail();
}
```

收益：

消除：

```text
瞬间卡顿
音频爆音
暂停1~2秒
```

---

## P0：修快进逻辑

你当前：

```cpp
4x:

跑2帧
sleep 0.5base
```

实际上：

```text
2x
```

应该：

```cpp
for(i=0;i<speed;i++)
{
    RunFrame();
}

if(speed==1)
    throttle();
```

即：

```cpp
if(ff>1)
{
    for(int i=0;i<ff;i++)
        RunFrame();
}
else
{
    RunFrame();
    throttle();
}
```

音频：

快进直接：

```cpp
drop samples
```

别输出全部。

收益：

```text
4x→≈4x
8x→≈8x
```

---

# 四、P1：消灭GL readback

当前：

```cpp
GPU
↓

FBO

↓

glReadPixels

↓

CPU

↓

RGBA转换

↓

upload

↓

GPU
```

这是灾难路径：

```text
GPU→CPU→GPU
```

尤其 Tegra X1。

应该强制：

```cpp
melonDS texture

↓

GameRenderer texture
```

即：

```cpp
glCopyImageSubData()
```

或者：

```cpp
FBO blit
```

不要：

```cpp
glReadPixels()
```

建议：

```cpp
if(accelerated)
    forceTextureConsumer=true;
```

失败不要自动退。

直接：

```cpp
retry lock
```

不要CPU回退。

收益：

复杂3D：

```text
+10~20%
```

---

# 五、P1：减小锁竞争

现在：

```cpp
RunFrame()
{
    lock(m_ndsMutex);

    m_nds->RunFrame();

    SPU.ReadOutput();

    unlock
}
```

UI：

```cpp
copyTexture()
{
    lock(m_ndsMutex)
}
```

于是：

```text
UI等待Game

Game等待UI
```

建议：

复制纹理不要持整个锁：

```cpp
lock()

GLuint tex=
renderer->GetOutputTexture();

unlock()

copy(tex);
```

或：

双buffer：

```cpp
textureA
textureB

frame结束交换
```

收益：

```text
帧抖动减少
```

---

# 六、P1：优化SRAM检测

现在：

```cpp
每秒：

CRC32(整个SRAM)
```

假设：

```text
512KB SRAM
```

虽然不算巨大，但持续做没意义。

改：

写时标记：

```cpp
bool dirty=false;

memoryWrite()
{
    dirty=true;
}
```

然后：

```cpp
if(dirty && elapsed>2s)
    save();
```

不要：

```cpp
CRC32全扫描
```

---

# 七、P2：开启FastMemory

你当前：

```cpp
JIT=true
FastMemory=false
```

这是非常保守的配置。

FastMemory：

```cpp
ARM虚拟地址

↓

直接映射宿主地址
```

减少：

```text
页查找
TLB模拟
```

通常：

```text
+10~25%
```

但要验证：

```cpp
cfg.FastMemory=true;
```

看：

```text
宝可梦黑2
DQ9
马车DS
```

是否崩溃。

如果稳定就永久开启。

---

# 八、最终建议的Switch线程布局

建议改成：

```text
core0
└─ UI

core1
└─ NDS主模拟线程

core2
└─ Audio

core3
├─ SoftRenderer
└─ SaveWorker
```

如果 SaveWorker 工作量大：

```text
core3:
    SoftRenderer(高优先级)

core0:
    SaveWorker(低优先级)
```

---

# 九、预期结果

修完 P0 后：

```text
宝可梦黑2观景台：

当前:
42~48fps
core2≈100%
爆音

↓

修复后:

55~60fps
core2≈25~40%
无爆音
```

修完 P1：

```text
60fps稳定
快进4x≈4x
快进8x≈6~8x
```

修完 P2：

```text
复杂3D还能再提升10~20%
```

从你给的信息看，我认为真正的根因不是“melonDS 太慢”，而是 **Switch 调度把软件3D、音频、同步IO、GL回读全部挤在同一时间轴上了**。模拟核心本身大概率只占问题的 40%，剩下 60% 是外围架构。
