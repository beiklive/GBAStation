# 工作汇报 - 数据管理页面与BKAudioPlayer Switch平台优化

## 任务分析

### 任务目标

1. **BKAudioPlayer Switch平台优化**：将 Switch 平台的 UI 音效播放从使用 libpulsar（qlaunch BFSAR 官方音效）改为读取 WAV 文件，与 PC 平台保持一致。

2. **数据管理页面**：实现完整的数据管理功能，包括：
   - 按平台分类浏览游戏（GBA/GBC/GB）
   - 查看各游戏的存档状态
   - 支持删除存档文件和截图

### 输入/输出

- **输入**：GameDatabase 中已录入的游戏数据（GameEntry）、游戏存档目录中的 `.ssN` 文件
- **输出**：
  - DataManagementPage：平台+游戏双列表 UI
  - GameDetailPage：存档管理 UI（显示槽位信息，支持X键删除）

### 挑战与解决方案

| 挑战 | 解决方案 |
|------|----------|
| Switch 平台无 ALSA/WinMM，需要使用 libnx audout 播放 WAV PCM | 使用最近邻重采样将 WAV（44100Hz）转换至 audout 固定输出率（48000Hz），支持 pitch 参数影响重采样速率 |
| BKAudioPlayer 与 AudioManager 共用 audout 时可能冲突 | BKAudioPlayer 调用 `audoutStartAudioOut()` 忽略已启动错误；AudioManager 也不因此失败 |
| 存档文件扫描与 UI 更新的异步协调 | 使用 `ASYNC_RETAIN/ASYNC_RELEASE` + `brls::async/sync` 宏确保异步扫描+主线程更新 |

---

## 实现内容

### 1. BKAudioPlayer Switch平台WAV播放

**修改文件：**
- `src/ui/audio/BKAudioPlayer.hpp`：移除 `#include <pulsar.h>` 和 libpulsar 成员变量；保留 `m_switchInit`
- `src/ui/audio/BKAudioPlayer.cpp`：
  - 移除所有 libpulsar/BFSAR 代码
  - `_initSwitch()`：调用 `audoutInitialize()` + `audoutStartAudioOut()`，失败则记录日志
  - `load()`：所有平台统一使用 WAV 文件加载
  - `playSoundDirect()`（Switch）：最近邻重采样 + aligned_alloc + audoutAppendAudioOutBuffer 播放
  - pitch 参数通过调整重采样比率支持变速播放
- `src/game/audio/AudioManager.cpp`：`audoutStartAudioOut` 失败时仅记录警告，不返回 false
- `src/main.cpp`：移除 `#ifndef __SWITCH__` 保护，所有平台统一注册 BKAudioPlayer

### 2. 数据管理页面

**新增文件：**
- `src/ui/page/DataManagementPage.hpp/.cpp`：
  - 继承 `beiklive::Box`，使用页头/页脚
  - 左右布局：左侧 25% 为平台按钮（GBA/GBC/GB），右侧为 GridBox（单列，GAME_LIBRARY 模式）
  - 聚焦平台按钮时异步加载对应平台游戏列表
  - 点击游戏：pushActivity(GameDetailPage)

- `src/ui/page/GameDetailPage.hpp/.cpp`：
  - 标题显示游戏名称
  - 左右布局：左侧 25% 为功能按钮（存档/金手指/成就），右侧为内容面板
  - 存档面板：GridBox（双列，SAVE_STATE 模式），10个槽位（自动存档+槽位1-9）
  - 存档槽位支持 X 键弹出确认对话框删除存档和截图
  - 金手指/成就面板：占位（待实现）

**修改文件：**
- `src/core/game_database.hpp/.cpp`：添加 `getByPlatform(EmuPlatform)` 方法
- `src/ui/page/StartPage.hpp/.cpp`：添加 `_openDataManagement()` 方法，接入 `onDataManagementOpened` 回调

---

## 关键设计决策

1. **audout 共享模式**：BKAudioPlayer 和 AudioManager 均调用 `audoutInitialize()`（libnx 引用计数，安全），两者都能提交缓冲区到同一 audout 队列。菜单 UI 音效（约100ms以内）与游戏音频分时复用队列，实际使用中无明显冲突。

2. **存档路径复用**：`GameDetailPage` 使用与 `GameView::getStatePath()` 相同的命名规则（`{saveDir}/{stem}.ssN`），确保存档文件定位一致。

3. **异步数据加载**：所有数据库查询和文件系统操作均在 `brls::async` 中执行，UI 更新在 `brls::sync` 中执行，避免阻塞渲染线程。

---

## 构建验证

- **平台**：Desktop（Linux，ALSA音频后端）
- **结果**：构建成功，无 error（第三方库 warning 已存在，与本次修改无关）
