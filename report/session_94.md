# Session 94 工作汇报：gamedata 增加 playcount 字段

## 任务描述

在 `gamedata` 中增加 `playcount` 字段，每次启动游戏时对应游戏的计数加一。

## 完成情况

✅ 所有任务已完成

## 变更文件

### 1. `include/common.hpp`

**新增常量**：
```cpp
#define GAMEDATA_FIELD_PLAYCOUNT  "playcount"  ///< 游戏启动次数（每次启动加一，默认 0）
```

**`initGameData()` 中添加默认值**：
```cpp
// 启动次数：默认 0
setIfAbsent(GAMEDATA_FIELD_PLAYCOUNT, beiklive::ConfigValue(0));
```

**新增整数读写辅助函数**：
```cpp
/// 读取 gamedataManager 中某个游戏字段的整数值。
inline int getGameDataInt(const std::string& fileName, const std::string& field, int def = 0);

/// 设置 gamedataManager 中某个游戏整数字段并保存。
inline void setGameDataInt(const std::string& fileName, const std::string& field, int val);
```

### 2. `src/UI/StartPageView.cpp`

**`recordGameOpenTime()` 中增加 playcount 自增逻辑**：
```cpp
// 启动次数加一
int count = getGameDataInt(fileName, GAMEDATA_FIELD_PLAYCOUNT, 0);
setGameDataInt(fileName, GAMEDATA_FIELD_PLAYCOUNT, count + 1);
```

## 效果

每次游戏启动（从文件列表或最近游戏列表）时，`recordGameOpenTime()` 会：
1. 记录本次启动时间到 `lastopen` 字段（原有功能）
2. 将 `playcount` 字段值加一（新功能）

### gamedata.cfg 示例

```ini
pokemon.cheatpath=s|
pokemon.gamepath=s|/path/to/pokemon.gba
pokemon.lastopen=s|2026-03-16 23:55
pokemon.logopath=s|
pokemon.overlay=s|
pokemon.platform=s|GBA
pokemon.playcount=i|5
pokemon.totaltime=i|3600
```

## 构建验证

编译成功，无新增错误或警告。

## 调用路径

```
用户选择游戏（文件列表 / 最近游戏列表）
  ↓
recordGameOpenTime(fileName)
  ├─ setGameDataStr(fileName, GAMEDATA_FIELD_LASTOPEN, timeBuf)  // 更新时间戳
  └─ setGameDataInt(fileName, GAMEDATA_FIELD_PLAYCOUNT, count+1) // playcount 加一
```
