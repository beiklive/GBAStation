# 工作汇报：倒带快进移植、帧率修复、动画类、游戏循环重构

**日期**：2026-04-01  
**分支**：copilot/old-gameview

---

## 任务分析

### 任务目标
1. **移植old的倒带和快进功能到GameView中**：将快进→正常切换时的音频刷新行为推广到所有平台（原仅限 Switch 的 `#ifdef __SWITCH__` 块），同时将相关状态变量统一管理。
2. **帧率控制优化**：当前非快进状态下游戏帧率低于55，根本原因是帧率限制器以"帧结束时间"为基准，当音频推送（`pushSamples()`）阻塞时，会将阻塞时间错误地加到帧耗时上，导致"帧超预算"而跳过睡眠。修复方案：在每帧**开始**处记录 `frameStart`，以 `frameStart + frameDuration` 为截止时间精确控制帧率。
3. **动画类设计**：创建 `BKAnimator` 工具类，统一封装视图淡入/淡出动画以及 Activity 切换动画，避免各处直接调用 `setVisibility()`、`pushActivity()`，实现入场/出场动画效果。
4. **游戏循环代码整理**：原 `startGameThread()` 内的 lambda 超过250行，包含仿真执行、音频处理、定时器、存读档等多个职责。提取为独立私有成员函数，提升可读性和可维护性。

### 输入/输出
- **输入**：已有 `game_view.cpp`（2429行），`GameMenu.cpp`，`StartPageView.cpp`
- **输出**：
  - 新增 `include/UI/Utils/BKAnimator.hpp`
  - 新增 `src/UI/Utils/BKAnimator.cpp`
  - 修改 `include/Game/game_view.hpp`（新增计时器成员和游戏循环子函数声明）
  - 修改 `src/Game/game_view.cpp`（游戏循环重构、帧率修复、音频刷新跨平台化）
  - 修改 `src/UI/Utils/GameMenu.cpp`（菜单关闭动画）
  - 修改 `src/UI/StartPageView.cpp`（Activity推入动画）

### 挑战与解决方案
- **帧率问题**：`nowPost = Clock::now()` 在音频推送后捕获，音频阻塞直接影响帧计时。  
  → 改为在帧**开始**记录 `frameStart`，以 `frameStart + targetDuration` 为截止时间，各帧独立计算睡眠量，消除跨帧积累漂移。
- **动画API约束**：borealis 的 `view->alpha` 为 `protected` 成员，无法从外部直接动画化。  
  → 利用 borealis 已有的 `View::show()` / `View::hide()` 方法（内部使用 `Animatable alpha`），在调用前后配合 `setVisibility()` 实现淡入/淡出效果。
- **游戏循环重构**：循环内的局部变量（`fpsTimerStart`、`playtimeTimer` 等）需迁移到成员变量，才能被提取的子函数访问。  
  → 在 `game_view.hpp` 中新增 `m_loop*` 前缀的计时器成员变量，在 `startGameThread()` 中初始化，子函数直接访问。

---

## 实施内容

### 任务1：倒带和快进功能（跨平台音频刷新）

**变更位置**：`src/Game/game_view.cpp` `startGameThread()` lambda 内

**原代码**（仅Switch平台）：
```cpp
#ifdef __SWITCH__
bool prevFastForward = false;
// ...循环内...
if (!paused && prevFastForward && !ff) {
    beiklive::AudioManager::instance().flushRingBuffer();
}
prevFastForward = paused ? false : ff;
#endif
```

**修改后**（跨平台，使用成员变量 `m_loopPrevFf`）：
```cpp
// 快进结束时刷新音频环形缓冲区（跨平台）
if (!paused && m_loopPrevFf && !ff) {
    beiklive::AudioManager::instance().flushRingBuffer();
}
m_loopPrevFf = paused ? false : ff;
```

### 任务2：帧率控制修复

**变更位置**：`src/Game/game_view.cpp` 新增 `gameLoopWaitForDeadline()`

**核心思路**：以帧**开始时间**为基准，使每帧独立计算截止时间，消除前帧耗时对后帧的影响。

