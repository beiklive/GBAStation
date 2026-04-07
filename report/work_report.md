# GBAStation 工作汇报

## 2026-04-07 修复 Cannot set texture 和返回主页崩溃 + 联机可行性研究

### 任务分析

#### 任务目标
1. 修复每次页面都会有 `Cannot set texture: 0` 的错误日志
2. 修复从游戏返回主页时程序经常崩溃的问题
3. 研究基于 mgba libretro 实现联机功能的可行性

#### 问题1：`Cannot set texture: 0` 每次页面都出现

**根因分析**：

错误来源于 borealis 的 `Image::innerSetImage(int tex)` 函数（`third_party/borealis/library/lib/views/image.cpp`），
当 `nvgCreateImage()` 返回 0（文件不存在或加载失败）时记录该错误。

追踪调用链：
- `Box::setupBackgroundLayer()` (`src/ui/utils/Box.cpp:72`) 调用：
  ```cpp
  backgroundLayer->setImageFromFile("resources\\img\\bg2.png");
  ```
- 该路径使用了 **Windows 反斜杠硬编码路径**，在 Switch 平台（romfs）和 Linux 均无法找到文件
- 正确做法是使用 `BK_RES("img/bg2.png")` 宏，它在 Switch 上展开为 `romfs:/img/bg2.png`，在 Windows 上展开为 `./resources/img/bg2.png`
- `Box` 类作为所有页面的基础容器，每个页面实例化时都会触发此错误，因此"每次页面"都会出现

**修复方案**：
- 在 `Box.cpp` 中添加 `#include "core/common.h"` 以获得 `BK_RES` 宏
- 将硬编码路径改为 `BK_RES("img/bg2.png")`

**修改文件**：
- `src/ui/utils/Box.cpp`：引入 `common.h`，修正背景图路径

---

#### 问题2：从游戏返回主页时程序崩溃

**根因分析**：

崩溃原因是 `GameMenuView::_refreshStatePanel()` 中存在**异步任务与 UI 线程之间的竞态条件**：

```cpp
ASYNC_RETAIN
brls::async([ASYNC_TOKEN, infoCallback, isSave]() {
    // ... 后台线程：查询存档信息 ...
    ASYNC_RELEASE             // ← 在此处检查 this 是否已析构
    brls::sync([this, ...]()  // ← 但投递 sync 时 this 可能已被释放！
    {
        // 访问 this 成员 → Use-After-Free 崩溃
    });
});
```

**竞态窗口**：
1. 后台线程执行 `bool release = *token`（读到 false，视图存活）
2. UI 线程析构 `GameMenuView`（`*deletionToken` 设为 true）
3. 后台线程：`if (release) return;` → release 是旧的 false → **不返回**
4. 后台线程投递 `brls::sync([this, ...]())`
5. UI 线程在下一帧执行 sync 回调 → `this` 已被释放 → **崩溃**

**触发场景**：
- 用户打开"保存状态"面板 → 触发 `_refreshStatePanel` 异步扫描
- 用户快速退出游戏（保存或不保存后点击"退出游戏"）
- 退出动画结束 → `GamePage` 析构 → `GameMenuView` 析构
- 后台扫描线程尚未完成，在竞态窗口内向 UI 队列投递含悬空 `this` 的 sync 回调

**修复方案**：

参考 borealis 官方示例（`Image::setImageAsync` 使用的模式），
将 `ASYNC_RELEASE` **移入 `brls::sync` 回调内部**，在 UI 线程执行时检查视图状态：

```cpp
ASYNC_RETAIN
brls::async([ASYNC_TOKEN, infoCallback, isSave]() {
    // ... 后台扫描 ...
    brls::sync([ASYNC_TOKEN, infos = std::move(infos), isSave]() {
        ASYNC_RELEASE  // ← UI 线程执行：若视图已析构则提前 return
        // 安全访问 this 和 m_saveItems/m_loadItems
    });
});
```

**正确性保证**：
- `brls::sync` 回调在 UI 线程执行，视图析构也在 UI 线程执行
- 两者不能并发运行：若 sync 回调开始执行时 `*token == false`（视图存活），
  则视图**不可能在回调执行期间被析构**（UI 线程是单线程顺序执行的）
- 因此 `ASYNC_RELEASE` 在 sync 回调内部提供了**正确且无竞态的析构检测**

**修改文件**：
- `src/ui/utils/GameMenuView.cpp`：`_refreshStatePanel()` 中将 `ASYNC_RELEASE` 移入 `brls::sync` lambda

