# NDS 主程序 GameView/GameMenuView 功能梳理

日期：2026-07-03

## 目的

本报告整理主程序 `GameView`、`GameMenuView`、以及二者通过 `GamePage` 注入的所有 NDS 相关功能，作为后续在 `GBAStationNDSStub.nro` 内重做游戏层和模拟器菜单层的迁移清单。

当前方向是：主程序继续负责游戏列表、GameDB 和启动 stub；NDS 运行、Deko 渲染、音频、触摸和模拟器菜单由独立 stub 接管。stub 不应依赖主程序的 GameView/GameMenuView/Borealis UI 实现，但需要复刻对应行为和 GameDB 字段。

## 涉及文件

- `src/ui/view/GameView.cpp`
- `src/ui/view/GameView.hpp`
- `src/ui/view/GameMenuView.cpp`
- `src/ui/view/GameMenuView.hpp`
- `src/ui/page/GamePage.cpp`
- `src/core/game_database.cpp`

## GameDB 字段

stub 需要读取和保存下列 NDS 相关字段：

| 字段 | 含义 | 当前默认值/限制 |
| --- | --- | --- |
| `ndsScreenLayout` | NDS 双屏布局 | 默认 `vertical`，旧值 `separate` 兼容为 `custom` |
| `ndsScreenOrientation` | 屏幕方向 | `0` / `90` / `180` / `270`，空值默认 `0` |
| `ndsIntegerScale` | NDS 最大整数倍缩放 | 默认 `false` |
| `ndsTopScale` | 自定义布局上屏缩放 | 默认 `1.0`，运行时限制 `0.25` 到 `4.0` |
| `ndsTopOffsetX` | 自定义布局上屏 X 偏移 | 默认 `0.0` |
| `ndsTopOffsetY` | 自定义布局上屏 Y 偏移 | 默认 `0.0` |
| `ndsBottomScale` | 自定义布局下屏缩放 | 默认 `1.0`，运行时限制 `0.25` 到 `4.0` |
| `ndsBottomOffsetX` | 自定义布局下屏 X 偏移 | 默认 `0.0` |
| `ndsBottomOffsetY` | 自定义布局下屏 Y 偏移 | 默认 `0.0` |
| `ndsInternalResolution` | NDS 3D 内部分辨率倍率 | 主程序 Switch 版强制 `1` |
| `cheatPath` | 金手指文件 | NDS 默认优先 `usrcheat.dat` |
| `savePath` | SRAM/即时存档目录 | 默认由 `defaultGameSavePath(platform,path)` 生成 |
| `displayMode` / `integerAspectRatio` / `customScale` / `customOffsetX/Y` | 普通画面模式 | NDS 部分布局仍会复用 |
| `overlayPath` / `overlayEnabled` | 遮罩 | GameMenuView 有通用菜单 |
| `shaderPath` / `shaderEnabled` / `shaderPara*` | shader | Switch NDS 当前禁用 |

## GameView 中的 NDS 功能

### 1. 初始化策略

- `isNdsPlatform()` 判断平台是否为 `EmuNDS`。
- `shouldSetupCoreOnGameThread()` 对 NDS 返回 true，因此主程序会把 NDS core 初始化推迟到游戏线程，避免 UI 线程长时间阻塞。
- NDS 强制关闭倒带：
  - `m_rewindEnabled = false`
  - `m_rewindShowUI = false`
  - `GameSignal::requestRewind(false)`
- 初始化布局：
  - 空值默认 `vertical`
  - `separate` 兼容为 `custom`
  - 合法值：`vertical`、`horizontal`、`priority_top`、`custom`、`hybrid`、`top`、`bottom`
- 初始化方向：
  - `normalizeNdsScreenRotation()` 支持 `0/90/180/270`、`deg`、`°`、`vertical/horizontal` 等别名。
- Switch 版强制：
  - `ndsInternalResolution = 1`

stub 迁移建议：

- 保留相同布局字段和方向兼容逻辑。
- 不迁移主程序的 libretro 初始化链。
- NDS stub 内可以直接使用 ArcDelta/melonDS 初始化，不需要 `shouldSetupCoreOnGameThread()` 这套兼容逻辑。

### 2. 渲染模式和布局

