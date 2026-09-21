# noSync「锁定本游戏配置」实现规范

> 用途：主程序 GBAStation 与外置核心共享的 per-game 配置锁。
> 新增核心时按本文档实现即可与主程序互通。

## 1. 一句话说明

每个游戏在游戏库 JSON 里带一个整型字段 `noSync`；置 1 后，同平台其他游戏执行
“同步画面 / 遮罩 / 着色器”时跳过该游戏，从而保留它的独立配置。

## 2. 字段定义

| 项 | 值 |
|---|---|
| 字段名 | `noSync` |
| 位置 | 平台游戏库每个游戏对象的顶层字段，如 `sdmc:/GBAStation/data/GameData_NDS.json`、`GameData_PSP.json` |
| 类型 | **整数** `0` / `1`（`0`=未锁定，`1`=锁定）；**禁止 bool** |
| 缺省 | 旧数据无此字段按 `0` 处理 |
| 持久化 | 存 GameDB，跨重启有效 |

**类型约束原因**：主程序用 `j.value("noSync", 0)` 严格按 int 反序列化，
写成 `true` / `false` 会抛类型错误。

## 3. 行为规则

- 锁只影响“**别人同步给我**”；当前游戏自身总被同步跳过（按 `path` 匹配），与锁无关。
- 覆盖核心内**所有**批量同步项（画面、遮罩、着色器等），全部套用同一跳过逻辑。
- 同步只改同平台条目（同一 GameData 文件 / `platform` 一致）。
- 开关状态改动立即写库；主程序开关与核心内开关读写同一字段，天然互通。

## 4. 主程序侧（已实现，新核心无需改动）

| 文件 | 内容 |
|---|---|
| `src/core/enums.h:133` | `GameEntry::noSync` |
| `src/core/game_database.cpp:292,337` | JSON 序列化 / 反序列化 |
| `src/ui/view/GameMenuView.cpp:2250` | 「锁定本游戏配置」开关 |
| `src/ui/view/GameMenuView.cpp:2496,2520,2545` | 三个同步函数 `if (game.noSync != 0) continue;` |
| `src/network/ApiRouter.cpp:580` | 远程编辑白名单含 `noSync` |

## 5. 新核心实现清单

### ① 读取当前游戏的锁

启动 / 菜单初始化时，按 `path` 匹配当前 ROM 读取；读取要容错（int 或 bool 都能读）：

```cpp
bool jsonNoSync(const nlohmann::json& item) {
    if (!item.contains("noSync")) return false;
    const auto& v = item.at("noSync");
    if (v.is_boolean()) return v.get<bool>();      // 容错旧/异常数据
    if (v.is_number()) return v.get<int>() != 0;
    return false;
}
```

### ② 菜单开关 UI

在“画面设置 → 同步设置”区加一行「锁定本游戏配置」开关，显示当前锁状态。

### ③ 写回 GameDB

切换即写，沿用核心已有的落盘 / 备份逻辑：

```cpp
item["noSync"] = noSync ? 1 : 0;   // 必须整数
```

DB 路径按核心已有约定尝试 `sdmc:/GBAStation/data/...` 与 `/GBAStation/data/...`。

### ④ 同步函数跳过锁定条目

在“跳过空 path / 跳过当前游戏”之后加：

```cpp
if (jsonNoSync(item)) continue;
```

### ⑤ 文案与本地化

- 开关名：「锁定本游戏配置」
- 提示：「开启后同平台同步操作将跳过本游戏」
- 同步确认弹窗：「……已锁定的游戏会被跳过」
- 按核心机制补中 / 英 / 日词条；无本地化则用中文。

## 6. 参考实现

### DraSticDS（C + 手写 JSON 补丁器）

- `source/gamedb.c/.h`：`gamedb_get_no_sync()` / `gamedb_set_no_sync()`；
  `patch_database()` 的 `all_nds` 匹配加 `read_int_field(object,"noSync",0)==0`
- `source/drastic_config.h/.c`：`DrasticRuntimeConfig.no_sync`，启动读取
- `source/ingame_menu.c`：画面页开关行，A / 左右切换落库

### melonDS nds_stub（C++ + nlohmann）

- `nds_stub/include/nds_stub/NdsMenuLayer.hpp`：`NdsDisplaySettings::noSync`、
  `NdsMenuAction::NoSyncChanged`
- `nds_stub/src/NdsMenuLayer.cpp`：`kDisplayRowNoSync` 行 + A 键切换
- `nds_stub/src/NdsDekoRuntime.cpp`：`loadNdsNoSyncFromGameDb()` /
  `saveNdsNoSyncToGameDb()`、action 落库、三个 sync 跳过
- `nds_stub/src/ui/UiComponents.cpp` + `resources/lang/*.json`：绘制与词条

## 7. 验收清单

1. 核心内开锁 → 重进仍为开（已落库）。
2. A 同步 → 锁定的 B 不变，未锁定的 C 被覆盖。
3. B 解锁后同步 → B 被覆盖。
4. 主程序给 B 开锁 → 核心内同步同样跳过 B。
5. 无 `noSync` 的旧条目默认不锁。
6. 同步后 JSON 合法且 `noSync` 为数字 `0/1`。

## 8. 给 AI 的提示词模板

> 这是 GBAStation 的外置核心项目。请给核心内菜单加「锁定本游戏配置」功能：
>
> 1. 在 `GameData_<平台>.json` 的游戏条目中读写整型字段 `noSync`（0/1，必须写整数，读取兼容 bool）；
> 2. 在菜单“画面设置 / 同步设置”区加开关，切换后立即落库；
> 3. 核心内所有“同步设置到同平台其他游戏”的函数，遍历时跳过 `noSync != 0` 的条目；
> 4. 补文案「锁定本游戏配置」「开启后同平台同步操作将跳过本游戏」，
>    同步确认弹窗注明“已锁定的游戏会被跳过”；
> 5. 参照现有 sync 函数风格实现，并给出验收步骤。
>
> 详细规范、参考实现和验收清单见 `report/noSync字段规范.md`。

## 9. 常见坑

- 写 bool 导致主程序反序列化报错 → 一律写 `1/0`。
- 只改主程序同步、漏改核心内同步 → 两个入口都要跳过。
- 忘记平台过滤 → 误改其他机种条目。
- 只读一次不落库 → 重启后锁丢失。
- 同步函数里漏掉遮罩 / 着色器分支 → 逐项检查。
