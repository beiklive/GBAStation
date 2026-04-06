# GBAStation 工作汇报

## 2026-04-06 修复两个问题

### 任务分析

#### 问题1：保存/读取状态面板焦点位置不正确
**目标**：每次进入保存/读取状态面板时，焦点应聚焦在第一个 item 上，而非上次选择的位置。

**根因分析**：
- 点击"保存状态"/"读取状态"按钮时，调用 `brls::Application::giveFocus(sonPanel)`
- 由于 `sonPanel` 设为不可聚焦（`setFocusable(false)`），borealis 会调用 `sonPanel->getDefaultFocus()` 来寻找可聚焦子视图
- borealis 的 `ScrollingFrame` 会记忆上次聚焦的子视图（last focused child），导致焦点恢复到之前选择的位置
- 解决方案：绕过 `getDefaultFocus()` 的记忆机制，直接给第一个 `LazyCell`（`grid->getItemView(0)`）赋予焦点

**修改文件**：
- `src/ui/utils/GameMenuView.hpp`：添加 `m_saveGrid`/`m_loadGrid` 成员变量，`_createMenuButton` 增加 `firstFocusView` 参数
- `src/ui/utils/GameMenuView.cpp`：
  - `_createSaveStatePanel()`/`_createLoadStatePanel()` 中保存 grid 指针
  - `_createMenuButton()` 增加 `firstFocusView` 参数，点击和右导航时优先聚焦指定视图
  - `_initLayout()` 中传入 `m_saveGrid->getItemView(0)` / `m_loadGrid->getItemView(0)` 作为第一聚焦目标

#### 问题2：可视化倒带每次关闭后缓存变少
**目标**：多次打开可视化倒带时，item 数量不应大幅减少。

**根因分析**：
倒带帧恢复逻辑存在 bug：
```cpp
// 错误代码：
while (static_cast<int>(m_rewindBuffer.size()) > restoreIdx)
    m_rewindBuffer.pop_front();
```
- 目的：删除比选中帧更新的帧（下标 0..restoreIdx-1）
- 实际效果：保留 `restoreIdx` 个帧，删除其余，方向完全相反
- 特别是当 `restoreIdx == 0`（选中最新帧）时，`while(size() > 0)` 会清空**整个**缓冲区
- 正确做法：从队首 pop_front `restoreIdx` 次

**修改文件**：
- `src/ui/utils/GameView.cpp`：将 `while (size() > restoreIdx) pop_front()` 改为 `for (i=0; i<restoreIdx; i++) pop_front()`

**修复效果**：
- 选中最新帧（restoreIdx=0）：缓冲区不变（不再错误清空）
- 选中第 k 帧（restoreIdx=k）：正确删除 k 个更新的帧，保留第 k 帧及更旧的帧

---

## 2026-04-07 修复lastPlayed排序格式和GameMenuView崩溃

### 任务分析

#### 任务目标
1. 修复 startpage 游戏排序顺序异常（lastPlayed 字段格式问题）
2. 修复 GameMenuView 保存状态后退出游戏时程序崩溃

#### 输入输出
- 输入：问题报告及代码库
- 输出：修复后的 Tools.cpp/hpp、GameLibraryPage.cpp/hpp、DataManagementPage.cpp/hpp、GamePage.cpp

---

### 问题1：lastPlayed 字段格式导致排序异常

**根因分析**：
- `getTimestampString()` 返回格式为 `"26-04-06 16时%M分"` 含中文字符且无秒
- 字符串字典序排序时，中文字节（UTF-8 多字节）导致潜在平台兼容性问题
- 同分钟内游玩多次时，时间戳完全相同，排序不稳定

**修复方案**：
- `getTimestampString()` 改为返回 `"%y-%m-%d %H-%M-%S"` 格式（如 `"26-03-31 09-38-11"`）
  - 纯 ASCII 字符，字符串比较可靠
  - 精确到秒，排序稳定
- 新增 `formatTimestampForDisplay(ts)` 函数：将存储格式转换为 `"26-04-06 16时13分"` 显示格式
  - 含输入范围验证（月/日/时/分/秒合法性检查）
  - 解析失败时原样返回（向后兼容旧格式数据）
- 更新 `GameLibraryPage.cpp` 和 `DataManagementPage.cpp` 在显示时调用 `formatTimestampForDisplay()`

