# BeikLiveStation 第三轮问题修复报告

## 修复概述

本轮修复针对五个已知问题，修改文件如下：

- `include/Audio/AudioManager.hpp`
- `src/Audio/AudioManager.cpp`
- `src/UI/game_view.cpp`

---

## 问题一：倒带时画面不更新（实时绘制）

### 问题描述

倒带时调用 `m_core.unserialize()` 恢复状态后，没有调用 `m_core.run()`，导致视频回调没有被触发，视频帧不更新，画面冻结在倒带开始前的最后一帧。

### 修复方案

在倒带逻辑中，恢复状态后（释放 `m_rewindMutex` 锁之后）立即调用 `m_core.run()`，触发视频回调，产生新的视频帧。

```cpp
// 恢复状态
{
    std::lock_guard<std::mutex> lk(m_rewindMutex);
    for (unsigned step = 0; ...) {
        m_core.unserialize(...);
        m_rewindBuffer.pop_front();
        didRestore = true;
    }
}
// 调用 run() 更新视频帧
if (didRestore) {
    m_core.run();
}
```

---

## 问题二：键盘按键映射可读性（枚举值代替整数）

### 问题描述

配置文件中键盘按键映射以原始整数值存储（如 `keyboard.a = 88`），难以阅读。

### 修复方案

1. 新增 `k_kbdKeyNames[]` 查找表，将常见按键的枚举名称映射到扫描码整数值。  
   支持字母键（A-Z）、数字键（0-9）、方向键、功能键及特殊键（SPACE、ENTER、TAB、ESC、GRAVE 等）。

2. 新增 `parseKbdScancode(const std::string&)` 辅助函数：  
   先按名称查找（如 `"X"` → 88），再回退解析为整数字符串，向后兼容旧配置。

3. 配置文件默认值改为使用字符串名称：
   ```ini
   keyboard.a = X         ; 而不是 88
   keyboard.b = Z         ; 而不是 90
   keyboard.fastforward = TAB   ; 而不是 258
   keyboard.rewind = GRAVE      ; 而不是 96
   ```

4. `loadButtonMaps()` 和 `pollInput()` 中的键值解析均更新为优先尝试名称查找，再回退整数解析。

---

## 问题三：倍速功能的倍率设置无效，始终以最大速率加速

### 问题描述

原代码在快进且 `multiplier >= 1.0` 时，直接跳过帧率控制睡眠（"run as fast as possible"），导致实际速度取决于 CPU 性能，而非配置的倍率。

```cpp
// 原有错误逻辑
if (ff && m_ffMultiplier >= 1.0f) {
    // Run as fast as possible  ← 忽略倍率值
}
```

### 修复方案

移除"尽可能快"的特殊分支，统一使用带精确时序控制的睡眠逻辑：

- 快进时，每次迭代运行 `round(multiplier)` 帧
- 同时保留 `frameDuration` 的帧时间限制

实际效果：在一个正常帧周期内运行 `N` 帧，有效速度恰为 `N × fps`，与配置倍率精确对应。

```
// 2x 快进：每个 16.67ms 内运行 2 帧 → 精确 2x 速度 ✓
// 4x 快进：每个 16.67ms 内运行 4 帧 → 精确 4x 速度 ✓
```

---

## 问题四：Windows 默认帧率只有四十多帧

### 问题描述

Windows 系统 `std::this_thread::sleep_for()` 的定时器精度默认为约 15ms。目标帧周期约 16.67ms，每次休眠可能实际休眠约 15-30ms，导致帧率仅有约 40fps。

### 修复方案

**双重修复**：

1. **`timeBeginPeriod(1)`**：在游戏线程启动时调用，将 Windows 系统定时器精度提升至 1ms；退出时调用 `timeEndPeriod(1)` 恢复。

2. **混合睡眠策略（Hybrid sleep）**：  
   - 粗粒度睡眠：睡眠 `(目标时间 - 当前时间 - 2ms)` 以降低 CPU 占用
   - 精确自旋等待：最后 2ms 使用忙等循环精确控制帧边界

```cpp
auto sleepTarget = frameStart + targetDur;
auto nowPre = Clock::now();
if (nowPre < sleepTarget) {
    // 粗粒度睡眠（保留 2ms 自旋预算）
    auto coarseDur = (sleepTarget - nowPre) - spinGuard;
    if (coarseDur.count() > 0)
        std::this_thread::sleep_for(coarseDur);
    // 精确自旋等待至目标时间点
    while (Clock::now() < sleepTarget) {}
}
```