主程序使用 OpenGL `GameRenderer` 渲染 NDS 帧，有三类路径：

1. 普通 CPU 帧上传：
   - 游戏线程取 `m_core->GetVideoFrame()`。
   - UI 线程 `_uploadPendingFrame()` 上传到 GL 纹理。
   - 非 `priority_top` 布局会先走 `_layoutNdsFrame()` 做 CPU 重排。

2. 分屏 shader 路径：
   - `_useNdsSplitShader()` 非 Switch 且 shader 可用时启用。
   - 拆出上下屏分别上传到 `m_ndsTopRenderer` / `m_ndsBottomRenderer`。
   - Switch 版返回 false。

3. 加速纹理路径：
   - `_useNdsAcceleratedTexture()` 非 Switch 且 core 暴露 `IEmulatorVideoTexture` 时启用。
   - Switch 版返回 false。

支持的布局：

| 布局 ID | UI 名称 | 主程序行为 |
| --- | --- | --- |
| `vertical` | 上下屏 | 256x384，默认上下排列 |
| `horizontal` | 左右屏 | 512x192，左右排列 |
| `priority_top` | 上屏优先 | 1024x768 逻辑画布，上屏大、下屏小 |
| `custom` | 自定义 | 1280x720 逻辑画布，上下屏分别有偏移/缩放 |
| `hybrid` | 混合 | 1280x720 逻辑画布，大上屏 + 小上屏 + 小下屏 |
| `top` | 仅上屏 | 256x192 |
| `bottom` | 仅下屏 | 256x192 |

额外规则：

- `custom`、`hybrid`、`priority_top` 强制使用 `Fit`。
- 开启 `ndsIntegerScale` 时，普通布局使用最大整数倍缩放。
- `priority_top` 在绘制时直接使用整个 GameView 区域。
- `hybrid` 布局强制关闭纹理过滤。
- 方向为 `90/270` 时，普通布局会交换逻辑宽高。

stub 迁移建议：

- 第一优先级：在 `NdsGameLayer` 内实现同样的布局矩形计算，但直接用 Deko 绘制上下屏纹理/Framebuffer 区域，避免 CPU 重排整张 1280x720 图。
- `custom` 和 `hybrid` 不应照搬 `_layoutNdsFrame()` 的 CPU blit，应该用多次 Deko quad 绘制完成。
- `ndsInternalResolution` 在 Switch stub 中继续锁定 x1，除非后续真正接入 GPU 3D renderer 并证明性能稳定。

### 3. 屏幕旋转和 UV

主程序有完整方向映射：

- `_ndsOrientationUv()` 返回旋转后的 UV。
- `_rotateNdsScreenRect()` 根据方向旋转单屏绘制矩形。
- `_unrotateNdsRect()` 计算未旋转布局矩形。
- `_mapNdsPointToUnrotated()` 辅助把触点从旋转后画面映射回未旋转布局。

stub 迁移建议：

- 把方向处理抽成独立纯函数，例如 `NdsLayout::computeRects()` 和 `NdsLayout::mapTouch()`。
- 渲染时旋转 UV；触摸时用相反映射。
- 先实现 `0` 度，随后补 `90/180/270`，每个方向都要单独验证下屏触摸坐标。

### 4. 触摸输入

主程序有两套触摸入口：

- Borealis gesture：
  - `TapGestureRecognizer`
  - `PanGestureRecognizer`
- 每帧原始输入轮询：
  - `updateTouchStates()`
  - `updateMouseStates()`

触摸映射逻辑：

- 只对 NDS 下屏提交触摸。
- 通过 `m_ndsTouchRect` 保存当前 NDS 绘制区域。
- 遍历 `_computeNdsScreenDrawRects()` 找到底屏矩形。
- 按方向修正 `u/v`。
- 最终提交 `SetTouch(ndsX, ndsY, true)`，坐标范围：
  - X：0 到 255
  - Y：0 到 191
- 释放时提交 `SetTouch(0, 0, false)`。

stub 迁移建议：

- 使用 libnx hid touch 直接读取，不依赖 Borealis。
- 触摸映射需要跟布局/旋转/交换屏幕共用同一套矩形计算。
- 菜单打开时必须屏蔽游戏触摸，关闭菜单时释放一次触摸状态。

