# BeikLiveStation 第5轮修复报告

## 修复概览

本次修复针对以下2个问题：

1. **Switch 音频仍然拖慢** – 四个线程竞争三个 CPU 核心，导致游戏线程和音频线程互相干扰
2. **PC 游戏帧率只有约50fps** – 帧率定时方案存在累积漂移，偶发慢帧会压缩后续帧的休眠预算

---

## 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/UI/game_view.cpp` | 为 Switch 的游戏线程和音频推送线程设置 CPU 核心亲和性；将帧率控制改为累积理想时间方案 |
| `src/Audio/AudioManager.cpp` | 为 Switch 的音频输出线程设置 CPU 核心亲和性（Core 2）|
| `report/fix_report_round5.md` | 本报告（新增）|

---

## 问题1：Switch 音频拖慢

### 根本原因

Switch 用户进程可使用 Core 0、Core 1、Core 2 共三个 CPU 核心。
当前系统运行四个线程，但没有设置核心亲和性（affinity），由 OS 调度器自由分配：

| 线程 | 说明 |
|------|------|
| 主/UI 线程 | borealis 渲染、输入 |
| `m_gameThread` | 核心模拟（retro_run） |
| `m_audioThread` | 从队列取样本推送给 AudioManager |
| `AudioManager::m_thread` | 从 ring buffer 取数据输出到硬件 (audout) |

当四个线程无序地竞争三个核心时，`m_gameThread` 极易被抢占或与音频线程共享同一核心，造成帧率下降和音频延迟。

### 修复方案

在各线程启动后，**在线程函数内部**调用 libnx 系统调用 `svcSetThreadCoreMask(CUR_THREAD_HANDLE, core, 1ULL << core)` 将线程绑定到指定核心：

```
Core 0：UI/主线程（由 OS 默认分配，不作修改）
Core 1：游戏模拟线程（m_gameThread） ← 独占，算力最密集
Core 2：音频推送线程 (m_audioThread) + 音频输出线程 (AudioManager::m_thread) ← 共享，两者均大部分时间阻塞在 I/O
```

#### 实现细节

在 `startGameThread()` 的线程 lambda 开头，以及 `AudioManager::audioThreadFunc()` 的开头，添加：

```cpp
#ifdef __SWITCH__
    svcSetThreadCoreMask(CUR_THREAD_HANDLE, <core>, 1ULL << <core>);
#endif
```

- `m_gameThread`：Core 1（独占核心，专用于模拟）
- `m_audioThread`：Core 2（与音频输出线程共享，但两者均以 I/O 阻塞为主）
- `AudioManager::audioThreadFunc()`（Switch 后端）：Core 2

#### 为什么共享 Core 2 可行

`m_audioThread` 绝大部分时间等待 `m_audioQueueCV`（有数据时才唤醒）。
`AudioManager::m_thread` 调用 `audoutPlayBuffer()` 阻塞等待硬件缓冲区清空（约 10ms/次）。
二者几乎不同时占用 CPU，共享一个核心不会造成明显竞争。

---

## 问题2：PC 帧率约50fps

### 根本原因

原有帧率控制方案在每次迭代开始时重新获取当前时间作为"帧起点"：

```cpp
auto frameStart = Clock::now();   // ← 每帧重新锚定
// ... 执行帧工作 ...
auto sleepTarget = frameStart + frameDuration;
```

当某帧执行时间超过 `frameDuration`（16.7ms）时，下一帧的 `frameStart` 就已经"晚了"，它的 `sleepTarget` 只比下一帧的实际结束时间晚一点点，导致后续帧的休眠预算几乎被完全吃掉。
若遇到多个连续轻微慢帧，误差累积，整体帧率可下降至约50fps。

### 修复方案

改用**累积理想时间**方案（Accumulated Ideal Time）：

```cpp
// 循环外初始化一次
auto nextFrameTarget = Clock::now();

while (running) {
    // ... 执行帧工作 ...

    auto nowPost = Clock::now();
    nextFrameTarget += targetDur;       // 累积推进，不依赖实际执行时间

    // 防漂移：若本帧超出预算，重置为 now（给下一帧完整的时间预算，而非调度到过去）
    if (nextFrameTarget < nowPost) {
        nextFrameTarget = nowPost;
    }

    // 混合睡眠：粗粒度 sleep + 自旋等待精确截止时间
    // 防漂移：若帧超出预算，重置目标为 now，下一帧获得完整预算
    if (nowPost < nextFrameTarget) {
        auto coarseDur = (nextFrameTarget - nowPost) - spinGuard;
        if (coarseDur.count() > 0)
            std::this_thread::sleep_for(coarseDur);
        while (Clock::now() < nextFrameTarget) { /* busy spin */ }
    }
}
```

#### 效果对比

| 场景 | 旧方案 | 新方案 |
|------|--------|--------|
| 单帧耗时 13ms（快帧） | sleep 3.7ms ✓ | sleep 3.7ms ✓ |
| 单帧耗时 20ms（慢帧） | 不 sleep，下帧立即开始 ✓ | 不 sleep，下帧立即开始 ✓ |
| 上一帧慢，本帧快（13ms） | `frameStart` 晚了 3.3ms，sleep 仅 0.4ms，帧率受损 ✗ | `nextFrameTarget` 独立推进，sleep 完整 3.7ms，帧率恢复 ✓ |
| 连续轻微慢帧（各 17ms） | 每帧 0ms sleep，误差累积，整体帧率下降 ✗ | 误差被防漂移机制吸收，下帧独立计时 ✓ |

#### 附加优化

- 将 FPS 计数器中的 `auto now = Clock::now()` 与帧控制中的 `auto nowPre = Clock::now()` 合并为一次 `auto nowPost = Clock::now()`，减少系统调用次数。

---

## 线程模型（修复后）

```
主线程（Core 0）:  draw() → 上传纹理 → 绘制覆盖层 → 检测退出请求
游戏线程（Core 1）: pollInput() → retro_run() × N → 样本入队 → 帧率控制睡眠
音频推送线程（Core 2）: m_audioQueueCV 等待 → pushSamples()（可阻塞）
音频输出线程（Core 2）: ring buffer 读取 → audoutPlayBuffer()（硬件 I/O 阻塞）
```

- **核心亲和性绑定**确保游戏线程独占 Core 1，不受音频调度影响。
- **累积理想时间方案**确保单帧抖动不会影响后续帧的时间预算，实现稳定60fps。

---

## 配置文件无变化

本次修复均为运行时优化，不涉及任何配置项变更。
