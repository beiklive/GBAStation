# Session 95 工作汇报：新增 PlaylistManager

## 任务概述

实现 PlaylistManager，用于统计应用生命周期内启动过的所有游戏文件名。

## 完成内容

### 修改文件

**1. `include/common.hpp`**
- 新增 `extern beiklive::ConfigManager* PlaylistManager` 全局指针声明
- 新增 `pushPlaylistGame(const std::string& gameFileName)` 内联辅助函数，风格与 `pushRecentGame` 一致

**2. `main.cpp`**
- 新增 `beiklive::ConfigManager* PlaylistManager = nullptr` 全局变量定义
- 在 `ConfigManagerInit()` 中初始化，对应配置文件 `GBAStation/config/playlist.cfg`
- 设置 `listcount` 默认值为 0，无需立即保存（首次记录游戏时才触发保存）

**3. `src/UI/StartPageView.cpp`**
- 在 `recordGameOpenTime()` 末尾调用 `pushPlaylistGame(fileName)`，确保游戏启动时更新列表

### 配置文件格式（playlist.cfg）

```
listcount=i|3
game0=s|pokemon.gba
game1=s|zelda.gba
game2=s|mario.gb
```

## 设计说明

- PlaylistManager 使用独立配置文件，与其他管理器并列，职责单一
- `pushPlaylistGame` 先读取 `listcount`，遍历已有条目检查重复；若不存在则追加 `game<count>` 并递增计数
- 调用位置 `recordGameOpenTime()` 覆盖所有游戏启动入口（文件列表页和 APP 页）

## 安全分析

无安全漏洞。所有操作均为本地文件读写，使用已有的 ConfigManager 机制，类型安全。
