# NDS独立NRO方案评估

## 背景

当前“在主程序内软切换 Deko3D 接管画面”的探针已经完成：

- `dk::Device`、`dk::Queue`、present-capable framebuffer、`nwindowGetDefault()` 都能创建或调用。
- 崩溃点锁定在同一个默认窗口上第二次创建 Deko swapchain：

```cpp
swapchain = swapchainMaker.create();
```

结论：主程序 Borealis/OpenGL/GLFW 已经持有默认窗口显示链路时，不能再在同一进程中软创建第二套 Deko swapchain。

因此改为“独立 NRO”是合理下一步：NDS 运行时作为单独程序启动，从进程初始化阶段就独占 Deko3D 显示链路，避开主程序已有 OpenGL/Borealis swapchain。

## 方案概述

主程序 `GBAStation.nro` 保留现有 UI、游戏库、数据库、配置、下载、非 NDS 核心。

NDS 游戏启动时：

1. 主程序识别 `EmuNDS`。
2. 写入一份启动上下文文件，或直接组装 argv。
3. 调用 libnx homebrew chainload 接口：

```cpp
envSetNextLoad("sdmc:/switch/GBAStationNDS.nro", "\"sdmc:/path/game.nds\"");
brls::Application::quit();
```

4. hbloader 在主程序退出后启动 `GBAStationNDS.nro`。
5. `GBAStationNDS.nro` 读取 argv 或启动上下文，直接加载 NDS ROM。
6. NDS NRO 退出时，可选择再 chainload 回主程序：

```cpp
envSetNextLoad("sdmc:/switch/GBAStation.nro", "");
```

## 本地证据

### libnx支持链式启动

本机 devkitPro/libnx 头文件存在：

```cpp
Result envSetNextLoad(const char* path, const char* argv);
bool envHasNextLoad(void);
```

语义是配置下一个要加载的 homebrew NRO 路径和参数。

### 本项目已有类似痕迹

`src/ui/page/UpdatePage.cpp` 中存在被注释的重启思路：

```cpp
// envSetNextLoad("sdmc:/switch/GBAStation.nro", "sdmc:/switch/GBAStation.nro");
```

说明工程层面已经具备或考虑过“退出后拉起另一个 NRO”的方向。

### RetroArch Switch也使用该机制

`third_party/RetroArch-1.22.2/frontend/drivers/platform_switch.c` 中使用：

```cpp
envSetNextLoad(path, args);
```

这证明该机制不是仅存在于头文件，而是成熟 homebrew 项目中的实际用法。

### ArcDelta_melonDS已支持argv加载ROM

`third_party/ArcDelta_melonDS/src/frontend/switch/main.cpp` 中已有：

```cpp
if (!argvLoaded && argc == 2)
{
    Emulation::LoadROM(argv[1]);
    argvLoaded = true;
}
```

这对独立 NRO 方案非常有利：第一版可以直接把 ROM 路径作为 argv 传入，不需要先改复杂 IPC。

## 可行性结论

总体可行，且比“硬切换销毁/重建 Borealis/OpenGL”更推荐。

原因：

- 独立 NRO 避开同进程双 swapchain 冲突。
- NDS NRO 可从启动阶段独占 Deko3D，与 ArcDelta_melonDS 架构一致。
- 主程序和 NDS 运行时进程级隔离，不会污染 libretro/mGBA/OpenGL 音频渲染路径。
- NDS 性能优化可以直接沿用 ArcDelta_melonDS 的 Switch/Deko 前端，而不是在现有 GameView 上继续缝补。

## 推荐形态

### 第一版推荐：封装ArcDelta_melonDS为GBAStationNDS.nro

先不要把 ArcDelta 大量代码拆进主工程。

推荐先新增一个独立构建产物：

```text
build_switch/GBAStation.nro
build_switch/GBAStationNDS.nro
```

`GBAStationNDS.nro` 初期可以高度接近 ArcDelta_melonDS：

- 使用 ArcDelta 的 `Gfx::Init()`、Deko device/queue/swapchain。
- 使用 ArcDelta 的 GPU2D_Deko/GPU3D_Deko 和 shader romfs。
- 使用 ArcDelta 的输入、音频、暂停菜单作为第一阶段运行壳。
- 增加 GBAStation 启动参数解析、路径映射、退出返回主程序。

这样最短路径验证性能收益，不先做大规模架构融合。

### 第二版再做品牌和体验融合

稳定后再逐步接入：

- GBAStation 的配置路径。
- GBAStation 的 BIOS 路径。
- GBAStation 的存档路径。
- GBAStation 的按键映射。
- GBAStation 风格的暂停菜单。
- 退出后返回主程序并刷新最近游戏。

## 参数传递设计