**修改文件**：
- `src/core/Tools.hpp`：添加 `formatTimestampForDisplay()` 声明
- `src/core/Tools.cpp`：修改 `getTimestampString()` 格式，实现 `formatTimestampForDisplay()`
- `src/ui/page/GameLibraryPage.hpp`：添加 `#include "core/Tools.hpp"`
- `src/ui/page/GameLibraryPage.cpp`：使用 `formatTimestampForDisplay()` 显示时间
- `src/ui/page/DataManagementPage.hpp`：添加 `#include "core/Tools.hpp"`
- `src/ui/page/DataManagementPage.cpp`：使用 `formatTimestampForDisplay()` 显示时间

---

### 问题2：GameMenuView 保存状态后退出崩溃

**崩溃流程分析**：
1. 用户聚焦"保存状态"按钮 → `onFocusGainedCallback` 触发 `_refreshStatePanel(true)`
2. `_refreshStatePanel` 启动 `brls::async` 后台线程，循环调用 `infoCallback(0..9)`
3. `infoCallback` 是在 `GamePage::GameMenuInitialize()` 中设置的 lambda，捕获了 `this`（GamePage 指针）和 `m_gameView`（GameView 原始指针）
4. 用户保存存档 → `m_onResume()` 返回游戏 → 再次打开菜单 → 点击退出
5. `GameSignal::requestExit()` → `GameView::draw()` 检测到退出信号 → `brls::sync([this]{ Application::popActivity() })` 
6. 下一帧：`popActivity()` 销毁 Activity → GamePage 被删除 → 子视图 GameView 被删除
7. **同时**：后台线程仍在调用 `infoCallback(slot)` 访问已销毁的 `m_gameView` → **Use-After-Free 崩溃**

**修复方案**：
在 `GamePage::GameMenuInitialize()` 中：
- 在 UI 线程（构造时）预先计算所有 10 个槽位的存档路径和缩略图路径（`getStatePath(slot)` / `getStateThumbPath(slot)` 均为字符串操作）
- 将路径通过 `std::vector<std::string>` 值捕获传入 `setStateInfoCallback` 的 lambda
- 后台线程只对字符串路径做文件系统操作，不再持有任何 GameView/GamePage 原始指针

**修改文件**：
- `src/ui/page/GamePage.cpp`：`GameMenuInitialize()` 中预计算路径并值捕获


---

## 2026-04-06 修复音频撕裂音、倒带缓存计算错误和倒带UI音调偏高

### 任务分析

#### 问题1：游戏启动/退出时有严重撕裂音

**目标**：消除游戏启动和退出时的音频撕裂噪声。

**根因分析**：
`BKAudioPlayer` 和 `AudioManager` 共用同一个 Switch audout 硬件设备，导致以下竞争问题：

- **启动撕裂音**：用户点击游戏时 BKAudioPlayer 播放点击音，约 0-50ms 后 `AudioManager::init()` 开始初始化。AudioManager 的音频线程调用 `audoutWaitPlayFinish`（timeout=0）时可能"偷走" BKAudioPlayer 的完成通知。BKAudioPlayer 等待完整 `waitNs` 超时后才 `free(rawBuf)`，但此时内存可能已被 AudioManager 重用，硬件 DMA 读取新数据产生撕裂音（use-after-free）。

- **退出撕裂音**：游戏退出时音频线程停止后，硬件队列中仍有 1-3 个未播完的 512帧缓冲区，`audoutStopAudioOut()` 直接截断这些缓冲区，产生爆音。

**修改方案**：
1. **`BKAudioPlayer::playSoundDirect` (Switch)**：检测 `AudioManager::instance().isRunning()`，若游戏音频系统运行中则跳过播放，彻底避免 audout 设备争用
2. **`BKAudioPlayer`**：添加 `m_isPlaying` 原子标志 + `isPlaying()` 方法，用于外部等待
3. **`GameView::_registerGameRuntime()`**：启动 `AudioManager` 前先等待 `BKAudioPlayer` 完成（最多 500ms），确保硬件队列清空
4. **`AudioManager::deinit()` (Switch)**：join 音频线程后，循环调用 `audoutWaitPlayFinish` 排空硬件缓冲区（超时 200ms/次），再调用 `audoutStopAudioOut()`

#### 问题2：可视化倒带可倒回两分钟，但最大缓存设置仅1分钟

**目标**：确保"约1分钟"的倒带设置实际只缓存1分钟。

