# GBAStation 全系统可配置项清单

> 从 `src/` 目录全部源码分析整理 (排除 json.hpp)  
> 共 **100 项** SettingManager 配置键

---

## 访问宏 (src/core/common.h)

| 宏 | 说明 |
|----|------|
| `GET_SETTING_KEY_STR(key, def)` | 读取字符串 |
| `SET_SETTING_KEY_STR(key, val)` | 写入字符串 |
| `GET_SETTING_KEY_INT(key, def)` | 读取整数 |
| `SET_SETTING_KEY_INT(key, val)` | 写入整数 |
| `GET_SETTING_KEY_FLOAT(key, def)` | 读取浮点 |
| `SET_SETTING_KEY_FLOAT(key, val)` | 写入浮点 |
| `CHECK_KEY(key)` | 检查键是否存在 |
| `GET_MAPPING_KEY_STR(key, def)` | 读取名称映射 (NameMappingManager) |
| `SET_MAPPING_KEY_STR(key, val)` | 写入名称映射 |

---

## 一、UI 界面设置 (10项)

| # | 键 | 类型 | 默认值 | 说明 |
|---|-----|------|--------|------|
| 1 | `UI.startPage` | String | - | 起始页面 |
| 2 | `UI.language` | String | - | 语言 |
| 3 | `UI.theme` | String | - | 主题 |
| 4 | `UI.showBgImage` | Bool | false | 显示背景图片 |
| 5 | `UI.bgImagePath` | String | "" | 背景图片路径 |
| 6 | `UI.bgBlurEnabled` | Bool | false | 背景模糊 |
| 7 | `UI.bgBlurRadius` | Float | 12.0 | 模糊半径 |
| 8 | `UI.showXmbBg` | Bool | false | XMB 风格背景 |
| 9 | `UI.pspxmb.color` | String | "blue" | XMB 颜色预设 |
| 10 | `UI.useSavestateThumbnail` | Bool | false | 无封面时用存档截图 |

## 二、画面遮罩 (4项)

| # | 键 | 类型 | 默认值 | 说明 |
|---|-----|------|--------|------|
| 11 | `display.overlay.enabled` | Bool | false | 遮罩总开关 |
| 12 | `display.overlay.gbaPath` | String | "" | GBA 遮罩图片 |
| 13 | `display.overlay.gbcPath` | String | "" | GBC 遮罩图片 |
| 14 | `display.overlay.gbPath` | String | "" | GB 遮罩图片 |

## 三、着色器 (5项)

| # | 键 | 类型 | 默认值 | 说明 |
|---|-----|------|--------|------|
| 15 | `display.shaderEnabled` | Bool | false | 着色器总开关 |
| 16 | `display.shader` | String | "" | 全局着色器预设 (.glslp) |
| 17 | `display.shader.gba` | String | "" | GBA 着色器 |
| 18 | `display.shader.gbc` | String | "" | GBC 着色器 |
| 19 | `display.shader.gb` | String | "" | GB 着色器 |

## 四、画面显示 (7项)

| # | 键 | 类型 | 默认值 | 说明 |
|---|-----|------|--------|------|
| 20 | `display.mode` | String | "original" | 画面模式 |
| 21 | `display.integer_scale_mult` | Int | 0 | 整数缩放倍率 |
| 22 | `display.filter` | String | "nearest" | 纹理过滤 (nearest/linear) |
| 23 | `display.showFps` | Bool | false | 显示 FPS |
| 24 | `display.showFfOverlay` | Bool | true | 快进覆盖层 |
| 25 | `display.showRewindOverlay` | Bool | true | 倒带覆盖层 |
| 26 | `display.showMuteOverlay` | Bool | true | 静音覆盖层 |

## 五、音频 (2项)

| # | 键 | 类型 | 默认值 | 说明 |
|---|-----|------|--------|------|
| 27 | `audio.buttonSfx` | Bool | true | 按钮音效 |
| 28 | `core.mgba_audio_low_pass_filter` | String | "disabled" | 低通滤波器 (disabled/enabled) |
| 29 | `core.mgba_audio_low_pass_range` | String | "60" | 截止频率 (%) |

## 六、模拟器核心 (12项)