---

#### 任务3：mgba libretro 联机功能可行性研究

已完成研究并提交报告：`report/mgba_libretro_netplay_feasibility.md`

**结论摘要**：
- **技术上完全可行**，mgba 提供完善的 SIO/Lockstep 接口和跨平台 Socket 工具
- **推荐方案 B**（libretro 层回滚联机）：不修改 mgba 核心，工作量较小
- **方案 A**（真实 SIO 网络仿真）：兼容性更好，但延迟敏感（仅适合局域网）
- 建议先实现局域网回滚输入共享，再逐步迭代

### 修改文件汇总

| 文件 | 修改内容 |
|------|---------|
| `src/ui/utils/Box.cpp` | 引入 `common.h`；修正背景图路径为 `BK_RES("img/bg2.png")` |
| `src/ui/utils/GameMenuView.cpp` | `_refreshStatePanel()` 将 `ASYNC_RELEASE` 移入 `brls::sync` 回调内部，消除竞态条件 |
| `report/mgba_libretro_netplay_feasibility.md` | 新增联机功能可行性分析报告 |

---

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

---

## 2026-04-06 修复第二次运行游戏的全程爆音和撕裂（#182 未修复）

### 任务分析

**问题现象**：程序启动后第一次运行游戏声音正常，第二次运行游戏从一开始就出现爆音，且整个运行过程中持续有爆音和撕裂声。

**输入**：`src/game/audio/AudioManager.cpp`（Switch 后端 `deinit()`）

**输出**：修复第二次游戏运行时全程爆音和撕裂问题

### 根本原因分析

**核心问题：`audoutStopAudioOut()` 破坏第二次游戏的缓冲区计数**

Switch 平台上，`BKAudioPlayer` 和 `AudioManager` 共享同一个 `audout` 服务会话（libnx 内部引用计数，同一 IPC 流）。两者的 `audoutWaitPlayFinish()` 调用竞争同一个完成事件队列。

**问题触发流程**：

1. 第一次游戏结束：`AudioManager::deinit()` 调用 `audoutStopAudioOut()` 停止了流
2. 第一次结束后（间隙期）：`AudioManager::m_running = false`，`BKAudioPlayer::isRunning()` 返回 false → BKAudioPlayer 有机会向已停止的流提交音效缓冲区
3. 第二次游戏 `init()` 调用 `audoutStartAudioOut()` **成功**（因为流已停止）→ 重启流
4. 重启的流中包含：BKAudioPlayer 残留的音效缓冲区 + AudioManager 新提交的 4 个静音缓冲区
5. 音频线程的非阻塞回收 `audoutWaitPlayFinish(0)` 得到 BKAudioPlayer 的缓冲区完成事件
6. `sw->enqueuedBuffers` 错误减少 1（认为是自己的缓冲区被回收）
7. `enqueuedBuffers(3) < SWITCH_N_BUFFERS(4)` → 跳过等待，直接向 `curBuf=0` 写入音频数据
8. 但 `outBuf[0]` 此时**仍在硬件 DMA 队列中**！音频线程向 DMA 进行中的缓冲区写入 → **全程爆音/撕裂**
9. `curBuf` 自此一直与硬件实际状态错位，问题持续整个游戏运行周期

**第一次运行正常的原因**：
- 第一次 `init()` 时 `audoutStartAudioOut()` **失败**（BKAudioPlayer 已启动流）→ 不存在"停止后重启"
- 没有 BKAudioPlayer 残留缓冲区混入 AudioManager 的计数

### 修复方案

**移除 `deinit()` 中的 `audoutStopAudioOut()` 调用**：

- `audout` 流的生命周期由 `BKAudioPlayer` 负责（它在程序启动时调用 `audoutStartAudioOut()`，退出时调用 `audoutStopAudioOut()`）
- `AudioManager` 只是借用共享流提交缓冲区，不应控制流的启停
- 移除后，两次游戏看到完全相同的 `audout` 状态（流持续运行，`audoutStartAudioOut()` 失败→继续）

**同时改进 drain 循环**：

- 使用独立计数器 `ourEnqueued`（而非 `sw->enqueuedBuffers`）
- 遍历返回缓冲区链表，通过指针比对验证是否属于本 `AudioManager` 实例
- 外来缓冲区（BKAudioPlayer 的）丢弃完成事件，继续等待自身缓冲区

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/game/audio/AudioManager.cpp` | Switch `deinit()`：移除 `audoutStopAudioOut()`，改进 drain 循环以正确过滤外来缓冲区 |