效果：在 Windows 无 `timeBeginPeriod` 时，15ms 粗睡 + 1.67ms 自旋 ≈ 16.67ms ✓  
加上 `timeBeginPeriod(1)` 后睡眠更精确，自旋时间更短。

---

## 问题五：PC 与 Switch 音频始终有拖慢（重点修复）

### 问题描述

音频延迟（拖慢）的根本原因有三：

1. **环形缓冲区过大**：原 `RING_CAPACITY = 32768 × 2 = 65536`（约 1 秒音频），允许大量样本积压，产生高达 1 秒的延迟。
2. **硬件缓冲区过大**：
   - WinMM：4 × 1024 帧 = 125ms 硬件输出延迟
   - Switch：1024 帧/缓冲 ≈ 21ms，但积累仍导致延迟
3. **无背压（backpressure）控制**：游戏线程可以无限比音频线程快，环形缓冲区持续填满，音频播放的是"旧"样本。

### 修复方案

#### 1. 条件变量背压控制（核心修复）

修改 `AudioManager::pushSamples()` 为阻塞式：当环形缓冲区填充量超过阈值时，等待音频线程消耗部分数据。

```cpp
void AudioManager::pushSamples(const int16_t* data, size_t frames)
{
    if (!m_running) return;
    std::unique_lock<std::mutex> lk(m_mutex);
    // 超过阈值时阻塞，将游戏速度与音频输出速率同步
    m_spaceCV.wait(lk, [&] {
        return m_available + count <= m_maxLatencySamples || !m_running;
    });
    if (!m_running) return;
    ringWrite(data, count);
}
```

音频线程每次 `ringRead()` 后通知条件变量，唤醒等待的游戏线程：

```cpp
size_t AudioManager::ringRead(int16_t* out, size_t maxCount)
{
    // ...
    m_available -= n;
    if (n > 0)
        m_spaceCV.notify_all();  // 通知生产者可以继续写入
    return n;
}
```

#### 2. 动态阈值设置

游戏线程启动时，根据核心帧率动态计算音频延迟阈值（约 4 帧音频）：

```cpp
size_t maxLatencyFrames = static_cast<size_t>(32768.0 / coreFps * 4.0 + 0.5);
AudioManager::instance().setMaxLatencyFrames(maxLatencyFrames);
// 对 GBA (59.73fps): ≈ 2194 帧 = 约 67ms 延迟上限
```

#### 3. 减小硬件缓冲区尺寸

| 平台 | 修改前 | 修改后 | 延迟改善 |
|------|--------|--------|----------|
| WinMM | 4 × 1024 帧 = 125ms | 3 × 512 帧 = 47ms | -78ms |
| Switch | 1024 帧/缓冲 | 512 帧/缓冲 | 减半 |
| ALSA (Linux) | 512 帧/周期 | 256 帧/周期 | 减半 |

#### 4. 减小环形缓冲区容量

```cpp
// 修改前
static constexpr size_t RING_CAPACITY = 32768 * 2; // 65536 样本，~1s
// 修改后
static constexpr size_t RING_CAPACITY = 8192;      // ~125ms at 32768Hz stereo
```

#### 5. 防止关闭时死锁

原来 `cleanup()` 的顺序：先停止游戏线程，再停止音频管理器。  
但游戏线程可能正在 `pushSamples()` 内阻塞等待，导致死锁。

修复：**先调用 `AudioManager::deinit()`**（通知条件变量，唤醒等待线程），再停止游戏线程。

所有平台的 `deinit()` 都已更新为在停止前调用 `m_spaceCV.notify_all()`。

---

## 修改文件汇总

| 文件 | 修改内容 |
|------|---------|
| `include/Audio/AudioManager.hpp` | 新增 `std::condition_variable m_spaceCV`；减小 `RING_CAPACITY` 至 8192；新增 `m_maxLatencySamples` 及 `setMaxLatencyFrames()`；`pushSamples()` 文档更新 |
| `src/Audio/AudioManager.cpp` | `pushSamples()` 改为阻塞式；`ringRead()` 通知条件变量；所有 `deinit()` 通知条件变量；减小 WinMM/Switch/ALSA 缓冲区大小 |
| `src/UI/game_view.cpp` | 修复倒带视频（增加 `run()` 调用）；新增键盘扫描码名称查找表及 `parseKbdScancode()`；配置默认值改为字符串名称；快进倍率精确帧时序；Windows `timeBeginPeriod(1)` + 混合睡眠；正确的 cleanup 顺序（先 deinit 音频）；游戏线程启动时设置音频延迟阈值 |
