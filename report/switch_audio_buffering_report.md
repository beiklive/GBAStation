# Switch 音频缓冲优化工作汇报

## 问题描述

在 Nintendo Switch 端，音频子系统会导致游戏运行速度降低（"音频拖慢"问题）。根本原因是旧实现使用了 `audoutPlayBuffer`（阻塞式 API），该函数在每次向硬件提交一个音频缓冲区后，会同步等待该缓冲区播放完毕（约 10.67 ms）。

**具体问题链：**
1. 音频线程每次只能提交一个缓冲区（2 个 buffer 轮流），然后阻塞等待它播完
2. 当游戏线程的音频产出速率与音频线程的消耗速率出现微小偏差时，环形缓冲区（ring buffer）会逐渐填满
3. 环形缓冲区满时，`pushSamples()` 通过条件变量阻塞游戏线程，导致游戏帧率下降
4. 加速模式（快进）结束时，环形缓冲区中可能残留过期音频数据，重新播放时产生杂音

---

## 修改内容

### 1. `src/Audio/AudioManager.cpp`（Switch 音频后端）

#### 旧方案（2 缓冲 + `audoutPlayBuffer` 阻塞）

```cpp
struct SwitchAudioState {
    int16_t bufData[2][SWITCH_FRAMES * 2];
    AudioOutBuffer outBuf[2];
    int curBuf = 0;
    AudioOutBuffer* released = nullptr;
};

// audioThreadFunc 核心循环（阻塞式）：
audoutPlayBuffer(&sw->outBuf[sw->curBuf], &sw->released);  // 阻塞约 10.67ms
sw->curBuf ^= 1;
```

**缺点：** 每次 IPC 调用后阻塞，音频线程无法及时清空 ring buffer，导致游戏线程挂起。

#### 新方案（N\_BUFFERS=4 + `audoutAppendAudioOutBuffer` 非阻塞）

- 将硬件缓冲区数量从 **2** 增加到 **4**（`SWITCH_N_BUFFERS = 4`）
- 初始化时将全部 4 个缓冲区预填静音并一次性提交，确保硬件立即开始无缝播放
- 音频线程采用**非阻塞检查 + 短超时等待**的方式回收已播放的缓冲区：
  1. 调用 `audoutWaitPlayFinish(&released, &count, 0)`（超时 0，非阻塞）尝试回收空闲缓冲区
  2. 若所有缓冲区槽位均已占满，在循环中以 10 ms 超时等待，直到有槽位空出或 `m_running` 被清除
  3. 填充下一个缓冲区后，调用 `audoutAppendAudioOutBuffer`（非阻塞）提交

**优点：**
- 音频线程不再在每个缓冲区上同步等待，可以持续、快速地清空 ring buffer
- 硬件队列中始终有多个缓冲区，减少音频断续的可能
- 游戏线程的 `pushSamples()` 被阻塞的概率大幅降低，帧率更稳定

### 2. `src/Audio/AudioManager.cpp` + `include/Audio/AudioManager.hpp`

新增跨平台公共方法 `flushRingBuffer()`：

```cpp
void AudioManager::flushRingBuffer()
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_available = 0;
    m_writePos  = m_readPos;   // 重置读写指针，丢弃全部待播数据
    m_spaceCV.notify_all();    // 唤醒任何在 pushSamples() 中等待的调用者
}
```

用于快进结束时清除 ring buffer 中残留的过期音频，防止杂音。

### 3. `src/UI/game_view.cpp`

在游戏主循环中（Switch 端），检测**快进→正常速度**的状态迁移，并在迁移发生时调用 `flushRingBuffer()`：

```cpp
#ifdef __SWITCH__
bool prevFastForward = false;
#endif

// 主循环内：
#ifdef __SWITCH__
if (prevFastForward && !ff) {
    beiklive::AudioManager::instance().flushRingBuffer();
}
prevFastForward = ff;
#endif
```

---

## 技术对比

| 指标 | 旧方案 | 新方案 |
|------|-------|-------|
| 硬件缓冲区数量 | 2 | 4 |
| 提交方式 | `audoutPlayBuffer`（阻塞） | `audoutAppendAudioOutBuffer`（非阻塞）|
| 音频线程等待策略 | 阻塞至每个 buffer 播完 | 非阻塞检查 + 10 ms 超时等待 |
| 快进结束清理 | 无 | `flushRingBuffer()` 丢弃过期样本 |
| 游戏线程被拖慢风险 | 高（ring 易满） | 低（音频线程消耗更快，ring 更空）|
| 总硬件延迟（约） | 2 × 10.67 ms ≈ 21 ms | 4 × 10.67 ms ≈ 43 ms |

> 注：新方案总硬件延迟略有增加，但在游戏场景下 43 ms 的延迟完全在可接受范围内（一般认为 <80 ms 无感知）。

---

## 影响范围

- 仅 Switch 平台（`#ifdef __SWITCH__`）的 AudioManager 后端受影响
- `flushRingBuffer()` 为跨平台方法，但目前仅在 game_view.cpp 的 `#ifdef __SWITCH__` 块中调用
- Windows / Linux / macOS 音频后端**不受任何影响**
