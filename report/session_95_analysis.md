# Session 95 分析文档：新增 PlaylistManager

## 任务目标

新增一个 PlaylistManager，用于统计打开过哪些游戏：
- 默认 `listcount` 值为 0
- 每次启动 playlist 中没有的新游戏时，`listcount` 加一
- 同时增加 `game<序号>` 键，值为游戏文件名称

## 输入

- `include/common.hpp`：全局声明、辅助函数
- `main.cpp`：全局变量定义和初始化
- `src/UI/StartPageView.cpp`：游戏启动时的统计调用

## 输出

- 新配置文件：`GBAStation/config/playlist.cfg`，格式与现有 ConfigManager 文件一致
- 新增全局变量 `PlaylistManager`
- 新增内联函数 `pushPlaylistGame()`

## 设计决策

1. **PlaylistManager 使用独立配置文件**：与 `gamedataManager`、`SettingManager` 并列，存储于 `playlist.cfg`，职责单一。

2. **pushPlaylistGame 函数放在 common.hpp**：与 `pushRecentGame` 等辅助函数保持一致，便于任何源文件调用。

3. **调用位置在 recordGameOpenTime()**：`StartPageView.cpp` 中所有游戏启动均经过此函数，无需修改多处。

4. **使用顺序追加而非哈希/集合**：配置文件格式决定必须顺序追加（`game0`、`game1`、...），与问题要求的 `game+序号` 格式完全吻合。

## 修改文件

| 文件 | 修改内容 |
|------|----------|
| `include/common.hpp` | 添加 `PlaylistManager` 外部声明；添加 `pushPlaylistGame()` 内联函数 |
| `main.cpp` | 添加 `PlaylistManager` 全局变量定义；在 `ConfigManagerInit()` 中初始化 |
| `src/UI/StartPageView.cpp` | 在 `recordGameOpenTime()` 中调用 `pushPlaylistGame()` |

## 可能的挑战

- **线程安全**：`recordGameOpenTime` 在主（UI）线程调用，PlaylistManager 仅在此线程访问，无并发问题。
- **大列表性能**：遍历所有已记录游戏以检查重复。对于正常用户（数百款游戏），性能可接受。