| # | 键 | 类型 | 默认值 | 可选值 |
|---|-----|------|--------|--------|
| 30 | `core.mgba_gb_model` | String | "Autodetect" | Autodetect / Game Boy / Super Game Boy / Game Boy Color / Game Boy Advance |
| 31 | `core.mgba_use_bios` | String | "ON" | ON / OFF |
| 32 | `core.mgba_skip_bios` | String | "OFF" | ON / OFF |
| 33 | `core.mgba_gb_colors` | String | "Grayscale" | 12种配色方案 |
| 34 | `core.mgba_gb_colors_preset` | String | "0" | 数字字符串 |
| 35 | `core.mgba_sgb_borders` | String | "ON" | ON / OFF |
| 36 | `core.mgba_allow_opposing_directions` | String | "no" | yes / no |
| 37 | `core.mgba_solar_sensor_level` | String | "0" | 数字字符串 |
| 38 | `core.mgba_force_gbp` | String | "OFF" | ON / OFF |
| 39 | `core.mgba_idle_optimization` | String | "Remove Known" | Remove Known / Detect and Remove / Don't Remove |
| 40 | `core.mgba_frameskip` | String | "0" | 跳帧数 |
| 41 | `theme` | Int | 1 | 主题布局 (0=DEFAULT, 1=SWITCH) |

## 七、快进 (4项)

| # | 键 | 类型 | 默认值 | 说明 |
|---|-----|------|--------|------|
| 42 | `fastforward.enabled` | Bool | true | 快进总开关 |
| 43 | `fastforward.mode` | String | "hold" | 触发模式 |
| 44 | `fastforward.multiplier` | Float | 4.0 | 倍率 |
| 45 | `fastforward.mute` | Bool | true | 快进时静音 |

## 八、倒带 (6项)

| # | 键 | 类型 | 默认值 | 说明 |
|---|-----|------|--------|------|
| 46 | `rewind.enabled` | Bool | false | 倒带总开关 |
| 47 | `rewind.mode` | String | "hold" | 触发模式 |
| 48 | `rewind.step` | Int | 2 | 步进帧数 |
| 49 | `rewind.mute` | Bool | false | 倒带时静音 |
| 50 | `rewind.saveInterval` | Int | 1 | 保存间隔(帧) |
| 51 | `rewind.showUI` | Bool | false | 可视化倒带界面 |
| 52 | `rewind.thumbCompression` | Int | 0 | 缩略图压缩 (0=最近邻, 1=双线性) |
| 53 | `rewind.bufferSize` | Int | 600 | 缓冲区最大帧数 |
| 54 | `rewind.uiItemCount` | Int | 10 | (已废弃) |

## 九、存档 (4项)

| # | 键 | 类型 | 默认值 | 说明 |
|---|-----|------|--------|------|
| 55 | `save.autoSaveState` | Bool | false | 自动保存状态 |
| 56 | `save.autoSaveInterval` | Int | 0 | 自动保存间隔(秒) |
| 57 | `save.autoLoadState0` | Bool | false | 启动时自动加载槽位0 |
| 58 | `save.sramDir` | String | "" | SRAM 目录 (空=ROM目录) |
| 59 | `save.stateDir` | String | "" | 即时存档目录 |

## 十、金手指 (2项)

| # | 键 | 类型 | 默认值 | 说明 |
|---|-----|------|--------|------|
| 60 | `cheat.enabled` | Bool | false | 金手指总开关 |
| 61 | `cheat.dir` | String | "" | 金手指目录 |

## 十一、截图 (1项)

| # | 键 | 类型 | 默认值 | 说明 |
|---|-----|------|--------|------|
| 62 | `screenshot.dir` | Int | 0 | 截图目录 |

## 十二、游戏按键映射 — 手柄 (16项)