**根因分析**：
设置页面标签 `"3600（约1分钟）"` 假设 saveInterval=1（每帧保存一次，60fps × 60s = 3600帧）。但当用户将 `m_rewindSaveInterval` 设为 2 时，每个缓冲区条目覆盖 2 帧，实际存储时长变为 3600 × 2/60 = 120秒 = 2分钟，与标签不符。

**修改方案**：
- **`GameView::_saveRewindState()`**：改为 `maxEntries = m_rewindBufferSize / m_rewindSaveInterval` 限制条目数，确保实际缓冲时长始终为 `m_rewindBufferSize / 60` 秒。

#### 问题3：运行游戏时可视化倒带界面item的音效音调偏高

**根因分析**：与问题1相同的 use-after-free 问题。BKAudioPlayer 的音效缓冲区被 free 后内存被重用，硬件 DMA 读到新数据，播放速度异常，表现为音调偏高。

**修改方案**：同问题1修复1（BKAudioPlayer 在游戏运行时跳过播放），彻底消除该问题。

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/ui/audio/BKAudioPlayer.hpp` | 添加 `m_isPlaying` 原子标志和 `isPlaying()` 方法 |
| `src/ui/audio/BKAudioPlayer.cpp` | Switch平台检测AudioManager运行状态跳过播放；追踪m_isPlaying状态 |
| `src/ui/utils/GameView.cpp` | 启动AudioManager前等待BKAudioPlayer完成；倒带缓冲区按saveInterval修正 |
| `src/game/audio/AudioManager.cpp` | deinit时排空硬件缓冲区后再停止 |

---

## 2026-04-06 修复音频撕裂与刺耳声问题

### 任务分析

#### 问题1：退出游戏再打开后声音撕裂

**目标**：第二次及以后进入游戏时，音频应与第一次一样正常。

**根因分析**：
`AudioManager::deinit()` 中调用 `m_ring.clear()` 清空了环形缓冲区的数据，但 `m_writePos`、`m_readPos`、`m_available` 三个状态变量**未被重置**。第二次调用 `init()` 时，`m_ring.resize(RING_CAPACITY)` 重新分配全零的缓冲区，但三个变量依然保留上次会话末尾的旧值。

若上次游戏退出时 `m_available > 0`（环中有未消费的数据），则：
1. 音频线程在第二次启动后立即满足 `m_available >= needed` 条件，从 `m_readPos` 开始读取——此时读到的是全零的环内容（silence）
2. 与此同时，游戏线程向 `m_writePos` 写入真实音频数据
3. 当音频线程的 `m_readPos` 追上 `m_writePos` 时，同一批输出包含了零（静音）→真实音频的突变，产生不连续的撕裂声/刺耳声

**修复**：在所有平台的 `init()` 中（Switch/ALSA/WinMM/CoreAudio/兜底），在 `m_ring.resize()` 之前显式重置：
```cpp
m_writePos          = 0;
m_readPos           = 0;
m_available         = 0;
m_maxLatencySamples = RING_CAPACITY / 2;
```

#### 问题2：游戏启动时的刺耳声

**根因分析**：
- 同问题1：stale `m_available` 导致起始时音频帧含混合数据块
- 另外，`retro_load_game` / `retro_reset` 阶段可能在 `LibretroLoader::m_audioBuffer` 中积累少量初始化音频（如 BIOS 启动音），这些数据会被游戏循环第一帧的 `DrainAudio()` 取出并推送到 `AudioManager`，引发起始噪音

**修复**：`AudioManager::init()` 调用完成后，立即 drain 并丢弃任何初始化阶段积累的音频数据，确保游戏循环从干净状态开始推送音频：
```cpp
{
    std::vector<int16_t> discard;
    m_gba_core->DrainAudio(discard);
}
```

#### 附加：ALSA `deinit()` 缺失 `m_dataCV.notify_all()`

ALSA 平台的 `deinit()` 缺少 `m_dataCV.notify_all()`，与 Switch 平台不一致，补充该调用以保证一致性（防止未来添加等待逻辑时产生死锁）。

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/game/audio/AudioManager.cpp` | 所有平台 `init()` 中重置环形缓冲区状态变量；ALSA `deinit()` 添加 `m_dataCV.notify_all()`；兜底 `deinit()` 添加 `m_dataCV.notify_all()` |
| `src/ui/utils/GameView.cpp` | `_registerGameRuntime()` 中 `AudioManager::init()` 后 drain 并丢弃初始化音频数据 |