### 5. NDS 虚拟指针

主程序额外支持手柄模拟触摸：

- 热键 `hotkey.pointer_mode.pad`：切换虚拟指针模式。
- 热键 `hotkey.pointer_click.pad`：按住触摸点击。
- 右摇杆控制虚拟指针移动。
- 虚拟指针开启时，右摇杆不再映射为游戏方向键。
- 坐标内部使用 NDS 下屏坐标：
  - `m_ndsVirtualPointerX`：0 到 255
  - `m_ndsVirtualPointerY`：0 到 191
- `_drawNdsVirtualPointer()` 在下屏绘制十字准星。

stub 迁移建议：

- 这是手柄模式下很有用的 NDS 功能，建议在触摸稳定后迁移。
- Stub 目前菜单键已改 ZR，右摇杆可保留给虚拟指针。

### 6. 交换上下屏

主程序支持热键：

- `hotkey.swap_screens.pad`

行为：

- 切换 `m_ndsScreensSwapped`。
- 清空 `m_ndsTouchRect`。
- 释放虚拟指针触摸。
- 请求上传上一帧，避免暂停/切换时画面空白。

stub 迁移建议：

- 迁移为菜单项或热键均可。
- 注意触摸始终跟“逻辑下屏”走，而不是跟视觉位置固定走。

### 7. NDS 音频和快进策略

主程序对 NDS 音频有特殊保护：

- 音频目标延迟下限：
  - `kNdsTargetLatencyFloorMs = 120`
  - `kNdsMaxLatencyFloorMs = 240`
- 音频同步强度下限更保守：
  - `kNdsMaxAudioSyncStrength = 0.008`
  - 修正范围 `0.99` 到 `1.01`
- NDS 推音频使用 `pushSamplesNoBlocking()`，避免阻塞游戏线程。

快进特殊逻辑：

- `_stepFrame(ff)` 中 NDS 快进时即使 `m_ffMultiplier >= 1` 也只运行 1 帧。
- `_pushFrameAudio(ff)` 中 NDS 快进不会按 `fastforward.mute` 静音。
- `AudioManager::setSpeed()` 对 NDS 不按倍率变速。

stub 迁移建议：

- 不建议照搬主程序这个快进策略。它是为 libretro/AudioManager 兜底，历史上容易表现为“跳帧而非真实加速”。
- Stub 已经使用 ArcDelta 风格 `audren/audrv` 后，应基于 melonDS/SPU 节奏设计独立快进：
  - 快进运行多帧；
  - 音频可选择丢弃、限量输出或独立重采样；
  - 不要把 32823Hz PCM 当 48kHz 直接输出。

### 8. 存档、读档、截图、SRAM

主程序菜单通过 `GameSignal` 把请求交给游戏线程：

- 保存状态：`requestQuickSave(slot)`
- 读取状态：`requestQuickLoad(slot)`
- 删除状态：删除 `.ss*` 和缩略图 `.png`
- 自动读档：`save.autoLoadState0`
- 自动存档：`save.autoSaveState` + `save.autoSaveInterval`
- 退出自动存档：`save.autoSaveOnExit`
- 截图：`requestScreenshot()`
- SRAM 自动落盘：
  - 周期 CRC 检测。
  - dirty 后延迟写盘。
  - 退出时强制 `saveSram()`。

即时存档路径：

- `getStatePath(slot)` 使用 `GameEntry.savePath`，为空时回退全局 saves。
- 命名依赖 `beiklive::tools::getStatePath(dir, gamePath, slot)`。

stub 迁移建议：

- 必须优先实现：
  - SRAM 正常读写。
  - 菜单保存状态/读取状态。
  - 退出前 flush SRAM。
- 缩略图和截图可第二阶段补。
- 自动存档/自动读档如果要保持主程序体验，需要读取同样 setting key。

### 9. 金手指

GameMenuView 的金手指面板是通用 UI，但 NDS 有特殊路径和格式：

- NDS 文件选择支持：
  - `.cht`
  - `.dat`
- NDS 默认 `cheatPath` 优先为 `usrcheat.dat`。
- `usrcheat.dat` 被视为只读数据库：
  - 不允许新增。
  - 不允许修改代码。
  - 不允许删除条目。