| # | 键 | 类型 | 默认值 | 对应功能 |
|---|-----|------|--------|----------|
| 63 | `handle.a` | String | "A" | A 键 |
| 64 | `handle.b` | String | "B" | B 键 |
| 65 | `handle.x` | String | "X" | X 键 |
| 66 | `handle.y` | String | "Y" | Y 键 |
| 67 | `handle.up` | String | "UP" | 上 |
| 68 | `handle.down` | String | "DOWN" | 下 |
| 69 | `handle.left` | String | "LEFT" | 左 |
| 70 | `handle.right` | String | "RIGHT" | 右 |
| 71 | `handle.l` | String | "LB" | L |
| 72 | `handle.r` | String | "RB" | R |
| 73 | `handle.l2` | String | "LT" | L2 |
| 74 | `handle.r2` | String | "RT" | R2 |
| 75 | `handle.l3` | String | "LSB" | L3(左摇杆按) |
| 76 | `handle.r3` | String | "RSB" | R3(右摇杆按) |
| 77 | `handle.start` | String | "START" | Start |
| 78 | `handle.select` | String | "BACK" | Select |

## 十三、摇杆方向映射 (8项)

| # | 键 | 类型 | 默认值 | 说明 |
|---|-----|------|--------|------|
| 79 | `handle.lstick_up` | String | "LEFTSTICKUP" | 左摇杆上 |
| 80 | `handle.lstick_down` | String | "LEFTSTICKDOWN" | 左摇杆下 |
| 81 | `handle.lstick_left` | String | "LEFTSTICKLEFT" | 左摇杆左 |
| 82 | `handle.lstick_right` | String | "LEFTSTICKRIGHT" | 左摇杆右 |
| 83 | `handle.rstick_up` | String | "RIGHTSTICKUP" | 右摇杆上 |
| 84 | `handle.rstick_down` | String | "RIGHTSTICKDOWN" | 右摇杆下 |
| 85 | `handle.rstick_left` | String | "RIGHTSTICKLEFT" | 右摇杆左 |
| 86 | `handle.rstick_right` | String | "RIGHTSTICKRIGHT" | 右摇杆右 |

## 十四、功能热键 (8项)

| # | 键 | 类型 | 默认值 | 功能 |
|---|-----|------|--------|------|
| 87 | `handle.fastforward` | String | "LSB" | 快进 |
| 88 | `handle.rewind` | String | "RSB" | 倒带 |
| 89 | `hotkey.menu.pad` | String | "LT+RT" | 打开菜单 |
| 90 | `hotkey.quicksave.pad` | String | "none" | 快速保存 |
| 91 | `hotkey.quickload.pad` | String | "none" | 快速读取 |
| 92 | `hotkey.mute.pad` | String | "none" | 静音 |
| 93 | `hotkey.pause.pad` | String | "none" | 暂停 |
| 94 | `hotkey.screenshot.pad` | String | "none" | 截图 |

## 十五、输入设置 (2项)

| # | 键 | 类型 | 默认值 | 说明 |
|---|-----|------|--------|------|
| 95 | `input.joystick.enabled` | Bool | true | 摇杆方向键启用 |
| 96 | `input.joystick.diagonal` | Bool | true | 斜向输入 |

## 十六、调试 (3项)

| # | 键 | 类型 | 默认值 | 说明 |
|---|-----|------|--------|------|
| 97 | `debug.logLevel` | String | "info" | 日志级别 (debug/info/warning/error) |
| 98 | `debug.logFile` | Bool | false | 日志输出到文件 |
| 99 | `debug.logOverlay` | Bool | false | 调试覆盖层 |

## 十七、数据库路径 (1项)

| # | 键 | 类型 | 默认值 | 说明 |
|---|-----|------|--------|------|
| 100 | `db_path` | String | 自动 | 数据库目录 |

---

## GB 配色方案 (core.mgba_gb_colors 可选值)

`Grayscale`, `Honey`, `Lime`, `Grapefruit`, `Game Boy`, `Burnt Orange`, `Mystic Blue`, `Motocross Pink`, `Gaiden Pink`, `Blues`, `Dark Knight`, `Solarized Gold`

---

## 配置文件路径

| 文件 | 路径 |
|------|------|
| 系统配置 | `{ROOT}/GBAStation/config/config.cfg` |
| 名称映射 | `{ROOT}/GBAStation/config/name_mapping.cfg` |
| 游戏数据库 | `{ROOT}/GBAStation/data/GameData.json` |
| 日志 | `{ROOT}/GBAStation/log/GBAStation.log` |