# Session 94 任务分析：gamedata 增加 playcount 字段

## 任务目标

在 `gamedata` 中增加 `playcount` 字段，每次启动游戏时对应游戏的计数加一。

## 输入/输出分析

**输入**：
- 玩家在 StartPageView 中选择并启动任意游戏

**输出**：
- `BKStation/config/gamedata.cfg` 中，对应游戏的 `playcount` 字段每次启动时自增 1
- 示例：`pokemon.playcount=i|5`

## 代码结构分析

### GameData 存储机制

- **位置**：`./BKStation/config/gamedata.cfg`
- **格式**：INI 键值对，整数用 `i|` 前缀（例：`pokemon.totaltime=i|3600`）
- **管理器**：全局 `gamedataManager`（`beiklive::ConfigManager*`）
- **字段常量**：定义在 `include/common.hpp`（第 173-180 行）
- **初始化**：`initGameData()` 在 `include/common.hpp`（第 192-236 行）

### 游戏启动流程

```
用户选择游戏
  ↓
recordGameOpenTime(fileName)  ← 记录 lastopen 时间戳
  ↓
launchGameActivity(romPath)  ← 推送 GameView 活动
  ↓
GameView::initialize()  ← 加载核心和 ROM
```

`recordGameOpenTime` 在两处被调用：
1. `StartPageView.cpp:179` — 从文件列表打开游戏
2. `StartPageView.cpp:356` — 从最近游戏列表打开游戏

### 现有字段常量

| 常量 | 值 | 类型 |
|------|-----|------|
| `GAMEDATA_FIELD_LOGOPATH` | "logopath" | String |
| `GAMEDATA_FIELD_GAMEPATH` | "gamepath" | String |
| `GAMEDATA_FIELD_TOTALTIME` | "totaltime" | Int |
| `GAMEDATA_FIELD_LASTOPEN` | "lastopen" | String |
| `GAMEDATA_FIELD_PLATFORM` | "platform" | String |
| `GAMEDATA_FIELD_OVERLAY` | "overlay" | String |
| `GAMEDATA_FIELD_CHEATPATH` | "cheatpath" | String |

### 现有辅助函数

- `getGameDataStr()` — 读取字符串字段
- `setGameDataStr()` — 写入字符串字段并保存

目前**缺少整数读写辅助函数**，需要添加 `getGameDataInt()` 和 `setGameDataInt()`。

## 实现方案

### 修改 1：`include/common.hpp`

1. 添加 `GAMEDATA_FIELD_PLAYCOUNT "playcount"` 常量
2. 在 `initGameData()` 中添加 `playcount` 默认值 0
3. 添加 `getGameDataInt()` 辅助函数
4. 添加 `setGameDataInt()` 辅助函数

### 修改 2：`src/UI/StartPageView.cpp`

在 `recordGameOpenTime()` 函数中，记录 lastopen 的同时，读取当前 playcount 并加一保存。

## 可能的挑战

1. **线程安全**：`recordGameOpenTime` 在主线程调用，`gamedataManager` 也在主线程操作，无并发问题。
2. **首次启动**：`initGameData()` 已确保 playcount 默认为 0，因此首次启动后变为 1。
3. **保存次数**：`setGameDataInt()` 和 `setGameDataStr()` 都会调用 `Save()`，游戏启动时会保存两次。这是可接受的（非性能关键路径）。

## 涉及文件

- `include/common.hpp`（主要修改）
- `src/UI/StartPageView.cpp`（playcount 增量逻辑）
- `report/session_94_analysis.md`（本文档）
- `report/session_94.md`（工作汇报）