- `.cht` 可编辑并保存。
- 支持分类条目：
  - `CheatEntry.code.empty()` 时显示为不可切换分类。
- 支持互斥组：
  - 打开某一项后，同组其他项自动关闭。
- UI 中切换后：
  - 更新内存里的 `m_cheats`。
  - 调 `m_cheatToggleCallback`。
  - 保存可编辑 `.cht`。
- GamePage 将 callback 转成：
  - `GameView::requestCheatPathUpdate(path)`
  - `GameView::applyCheatsUpdate(cheats)`
- GameView 游戏线程消费后调用：
  - `m_core->SetCheatPath(path)`
  - `m_core->ApplyCheats(cheats)`

stub 迁移建议：

- 不依赖主程序 `CheatSystem` 的情况下，可以先复制/抽取金手指解析逻辑到 stub 独立目录。
- 第一阶段菜单可先实现：
  - 显示当前 cheatPath。
  - 读取 `usrcheat.dat`。
  - 列表开关。
  - 应用到 melonDS。
- 编辑 `.cht`、新增、删除可后续补。

## GameMenuView 中的 NDS 菜单功能

### 1. TabFrame 结构

主菜单 Tab 顺序：

1. 返回游戏
2. 保存状态
3. 读取状态
4. 金手指设置
5. 画面设置
6. NES 平台才有手柄页
7. 分割线
8. 重置游戏
9. 退出游戏

NDS stub 需要还原的目标顺序：

1. 返回游戏
2. 保存状态
3. 读取状态
4. 金手指
5. 画面设置
6. 重置游戏
7. 退出游戏

当前用户要求的按键逻辑：

- `ZR` 打开/关闭模拟器菜单。
- 菜单内保持主程序相近的左右切 tab、A 确认、B 返回。

### 2. 保存状态/读取状态面板

主程序使用 10 个槽位：

- slot 0 通常是自动存档。
- slot 1 到 9 是手动槽位。
- 每个格子显示：
  - 槽位名称。
  - 是否存在。
  - 缩略图路径。
  - 文件修改时间。
- 删除存档会同步删除缩略图，并刷新保存/读取两个面板。

stub 迁移建议：

- 保持 10 槽位。
- 缩略图可以先用占位图，后续实现截图生成。

### 3. NDS 画面设置

NDS 专属设置项：

- `NDS屏幕方向`
  - `0度`
  - `90度`
  - `180度`
  - `270度`
- `NDS屏幕布局`
  - `上下屏` -> `vertical`
  - `左右屏` -> `horizontal`
  - `上屏优先` -> `priority_top`
  - `自定义` -> `custom`
  - `混合` -> `hybrid`
  - `仅上屏` -> `top`
  - `仅下屏` -> `bottom`
- `NDS画面整数缩放`
- `上屏调整`
  - 仅 `custom` 布局可用。
- `下屏调整`
  - 仅 `custom` 布局可用。
- `NDS内部分辨率`
  - Switch 版只显示 `x1 原生`。

自定义屏幕调整侧栏：

- `X轴偏移`
- `Y轴偏移`
- `缩放比例`
- `复原`
- `保存`

保存字段：

- `ndsTopOffsetX/Y/Scale`
- `ndsBottomOffsetX/Y/Scale`
- `ndsScreenLayout`
- `ndsScreenOrientation`

stub 迁移建议：

- 优先实现布局、方向、整数缩放。
- 第二步实现自定义偏移/缩放侧栏。
- 内部分辨率菜单在 Switch stub 中继续只保留 x1，避免引导用户使用不稳定功能。

### 4. 同步设置到其他游戏

主程序画面设置页底部有同步功能：

- 同步画面设置到同平台所有游戏。
- 对 NDS 额外同步：
  - `ndsTopScale`
  - `ndsTopOffsetX`
  - `ndsTopOffsetY`
  - `ndsBottomScale`
  - `ndsBottomOffsetX`
  - `ndsBottomOffsetY`
  - `ndsScreenLayout`
  - `ndsScreenOrientation`
  - `ndsInternalResolution`

注意：当前 `_syncDisplaySettings()` 没有同步 `ndsIntegerScale`。

stub 迁移建议：

