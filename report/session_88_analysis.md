# Session 88 任务分析：GameMenu 独立遮罩设置后不生效问题修复

## 任务目标

修复 GameMenu 中重新设置游戏独立遮罩后，返回游戏时遮罩未变更的问题。
并在设置完遮罩后立刻预加载遮罩纹理，使恢复游戏时不产生卡顿。

## 问题分析

### 问题现象

用户在 GameMenu 的画面设置中选择了新的游戏独立遮罩（PNG 文件），返回游戏后，
遮罩显示并未更新，仍然显示旧的遮罩或无遮罩。

### 根本原因

**数据流断裂**：GameMenu 保存遮罩路径到 `gamedataManager`，但 GameView 的运行时成员变量未同步更新。

| 流程步骤 | 代码位置 | 操作 | 状态 |
|---------|---------|------|------|
| 初始化时 | `game_view.cpp:218` | 从 `gamedataManager` 读取到 `m_overlayPath` | ✅ 正确 |
| 设置遮罩时 | `GameMenu.cpp:138` | `setGameDataStr()` 保存到 `gamedataManager` | ✅ 保存成功 |
| 设置遮罩时 | GameView 运行时 | `m_overlayPath` **未更新** | ❌ 断裂点 |
| 返回游戏时 | `game_view.cpp:2068` | 使用旧的 `m_overlayPath` 绘制 | ❌ 显示旧遮罩 |

**具体表现**：
- `GameMenu` 选择文件后调用 `setGameDataStr(m_romFileName, GAMEDATA_FIELD_OVERLAY, item.fullPath)` 将路径持久化
- 但 `GameView::m_overlayPath` 是在 `initialize()` 中一次性读取的，运行期间不会自动同步
- 因此渲染时 `m_overlayPath` 仍为旧值，新遮罩不显示

## 解决方案

### 方案设计

采用**回调机制**，在 `GameMenu` 与 `GameView` 之间建立实时通信：

1. `GameMenu` 添加 `m_overlayChangedCallback`，在用户选择新遮罩后触发
2. `GameView::setGameMenu()` 注册此回调，更新 `m_overlayPath` 并设置 `m_overlayReloadPending` 标志
3. `draw()` 检测到标志后立即调用 `loadOverlayImage()` 预加载纹理

### 预加载优势

遮罩图像在**游戏暂停期间**（菜单打开时）的下一帧 `draw()` 调用中即完成加载，
而非等到游戏恢复后的第一帧再加载，消除了恢复游戏时的卡顿。

## 修改文件清单

### `include/UI/Utils/GameMenu.hpp`
- 添加 `m_overlayChangedCallback` 成员（`std::function<void(const std::string&)>`）
- 添加 `setOverlayChangedCallback()` 公有方法

### `src/UI/Utils/GameMenu.cpp`
- 在文件选择确认回调中，`setGameDataStr()` 之后调用 `m_overlayChangedCallback`

### `include/Game/game_view.hpp`
- 添加 `m_overlayReloadPending`（`std::atomic<bool>`）成员

### `src/Game/game_view.cpp`
- `setGameMenu()` 中注册 `setOverlayChangedCallback` 回调：更新 `m_overlayPath`，设置 `m_overlayReloadPending`
- `draw()` 中在遮罩绘制块之前检测 `m_overlayReloadPending`，触发 `loadOverlayImage()`

## 潜在挑战与解决

| 挑战 | 解决方案 |
|-----|---------|
| `loadOverlayImage()` 需要 `NVGcontext*`，只在 `draw()` 中可用 | 用 `m_overlayReloadPending` 原子标志延迟到 `draw()` 执行 |
| 旧纹理需要正确释放避免 GL 资源泄漏 | `loadOverlayImage()` 内部先调用 `nvgDeleteImage()` 释放旧图像 |
| 线程安全：主线程写 `m_overlayPath`，`draw()` 在主线程读 | 两者均在主（UI）线程，无并发问题 |
| 不应强制覆盖全局遮罩开关状态 | 回调不修改 `m_overlayEnabled`，尊重用户的显示设置 |