### argv直接传ROM路径

最小可行：

```text
argv[0] = sdmc:/switch/GBAStationNDS.nro
argv[1] = sdmc:/GBAStation/roms/nds/game.nds
```

优点：

- 简单。
- ArcDelta 已有 `argc == 2` 自动加载。
- 首阶段改动少。

风险：

- 路径带空格、中文、引号时需要严格转义。
- argv 长度受 hbloader 环境限制，超长路径不宜全靠 argv。

### 推荐增强：启动上下文文件

主程序写：

```text
sdmc:/GBAStation/runtime/nds_launch.json
```

内容：

```json
{
  "romPath": "sdmc:/GBAStation/roms/nds/game.nds",
  "returnNro": "sdmc:/switch/GBAStation.nro",
  "saveDir": "sdmc:/GBAStation/saves/nds",
  "biosDir": "sdmc:/GBAStation/bios/nds",
  "configPath": "sdmc:/GBAStation/config/config.cfg",
  "gameId": "...",
  "title": "..."
}
```

然后 argv 只传：

```text
--launch sdmc:/GBAStation/runtime/nds_launch.json
```

优点：

- 不怕路径过长和转义。
- 可逐步加入更多配置。
- 方便记录启动失败原因。

建议第一阶段先支持两种方式：

- argv[1] 是 `.nds` 路径：直接加载。
- argv 包含 `--launch xxx.json`：读取上下文。

## 返回主程序设计

NDS NRO 退出时有三种模式：

1. **直接退出到 hbmenu**
   - 最简单。
   - 体验割裂。

2. **chainload返回主程序**
   - `envSetNextLoad("sdmc:/switch/GBAStation.nro", "")`
   - 用户体验最好。
   - 需要验证主程序二次启动稳定性。

3. **返回主程序并传回结果**
   - NDS NRO 退出前写：

```text
sdmc:/GBAStation/runtime/nds_result.json
```

   - 主程序启动后读取：

```json
{
  "romPath": "...",
  "playTimeSeconds": 1234,
  "saveChanged": true,
  "exitReason": "user_exit"
}
```

推荐第一阶段实现 2，第二阶段实现 3。

## 工程接入方式

### CMake

主工程现有 Switch 打包逻辑已经使用：

```cmake
NX_NACPTOOL_EXE
NX_ELF2NRO_EXE
```

ArcDelta_melonDS 自带 Switch CMake：

```cmake
add_executable(melonDS.elf ...)
add_custom_target(melonDS.nro ALL
    ${ELF2NRO} ... --romfsdir=${CMAKE_BINARY_DIR}/romfs
)
```

可选路径：

1. 先保留 ArcDelta 独立构建，在 `switchbuild.sh` 中增加第二段构建和复制。
2. 后续把 ArcDelta 的 Switch target 纳入主 CMake，生成 `GBAStationNDS.nro`。

推荐先走第 1 种，风险最低。

### 文件布局建议

```text
src/platform/switch/NroLauncher.hpp
src/platform/switch/NroLauncher.cpp

third_party/ArcDelta_melonDS/...

report/NDS独立NRO方案评估.md
report/NDS独立NRO阶段执行工作日志.md
```

运行时 SD 卡布局：

```text
sdmc:/switch/GBAStation.nro
sdmc:/switch/GBAStationNDS.nro
sdmc:/GBAStation/config/config.cfg
sdmc:/GBAStation/runtime/nds_launch.json
sdmc:/GBAStation/runtime/nds_result.json
sdmc:/GBAStation/bios/nds/
sdmc:/GBAStation/saves/nds/
```

## 与当前主程序的关系

主程序保留：

- 游戏列表。
- 收藏、最近游戏、封面。
- 设置。
- 非 NDS 游戏运行。
- libretro / mGBA / 当前 OpenGL 路径。

主程序 NDS 启动逻辑改为：

- 默认仍可走当前 melonDS OpenGL 路径，用于回退。
- 当 `nds.externalNro.enabled = 1` 时走独立 NRO。
- 如果 `envHasNextLoad()` 不支持或 NDS NRO 不存在，回退当前路径或提示错误。

## 优势

- 最大限度隔离，不破坏 libretro/mGBA。
- 直接绕过已确认的同进程 swapchain 冲突。
- 能利用 ArcDelta_melonDS 已验证的 Switch Deko 架构。
- 更接近“原生 NDS 应用”性能表现。
- 后续可独立调优 CPU/GPU/内存、overclock、shader、菜单，不受主 UI 渲染节奏影响。

## 风险

### 风险1：返回主程序体验

`envSetNextLoad()` 是“退出当前 NRO 后加载下一个 NRO”，不是进程内跳转。主程序状态会丢失，需要靠结果文件恢复。

