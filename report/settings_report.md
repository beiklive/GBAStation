# BeikLiveStation 设置项报告

## 概述

所有可配置项通过 `ConfigManager` (文件路径由 `gameRunner->settingConfig` 决定) 进行管理。
启动游戏时，`GameView::initialize()` 会将所有 mgba core 变量的默认值写入配置文件（仅在该键不存在时写入，不覆盖用户已设置的值）。

Core 变量名在配置文件中统一以 `core.` 为前缀，例如：
```
core.mgba_frameskip = 0
core.mgba_audio_low_pass_filter = disabled
```

---

## mGBA Libretro Core 可配置项

以下设置来源于 `third_party/mgba/src/platform/libretro/libretro_core_options.h`。

### 系统类 (system)

| 配置键 | 说明 | 可选值 | 默认值 |
|--------|------|--------|--------|
| `core.mgba_gb_model` | Game Boy 机型（需重启） | `Autodetect` / `Game Boy` / `Super Game Boy` / `Game Boy Color` / `Game Boy Advance` | `Autodetect` |
| `core.mgba_use_bios` | 使用 BIOS 文件（需重启） | `ON` / `OFF` | `ON` |
| `core.mgba_skip_bios` | 跳过 BIOS 开机动画（需重启） | `OFF` / `ON` | `OFF` |

### 视频类 (video)

| 配置键 | 说明 | 可选值 | 默认值 |
|--------|------|--------|--------|
| `core.mgba_gb_colors` | Game Boy 默认调色板 | `Grayscale`（以及运行时填充的调色板列表） | `Grayscale` |
| `core.mgba_gb_colors_preset` | 硬件预设调色板（需重启） | `0`（默认 GB 预设）/ `1`（GBC 预设）/ `2`（SGB 预设）/ `3`（任意可用预设） | `0` |
| `core.mgba_sgb_borders` | 显示 SGB 边框（需重启） | `ON` / `OFF` | `ON` |

### 音频类 (audio)

| 配置键 | 说明 | 可选值 | 默认值 |
|--------|------|--------|--------|
| `core.mgba_audio_low_pass_filter` | 低通音频滤波器 | `disabled` / `enabled` | `disabled` |
| `core.mgba_audio_low_pass_range` | 低通滤波器强度 | `5` \~ `95`（步进 5，单位 %） | `60` |

### 输入类 (input)

| 配置键 | 说明 | 可选值 | 默认值 |
|--------|------|--------|--------|
| `core.mgba_allow_opposing_directions` | 允许同时按下相反方向键 | `no` / `yes` | `no` |
| `core.mgba_solar_sensor_level` | 太阳能传感器等级 | `sensor` / `0` \~ `10` | `0` |
| `core.mgba_force_gbp` | Game Boy Player 震动（需重启） | `OFF` / `ON` | `OFF` |

### 性能类 (performance)

| 配置键 | 说明 | 可选值 | 默认值 |
|--------|------|--------|--------|
| `core.mgba_idle_optimization` | 空闲循环优化 | `Remove Known` / `Detect and Remove` / `Don't Remove` | `Remove Known` |
| `core.mgba_frameskip` | 跳帧数 | `0` \~ `10` | `0` |

---

## 显示配置项 (DisplayConfig)

由 `DisplayConfig::load()` 从配置文件读取，键名前缀为 `display.`。

| 配置键 | 说明 | 可选值 | 默认值 |
|--------|------|--------|--------|
| `display.screenMode` | 缩放模式 | `fit`（等比适配）/ `stretch`（拉伸填充）/ `original`（原始尺寸）/ `integer`（整数倍缩放） | `fit` |
| `display.filterMode` | 纹理过滤 | `nearest`（近邻，像素风）/ `linear`（线性，平滑） | `nearest` |

---

## 快速操作配置

| 按键 | 功能 |
|------|------|
| ZR（BUTTON_RT） | 按住加速（快进，4× 帧率，音频静音） |
| X（BUTTON_X）  | 退出游戏，返回主菜单 |
| A（BUTTON_A）  | 游戏内 A 键；加载失败时关闭错误界面 |

---

## 实现说明

1. **SettingManager 与 LibretroLoader 的集成**  
   `GameView::initialize()` 将 `settingConfig` 传递给 `LibretroLoader::setConfigManager()`，
   之后 mGBA 通过 `RETRO_ENVIRONMENT_GET_VARIABLE` 查询时，将从 `ConfigManager` 中读取用户保存的值。
   若某变量在配置文件中不存在，`ConfigManager::Get()` 返回空，LibretroLoader 返回 `false`，
   mGBA core 则使用自身的硬编码默认值（与本表一致）。

2. **变量首次写入**  
   `GameView::initialize()` 调用 `ConfigManager::SetDefault()` 为每个已知 core 变量
   写入默认值（仅在不存在时写入），随后调用 `ConfigManager::Save()` 持久化到文件。
   用户只需编辑配置文件即可覆盖默认值。

3. **mGBA 的 SET_VARIABLES 处理**  
   `LibretroLoader::s_environmentCallback` 处理 `RETRO_ENVIRONMENT_SET_VARIABLES` 命令，
   解析 `"描述; 默认值|选项2|..."` 格式，并调用 `SetDefault()` 补充 `GameView` 未预注册的变量。
