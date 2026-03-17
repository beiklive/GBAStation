# Session 97 任务分析：游戏库功能

## 任务目标

完善 APPPage 里的游戏库功能，在点击游戏库按钮后，打开新的 Activity，包含标题栏、主体和底栏。

## 需求分析

### 数据来源
- 数据来自 `gamedataManager`（使用 `collectGameEntries` 模式遍历所有 gamepath 条目）
- 每条游戏的字段：`logopath`、`gamepath`、`totaltime`、`lastopen`、`playcount`
- 显示名称优先从 `NameMappingManager` 获取，否则显示文件名（不含后缀）

### UI 结构
- **标题栏**：`BrowserHeader`，`setInfo` 显示游戏数量
- **主体**：`brls::ScrollingFrame`，5列网格布局（手动按行分组）
- **每个元素**：纵向 Box + `ProImage`（异步加载 logopath）+ `brls::Label`（单行滚动，焦点时显示，失焦时隐藏）
- **底栏**：`brls::BottomBar`

### 交互
- **A 键**：启动游戏，同时更新 `recent.game` 列表、`lastopen`、`playcount`
- **X 键**：弹出菜单（与 AppPage GameCard 相同：设置映射名、选择封面、从库中移除）
- **Y 键**：弹出 Dropdown 选择排序方式（按游玩顺序/按游玩时间/按游戏名称）

### 排序方式
- **按游玩顺序**（lastopen 字段，字典序 DESC，"从未游玩" 排最后）
- **按游玩时间**（totaltime 字段，数值 DESC）
- **按游戏名称**（displayName 字段，字母序 ASC）

### 重构
- 提取通用的"打开游戏"逻辑（`recordGameOpenTime + pushRecentGame + launchGameActivity`）为独立函数
- 在 StartPageView.cpp 内部重构（不改变外部 API）

## 实现计划

1. **`include/UI/Pages/GameLibraryPage.hpp`**
   - `GameLibraryEntry` 结构体
   - `GameLibraryItem` 类（网格单元素）
   - `GameLibraryPage` 类（主页面）

2. **`src/UI/Pages/GameLibraryPage.cpp`**
   - 从 gamedataManager 收集数据
   - 构建 5 列网格
   - 排序逻辑
   - X/Y 键菜单逻辑

3. **`include/UI/Pages/AppPage.hpp`**
   - 添加 `onOpenGameLibrary` 回调

4. **`src/UI/Pages/AppPage.cpp`**
   - 将"游戏库"按钮连接到 `onOpenGameLibrary`

5. **`include/UI/StartPageView.hpp`**
   - 添加 `openGameLibraryPage()` 方法声明

6. **`src/UI/StartPageView.cpp`**
   - 实现 `openGameLibraryPage()`
   - 重构 `openGameFromLibrary()` 辅助函数
   - 设置 `m_appPage->onOpenGameLibrary`

7. **i18n 文件更新**
   - 添加游戏库相关字符串

## 挑战与解决方案

- **网格布局**：borealis 无原生 Grid 组件，采用手动行分组（每行 5 个 Box，外层 COLUMN Box）
- **焦点管理**：排序后重建网格，焦点转移到第一个元素
- **异步图片加载**：使用 `ProImage::setImageFromFileAsync`
- **X 键菜单中的文件浏览**：复用 `FileListPage` 模式选择封面图片
