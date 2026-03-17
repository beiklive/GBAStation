# Session 96 任务分析

## 任务目标

在文件列表的 X 键菜单中，为游戏文件添加"添加到游戏库"选项。选择后弹出确认对话框，确认后将游戏添加到游戏库，并更新 `gamepath` 字段。

## 涉及文件

- `src/UI/Pages/FileListPage.cpp` — 主要修改，向 `openSidebar()` 添加新选项
- `resources/i18n/zh-Hans/beiklive.json` — 添加中文字符串
- `resources/i18n/en-US/beiklive.json` — 添加英文字符串

## 代码结构分析

### openSidebar 函数
位于 `src/UI/Pages/FileListPage.cpp:972`，负责构建 X 键菜单。  
对于非目录文件且平台被识别的游戏文件，已有"选择 Logo"选项，新选项应添加在此条件块中。

### 游戏库操作
- `initGameData(fileName, platform)` — 初始化 gamedataManager 条目
- `setGameDataStr(fileName, GAMEDATA_FIELD_GAMEPATH, fullPath)` — 设置 gamepath 字段
- `pushRecentGame(fileName)` — 加入最近游戏队列
- `g_recentGamesDirty = true` — 标记 AppPage 需要刷新

### 确认对话框
参考 `doDelete()` 使用 `brls::Dropdown` 实现二次确认。

## 实现方案

1. 在两个 i18n JSON 中添加字段：
   - `"add_to_library"`: "添加到游戏库" / "Add to Game Library"
   - `"add_to_library_confirm"`: "确认添加到游戏库？" / "Add to Game Library?"

2. 在 `openSidebar()` 的"游戏文件专属选项"块中，新增：
   - 条件：`!item.isDir && detectPlatform(item.fileName) != EmuPlatform::None`
   - 选项标签：`"beiklive/sidebar/add_to_library"_i18n`
   - 动作：弹出确认 Dropdown，确认后执行添加操作

## 挑战与解决方案

- 确保操作在 Dropdown 关闭后执行（参考已有的 dismissCb 模式）
- 避免重复添加（pushRecentGame 已有去重逻辑）
- gamepath 设置需要先 initGameData 初始化条目
