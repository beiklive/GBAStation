# 快进功能卡死问题修复报告

## 问题描述

当配置文件中设置 `fastforward.mute = false` 且 `fastforward.multiplier = 4` 时，
使用快进功能会导致游戏卡死（主线程无响应）。

## 根本原因分析

### 代码路径

文件：`src/UI/game_view.cpp`，函数 `startGameThread()`（游戏主循环）。

### 执行流程

1. **快进时运行多帧**：当快进激活且 `m_ffMultiplier = 4` 时，每次循环迭代运行 4 帧游戏逻辑
   (`framesThisIter = 4`)。
2. **生成 4 倍音频数据**：运行 4 帧会产生约 **4 倍**于正常量的音频采样（约 4 × 544 = 2176
   帧，以 32768 Hz / 60 fps 计算）。
3. **阻塞写入音频缓冲区**：`AudioManager::pushSamples()` 内部使用条件变量等待，当环形缓冲区
   可用空间不足时会**阻塞**：
   ```cpp
   m_spaceCV.wait(lk, [&] {
       return m_available + count <= m_maxLatencySamples || !m_running;
   });
   ```
4. **缓冲区溢出**：最大延迟阈值设为约 4 帧音频（`startGameThread` 中 `maxLatencyFrames ≈ 4`
   帧），而每次循环尝试推送 **4 帧**音频。仅仅 1~2 次迭代后缓冲区即告满载。
5. **死锁**：音频硬件以 1× 实时速度消耗采样，而游戏线程以 4× 速度生成。缓冲区持续满载，
   `pushSamples()` 永远阻塞，游戏线程卡死。

## 修复方案

**位置**：`src/UI/game_view.cpp`，快进音频推送逻辑（原第 786–789 行）。

**修复思路**：当快进激活且倍率 > 1 且 `mute = false` 时，将推送的音频采样限制为 **1 帧正常量**
（即 `总采样数 / 倍率`），丢弃多余部分。这样每个实时帧推送的音频量与正常模式相同，环形缓冲区
不会溢出，`pushSamples()` 不再阻塞。

**修改前**：
```cpp
if (!mute) {
    size_t frames = samples.size() / STEREO_CHANNELS;
    beiklive::AudioManager::instance().pushSamples(samples.data(), frames);
}
```

**修改后**：
```cpp
if (!mute) {
    size_t frames = samples.size() / STEREO_CHANNELS;
    // During fast-forward (multiplier > 1) with audio not muted,
    // limit the audio pushed to one normal frame's worth of samples.
    // Running N frames per loop iteration generates N× the usual
    // audio, which would saturate the ring buffer and cause
    // pushSamples() to block indefinitely, freezing the game thread.
    if (ff && m_ffMultiplier > 1.0f) {
        size_t limit = frames /
            static_cast<size_t>(std::round(m_ffMultiplier));
        // If limit rounds to zero the total is already tiny; push all.
        if (limit > 0)
            frames = limit;
    }
    beiklive::AudioManager::instance().pushSamples(samples.data(), frames);
}
```

## 修复效果

| 场景 | 修复前 | 修复后 |
|------|--------|--------|
| `mute=true, multiplier=4` | 正常（音频被静音） | 正常（无变化） |
| `mute=false, multiplier=4` | 游戏卡死 | 正常，音频以实时速率播放 |
| `mute=false, multiplier=1` | 正常 | 正常（无变化，`> 1.0f` 条件不触发） |
| `mute=false, multiplier=0.5`（慢动作）| 正常 | 正常（无变化，`> 1.0f` 条件不触发） |
| `mute=false, multiplier=8` | 游戏卡死 | 正常，推送 1/8 音频量 |

## 影响范围

- 仅修改 `src/UI/game_view.cpp` 一处代码（约 8 行）。
- 不影响 `mute=true` 路径（已提前跳过）。
- 不影响倍率 ≤ 1 的慢动作模式。
- 不影响正常（非快进）播放。
- 不影响倒带功能。