```cpp
void GameView::gameLoopWaitForDeadline(
    std::chrono::steady_clock::time_point frameStart,
    std::chrono::duration<double> targetDuration)
{
    auto deadline = frameStart + duration_cast<Clock::duration>(targetDuration);
    auto nowNow   = Clock::now();
    if (nowNow >= deadline) return;
    // 粗粒度睡眠 + 精确自旋等待
    auto coarseDur = (deadline - nowNow) - duration_cast<Clock::duration>(Duration(SPIN_GUARD_SEC));
    if (coarseDur.count() > 0) sleep_for(coarseDur);
    while (Clock::now() < deadline) { /* 忙等待 */ }
}
```

游戏循环中在迭代顶部捕获帧开始时间：
```cpp
auto frameStart = Clock::now();
// ... 所有工作 (pollInput, 仿真, 音频) ...
gameLoopWaitForDeadline(frameStart, targetDur);
```

### 任务3：BKAnimator 动画工具类

**新增文件**：
- `include/UI/Utils/BKAnimator.hpp`
- `src/UI/Utils/BKAnimator.cpp`

**接口**：
```cpp
namespace beiklive {
class BKAnimator {
public:
    static void showView(brls::View*, std::function<void()> = {}, int durationMs = 200);
    static void hideView(brls::View*, std::function<void()> = {}, int durationMs = 200);
    static void pushActivity(brls::Activity*);
    static void popActivity(std::function<void()> = {});
};
}
```

**实现要点**：
- `showView`：先设 VISIBLE，再调用 `view->show(cb, true, duration)` 淡入
- `hideView`：调用 `view->hide([view,cb]{ view->setVisibility(GONE); cb(); }, true, duration)` 淡出后设 GONE
- `pushActivity`：使用 `TransitionAnimation::SLIDE_LEFT` 滑动入场
- `popActivity`：使用 `TransitionAnimation::FADE` 淡出退场

**使用位置**：
- `GameMenu.cpp`：B键/返回按钮关闭菜单改用 `BKAnimator::hideView`
- `game_view.cpp` draw()：打开菜单改用 `BKAnimator::showView`
- `StartPageView.cpp`：`openFileBrowserPage`、`openSettingsPage`、`openDataPage`、`openGameLibraryPage`、`openAboutPage` 改用 `BKAnimator::pushActivity`；游戏启动 `launchGameActivity` 改为 `TransitionAnimation::FADE`

### 任务4：游戏循环代码重构

**提取的私有方法**：

| 方法名 | 职责 |
|--------|------|
| `gameLoopRunSimulation(ff, rew, paused)` | 执行核心仿真（快进/倒带/暂停逻辑，倒带缓冲管理），返回本次帧数 |
| `gameLoopPushAudio(ff, paused)` | 排空核心音频缓冲并按条件推送（静音/限流） |
| `gameLoopUpdateTimers(paused, now, frameCount)` | FPS计数、游戏时长追踪、RTC同步、自动存档 |
| `gameLoopProcessPendingActions()` | 处理即时存档/读档/游戏重置请求 |
| `gameLoopWaitForDeadline(frameStart, targetDur)` | 帧率精确限制器 |

**游戏循环主体（简化后）**：
```cpp
while (running) {
    auto frameStart = Clock::now();
    pollInput();
    // 快进音频刷新
    if (!paused && m_loopPrevFf && !ff) AudioManager::flushRingBuffer();
    m_loopPrevFf = paused ? false : ff;
    // 仿真 + 音频 + 定时器 + 待处理操作
    unsigned frames = gameLoopRunSimulation(ff, rew, paused);
    gameLoopPushAudio(ff, paused);
    gameLoopUpdateTimers(paused, Clock::now(), frames);
    if (!paused) gameLoopProcessPendingActions();
    gameLoopWaitForDeadline(frameStart, targetDur);
}
```

---

## 验证结果
- 编译成功（Linux Desktop Release，无新增错误）
- 预期效果：
  - 非快进帧率维持在 ~59.73fps（GBA原生帧率）
  - 快进→正常切换时音频不再有爆音/噪声（跨平台音频刷新）
  - 游戏菜单开/关有淡入/淡出动画效果
  - 各页面跳转有滑动入场动画