- 如果在 stub 菜单内提供“同步到全部 NDS 游戏”，应考虑补上 `ndsIntegerScale`，否则用户体验会和单游戏设置不一致。
- 同步功能可延后，因为它需要完整 GameDB 写回能力。

## GamePage 中的注入关系

GamePage 是主程序里 GameView 和 GameMenuView 的连接层。

NDS 相关 callback：

- `setNdsLayoutCallback()` -> `GameView::_onNdsLayoutChange()`
- `setNdsScreenOrientationCallback()` -> `GameView::_onNdsScreenOrientationChange()`
- `setNdsScreenAdjustCallback()` -> `GameView::_onNdsScreenValuesChanged()`
- `setNdsIntegerScaleCallback()` -> `GameView::_onNdsIntegerScaleChange()`
- `setNdsInternalResolutionCallback()` -> `GameView::_onNdsInternalResolutionChange()`

通用但 NDS 也要复刻的 callback：

- `setSaveStateCallback()`
- `setLoadStateCallback()`
- `setDeleteStateCallback()`
- `setStateInfoCallback()`
- `setCheatToggleCallback()`
- `setCheatPathCallback()`
- `setCheatsChangedCallback()`
- `setDisplayModeCallback()`
- `setOverlayToggleCallback()`
- `setOverlayPathCallback()`
- `setShaderToggleCallback()`
- `setShaderPathCallback()`

stub 迁移建议：

- Stub 内不需要照搬 callback 类名，但需要保留相同事件流：
  - 菜单选择更新运行时状态。
  - 运行时状态写回 GameDB。
  - 涉及核心配置时通知 melonDS 重新应用。

## Stub 迁移优先级

### P0：必须优先实现

- ZR 打开/关闭菜单，返回游戏、重置、退出。
- 基础 TabFrame 效果和菜单焦点逻辑。
- SRAM 正常读写，退出前 flush。
- 保存状态/读取状态 10 槽位。
- 触摸输入映射到下屏。
- FPS/RUN 指标显示。
- GameDB 读取当前游戏 NDS 字段。
- GameDB 写回布局、方向、整数缩放、自定义偏移/缩放。

### P1：影响体验，建议紧接着做

- 完整七种屏幕布局。
- 0/90/180/270 方向旋转。
- 屏幕交换热键或菜单项。
- 自定义布局侧栏。
- 金手指 `usrcheat.dat` 读取、列表、开关和应用。
- 菜单中文字体和图标继续完善。

### P2：可延后

- 金手指 `.cht` 编辑、新增、删除。
- 存档缩略图。
- 截图。
- 自动存档/自动读档。
- 遮罩。
- 同步设置到全部 NDS 游戏。
- 虚拟指针模式。

### 不建议照搬

- 主程序 OpenGL `GameRenderer` 上传/重排路径。
- `_layoutNdsFrame()` 的 CPU 1280x720 blit。
- NDS 快进“只跑 1 帧”的保守策略。
- Switch NDS shader/internal resolution x2-x4 相关 UI。

## 与当前 stub 的差距

当前 stub 已具备：

- 独立 NRO 启动。
- 读取 ROM path。
- 根据 path 查 GameDB。
- Deko-only 游戏运行。
- SRAM 读取。
- 基础菜单层。
- ZR 菜单键。
- audren/audrv 音频路径。
- 中文系统字体加载。
- FPS/RUN 显示。

仍需补齐：

- TabFrame 风格菜单还原。
- 保存/读取状态 UI 和实际状态文件。
- 金手指 UI 和 melonDS 应用链。
- 完整 NDS 布局、旋转、整数缩放。
- 触摸在所有布局下的坐标映射。
- 自定义上下屏调整。
- GameDB 写回与同步策略。

## 建议下一阶段拆分

1. 先在 `nds_stub` 内新增独立的 `NdsLayout` 模块，纯计算布局矩形、UV 和触摸映射。
2. 改造 `NdsGameLayer`，从固定绘制改为按 `NdsLayout` 绘制多个 screen quad。
3. 改造 `NdsMenuLayer`，做出 TabFrame 风格框架和基础 tab 切换。
4. 接入保存/读取状态 10 槽位。
5. 接入金手指读取和开关。
6. 最后补自定义布局侧栏、虚拟指针和同步到全部游戏。