应对：

- 主程序启动时读取 `nds_result.json`。
- 恢复到游戏列表或最近游戏页面。
- 刷新游玩时间、最近记录、存档状态。

### 风险2：路径和配置不一致

ArcDelta 默认配置路径可能是：

```text
sdmc:/switch/melonDS/...
```

本项目路径是：

```text
sdmc:/GBAStation/...
```

应对：

- 第一阶段只传 ROM。
- 第二阶段补 BIOS/save/config 路径适配。
- 不直接让 ArcDelta 使用自己的默认存档位置作为最终方案。

### 风险3：双项目构建复杂

ArcDelta 自带 shader 编译和 romfs 打包，纳入主工程可能引入 CMake 复杂度。

应对：

- 先独立构建并复制产物。
- 稳定后再整合 CMake target。

### 风险4：菜单和按键体验割裂

第一版 NDS NRO 可能使用 ArcDelta 菜单，而不是 GBAStation 菜单。

应对：

- 第一阶段接受体验割裂，优先验证性能。
- 第二阶段统一按键映射。
- 第三阶段再做 GBAStation 风格菜单。

### 风险5：NDS NRO崩溃后无法回主程序

如果 NDS NRO 崩溃，无法写结果文件，也无法 chainload 回主程序。

应对：

- 主程序不依赖返回结果完成关键数据。
- NDS NRO 增加独立日志。
- 保留当前 OpenGL NDS 路径作为回退。

## 分阶段执行方案

### 阶段A：启动机制验证

目标：主程序能启动一个最小 NDS Stub NRO，并带参数。

内容：

- 新增 `NroLauncher`。
- 新增配置：

```text
nds.externalNro.enabled = 0
nds.externalNro.path = sdmc:/switch/GBAStationNDS.nro
```

- NDS 游戏选择后，调用 `envSetNextLoad()`。
- Stub NRO 打印 argv 到日志，然后返回主程序。

验收：

- 主程序能退出并启动 Stub NRO。
- Stub NRO 能读到 ROM 路径。
- Stub NRO 能 chainload 回主程序。

### 阶段B：ArcDelta原样NRO接入

目标：主程序能拉起 ArcDelta_melonDS NRO 并自动加载 ROM。

内容：

- 构建 ArcDelta 的 `melonDS.nro`，改名或复制为 `GBAStationNDS.nro`。
- 使用 argv[1] 传 ROM 路径。
- 暂时使用 ArcDelta 默认菜单、设置和存档位置。

验收：

- 宝可梦黑2等大型游戏能进入。
- x1 性能与 ArcDelta 原版接近。
- 退出后可回主程序或 hbmenu。

### 阶段C：路径和配置适配

目标：NDS NRO 使用 GBAStation 的 BIOS/save/config 路径。

内容：

- 加入 `nds_launch.json`。
- NDS NRO 解析：
  - ROM 路径。
  - BIOS 路径。
  - 存档路径。
  - 返回 NRO。
  - 显示设置。
  - 按键映射。

验收：

- 存档与主程序数据库路径一致。
- BIOS 从 `sdmc:/GBAStation/bios/nds` 读取。
- 退出后主程序可更新最近游戏和游玩时长。

### 阶段D：体验整合

目标：NDS NRO 像 GBAStation 的一个专属运行模式，而不是外部程序。

内容：

- 统一暂停键。
- 统一快进键。
- 统一存档/读档入口。
- 增加返回主程序按钮。
- 根据需要替换 ArcDelta 菜单皮肤。

验收：

- 普通用户感知为“进入了 NDS 游戏全屏模式”。
- 退出后回到主程序列表。
- 错误提示和日志可读。

## 推荐决策

建议采用独立 NRO 方案，并从阶段A开始。

不建议继续投入“同进程软切换 Deko swapchain”。

也不建议立刻做“硬切换销毁 Borealis/OpenGL 再重建”，因为风险高、回归面大，而且仍然不如独立 NRO 干净。

最小闭环优先级：

1. 主程序 chainload Stub NRO。
2. Stub NRO 接收 argv 并返回主程序。
3. 主程序 chainload ArcDelta NRO 并传 ROM。
4. 验证宝可梦黑2 x1 性能。
5. 再做路径和体验整合。

## 结论

独立 NRO 是当前最合理的 NDS Deko 路线。

它牺牲的是“同进程无缝切换”，换来的是：

- Deko swapchain 独占初始化。
- 与主程序 OpenGL/Borealis 完全隔离。
- 更接近 ArcDelta_melonDS 的性能结构。
- 对 libretro/mGBA 零侵入。

从当前 probe 结果看，独立 NRO 不是绕路，而是避开已确认架构冲突后的主线方案。
