# NDS独立NRO阶段执行工作日志

## 2026-07-03 阶段A：Stub NRO链式启动闭环

### 目标

- 验证主程序能通过 libnx `envSetNextLoad()` 拉起独立 NDS NRO。
- 验证主程序能把 NDS ROM 路径作为 argv 传给独立 NRO。
- 验证独立 NRO 能通过 `envSetNextLoad()` 返回主程序。
- 不启动真实 melonDS，不接入 ArcDelta，不触碰 libretro/mGBA/现有 GameView 渲染音频路径。

### 已实施

- 新增 Switch-only NRO 启动封装：
  - `src/platform/switch/NroLauncher.hpp`
  - `src/platform/switch/NroLauncher.cpp`
- 新增最小 Stub NRO：
  - `src/platform/switch/nds_stub/NdsStubMain.cpp`
- Stub 行为：
  - 记录 `argc/argv`。
  - 识别 `argv[1]` 为 ROM 路径。
  - 识别 `--return <nro>` 为返回主程序路径。
  - 退出前尝试 `envSetNextLoad(returnNro, ...)` 返回主程序。
  - 写日志到：

```text
sdmc:/GBAStation/log/GBAStationNDSStub.log
/GBAStation/log/GBAStationNDSStub.log
sdmc:/GBAStationNDSStub.log
/GBAStationNDSStub.log
```

- `StartPage` 新增 NDS 外部 NRO 分流：
  - `nds.externalNro.enabled = 1` 时，NDS 游戏启动走独立 NRO。
  - 启动失败时回退原有路径。
  - 默认关闭，不影响当前 NDS/OpenGL 路径和 NDS Deko 实验页。
- 新增默认配置：

```text
nds.externalNro.enabled = 0
nds.externalNro.path = sdmc:/switch/GBAStationNDSStub.nro
nds.externalNro.returnPath = sdmc:/switch/GBAStation.nro
```

- CMake 新增独立产物：

```text
build_switch/GBAStationNDSStub.nro
```

- `switchbuild.sh` 新增 Stub NRO 打包和大小输出。

### 待验证

- Switch 编译通过。
- 将两个 NRO 放到 SD 卡：

```text
sdmc:/switch/GBAStation.nro
sdmc:/switch/GBAStationNDSStub.nro
```

- 配置：

```text
nds.externalNro.enabled=i|1
nds.externalNro.path=s|sdmc:/switch/GBAStationNDSStub.nro
nds.externalNro.returnPath=s|sdmc:/switch/GBAStation.nro
```

- 启动 NDS 游戏后应进入 Stub，再自动返回主程序。
- 查看 `GBAStationNDSStub.log` 中是否包含 ROM 路径。

### 构建记录

- 第一次 Switch 构建失败：
  - 原因：Stub target 命名为 `GBAStationNDSStub.elf`，CMake 输出实际变成 `GBAStationNDSStub.elf.elf`。
  - `elf2nro` 使用固定输入名 `GBAStationNDSStub.elf`，因此报错 `Failed to open input!`。
- 已修复：
  - Stub target 改名为 `GBAStationNDSStub`。
  - `elf2nro` 输入改为 `$<TARGET_FILE:GBAStationNDSStub>`。
- 第二次 Switch 构建通过：
  - 主程序：`build_switch/GBAStation.nro`
  - 大小：`26.71 MB`
  - Stub：`build_switch/GBAStationNDSStub.nro`
  - 大小：`0.29 MB`
  - 结论：阶段A最小 Stub NRO 链式启动闭环代码已通过编译和打包验证。
- 第三次 Switch 构建通过：
  - 新增启动前文件存在性检查：
    - NDS NRO 不存在时不退出主程序。
    - ROM 不存在时不退出主程序。
  - 产物大小保持：
    - `GBAStation.nro`：`26.71 MB`
    - `GBAStationNDSStub.nro`：`0.29 MB`

## 2026-07-03 阶段B：主程序移除 NDS 运行时，Stub 接管 NDS 入口数据

### 目标

- `GBAStation.nro` 不再链接 melonDS，不再进入旧 NDS `GamePage/GameView` 或 Deko probe 路径。
- Switch 主程序打开 NDS 游戏时只负责把 ROM path 传给独立 NRO。
- 独立 NRO 路径固定为 `/GBAStation/core/GBAStationNDSStub.nro`。
- `GBAStationNDSStub.nro` 根据传入 ROM path 读取 `/GBAStation/data/GameData_NDS.json` 中的 GameDB 数据。

### 已实施

- 主程序 CMake：
  - 排除 `src/emulator/melonds/**`。
  - 排除旧 Deko 占位页 `NdsDekoGamePage/View/MenuView`。
  - 移除主程序对 `melonds_core` 的链接。
  - 移除 Switch 主程序额外 `deko3d` 链接。
- `third_party/CMakeLists.txt`：
  - 新增 `BUILD_MELONDS_CORE` 开关，默认 `OFF`。
  - melonDS 静态库仅在显式开启时构建。
- `switchbuild.sh`：
  - 打包后额外复制 Stub 到 `build_switch/GBAStation/core/GBAStationNDSStub.nro`，与运行时路径 `/GBAStation/core/GBAStationNDSStub.nro` 对齐。
- `StartPage`：
  - Switch 上 NDS 游戏固定走外部 NRO。
  - 启动失败时只提示错误，不回退到旧 melonDS/GameView。
  - 默认 NDS Stub 路径改为 `/GBAStation/core/GBAStationNDSStub.nro`。
- 配置迁移：
  - 如果现有配置仍是旧路径 `sdmc:/switch/GBAStationNDSStub.nro`，启动时自动迁移为 `/GBAStation/core/GBAStationNDSStub.nro`。
- `EmulatorCoreFactory`：
  - 移除 `MelonDSCore` include。
  - `EmuNDS` 返回 `nullptr`，防止旧路径重新拉入 melonDS。
- `GBAStationNDSStub.nro`：
  - 读取 `/GBAStation/data/GameData_NDS.json` 或 `sdmc:/GBAStation/data/GameData_NDS.json`。
  - 按原始 path 和去掉 `sdmc:` 后的规范化 path 匹配记录。
  - 参数解析兼容 `argv[1]=rom` 和 `argv[1]=nro, argv[2]=rom` 两种形式。
  - 将匹配到的标题、存档路径、金手指路径、截图路径、NDS 布局和内部分辨率写入 `GBAStationNDSStub.log`。
- 主程序公共符号：
  - 新增 `src/emulator/IEmulatorVideoTexture.cpp`，把 `EmulatorGLMutex()` 从 melonDS 侧移到主程序公共实现。
- NDS 金手指解析：
  - `CheatSystem.cpp` 中的 melonDS `usrcheat.dat` 解析依赖改为 `GBASTATION_ENABLE_MELONDS_CHEAT_DAT` 条件编译。
  - 主 NRO 默认不再 include `ARDatabaseDAT.h / CRC32.h / NDS_Header.h`。

### 待验证

- Switch 构建通过。
- 主程序 NRO 体积应下降，且不应出现 `Linking melonDS (NDS) native core statically`。
- SD 卡放置路径：

```text
/GBAStation/core/GBAStationNDSStub.nro
/GBAStation/data/GameData_NDS.json
```

- 启动任意 NDS 游戏后，Stub 日志应包含：

```text
GBAStationNDSStub: romPath=...
GBAStationNDSStub: gameDb.found=1
GBAStationNDSStub: gameDb.title=...
```

### 构建记录

- 第一次阶段B构建失败：
  - 原因：`CheatSystem.cpp` 直接 include melonDS 的 `ARDatabaseDAT.h`。
  - 处理：将 melonDS `usrcheat.dat` 解析放入 `GBASTATION_ENABLE_MELONDS_CHEAT_DAT` 条件编译，主程序默认不编译。
- 第二次阶段B构建失败：
  - 原因：`GameView.cpp` 引用的 `EmulatorGLMutex()` 过去由 `MelonDSCore.cpp` 提供。
  - 处理：新增公共实现 `src/emulator/IEmulatorVideoTexture.cpp`。
- 第三次阶段B构建通过：
  - CMake 输出只包含：
    - `Linking FCEUmm (NES) core statically`
    - `Linking Snes9x (SNES) core statically`
    - `Linking Nestopia (NES) core statically`
    - `Linking Snes9x 2005 (SNES) core statically`
    - `Linking mGBA native core`
  - 未出现 `Linking melonDS (NDS) native core statically`。
  - 产物：

```text
build_switch/GBAStation.nro                         24.91 MB
build_switch/GBAStationNDSStub.nro                  0.82 MB
build_switch/GBAStation/core/GBAStationNDSStub.nro
```

  - 对比阶段A，主程序从 `26.71 MB` 降至 `24.91 MB`，说明 melonDS 静态链接已从主 NRO 移除。
- 第四次阶段B构建通过：
  - 补充旧配置路径迁移后重新验证。
  - 产物大小保持：
    - `GBAStation.nro`：`24.91 MB`
    - `GBAStationNDSStub.nro`：`0.82 MB`

## 2026-07-03 阶段C：Stub 内置渲染占位层与菜单覆盖层

### 目标

- 在 `GBAStationNDSStub.nro` 内先建立独立 UI 主循环。
- 实现一个游戏渲染占位层，用于后续接入真实 NDS framebuffer。
- 实现一个菜单覆盖层，先模仿 `GameMenuView` 的功能结构。
- 菜单具备返回游戏、保存状态、读取状态、金手指、画面设置、重置游戏、退出游戏。
- 退出游戏时通过 `envSetNextLoad()` 重新打开 `GBAStation.nro`。

### 已实施

- `NdsStubMain.cpp`：
  - 使用 libnx software framebuffer 创建 1280x720 独立渲染层。
  - 线性 framebuffer 初始化失败时会记录日志并返回主程序，避免错误格式下继续绘制。
  - 添加轻量矩形绘制、透明混合、5x7 ASCII 字体绘制。
  - 绘制 NDS 上下屏占位画面：
    - `TOP SCREEN`
    - `BOTTOM SCREEN`
    - 动态扫描条用于验证每帧刷新。
  - 绘制右侧滑入菜单覆盖层。
  - 菜单项：

```text
Resume Game
Save State
Load State
Cheats
Display Settings
Reset Game
Exit Game
```

  - 输入逻辑：
    - `PLUS`：打开/关闭菜单。
    - `MINUS`：打开菜单。
    - `方向键/摇杆上下`：菜单选择。
    - `A`：执行当前菜单项。
    - `B`：返回游戏。
  - 菜单动作：
    - `Resume Game`：关闭菜单。
    - `Save/Load/Cheats/Display/Reset`：先写占位状态文字和日志。
    - `Exit Game`：调用 `envSetNextLoad(returnNro)`，退出后回到主程序。
  - 保留 GameDB 读取，标题和 NDS 内部分辨率会显示在占位游戏层。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.91 MB
GBAStationNDSStub.nro 0.86 MB
```

- 输出路径：

```text
build_switch/GBAStation.nro
build_switch/GBAStationNDSStub.nro
build_switch/GBAStation/core/GBAStationNDSStub.nro
```

### 待实机验证

- 打开 NDS 游戏后应进入 Stub 占位游戏画面。
- `PLUS` 应显示右侧滑入菜单。
- `B` 或 `Resume Game` 应返回占位游戏画面。
- `Exit Game` 应重新拉起 `GBAStation.nro`。

## 2026-07-03 阶段D：NRO 切换退出顺序隐患检查与收敛

### 检查结论

- 当前 Switch 前端并不是普通 `OpenGL/EGL/GLES` 渲染链，Borealis 使用的是 `nanovg_dk` + `deko3d`：
  - `SwitchVideoContext` 创建 `dk::Device`、`dk::Queue`、deko swapchain。
  - `destroyFramebufferResources()` 在销毁 swapchain/framebuffer 前会调用 `queue.waitIdle()`。
- 因此当前真实链路更接近：

```text
GBAStation.nro Borealis/deko3d
    -> GBAStationNDSStub.nro software framebuffer
    -> GBAStation.nro Borealis/deko3d
```

- 仍存在两个需要收敛的退出时序风险：
  - 主程序点击 NDS 时原先立即调用 `envSetNextLoad()`，随后才请求 Borealis 退出。
  - `BKAudioPlayer` 由 `new` 创建并交给 Borealis 自定义播放器指针，但 Borealis 注释明确 caller retains ownership，`Application::exit()` 不会删除它；它内部有 audout 线程和 audout 服务。
  - Stub 选择 `Exit Game` 时原先先调用 `envSetNextLoad(returnNro)`，随后才跳出循环并关闭 framebuffer。

### 已实施

- `NroLauncher` 改为两阶段：
  - `launchNroOnExit()` 只做路径校验并登记 pending NRO，不再立刻调用 `envSetNextLoad()`。
  - 新增 `commitPendingNroLaunch()`，用于主循环退出并清理完成后再实际调用 `envSetNextLoad()`。
- `main.cpp`：
  - 保存 `BKAudioPlayer*` 所有权。
  - `brls::Application::mainLoop()` 返回后，等待更新线程、停止 WebService。
  - 主动调用 `ThreadPool::shutdown()`，丢弃尚未执行的后台任务并等待正在执行的任务结束，避免全局线程池等到 `main` 返回后才析构。
  - 调用 `brls::Application::setAudioPlayer(nullptr)` 并 `delete audioPlayer`，确保 audout 线程 join、audoutStop/audoutExit 执行。
  - 最后调用 `commitPendingNroLaunch()`，让 next-load 设置发生在主 UI 与音频清理之后。
- `GBAStationNDSStub.nro`：
  - 菜单 `Exit Game` 只设置 `pendingReturnToMain` 并退出循环。
  - 主循环结束后先 `framebufferClose(&fb)`。
  - framebuffer 释放完成后再调用 `envSetNextLoad(returnNro)`。

### 后续接入真实 NDS/deko 渲染时的硬性要求

- Stub 内真实 NDS 渲染线程、模拟线程、音频线程必须先停止并 join。
- deko 渲染层必须在返回主程序前执行：

```text
queue.waitIdle()
destroy swapchain/framebuffers
destroy command buffers / memory blocks / renderer
```

- `envSetNextLoad()` 必须保持在上述 cleanup 之后。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 0.86 MB
```

- 输出路径：

```text
build_switch/GBAStation.nro
build_switch/GBAStationNDSStub.nro
build_switch/GBAStation/core/GBAStationNDSStub.nro
```

## 2026-07-03 阶段E：Stub 接入真实 melonDS 软件渲染运行链

### 目标

- 让 `GBAStationNDSStub.nro` 不再只是占位画面，而是能直接启动传入的 NDS ROM。
- 保持 NDS 运行逻辑只存在于 Stub NRO，不把 melonDS 链回 `GBAStation.nro`。
- 先使用 x1 软件渲染 framebuffer，避免重新引入 OpenGL/deko 切换风险。

### 已实施

- CMake：
  - Switch 构建时重新构建 `melonds_core`。
  - `melonds_core` 只链接到 `GBAStationNDSStub`，主程序仍不链接 melonDS。
- 新增 Stub 专用 melonDS 平台层：
  - `src/platform/switch/nds_stub/NdsStubMelonPlatform.hpp`
  - `src/platform/switch/nds_stub/NdsStubMelonPlatform.cpp`
  - 提供 melonDS 需要的文件、线程、信号量、互斥锁、时间、日志、SRAM 写回、麦克风/摄像头/网络空实现。
- `NdsStubMain.cpp`：
  - 新增最小 `NdsRuntime`。
  - 加载：

```text
sdmc:/GBAStation/bios/nds/bios9.bin
sdmc:/GBAStation/bios/nds/bios7.bin
sdmc:/GBAStation/bios/nds/firmware.bin
```

  - 使用 ARM64 JIT，`FastMemory=false`。
  - 使用 melonDS threaded software renderer。
  - 读取 ROM path 并 `ParseROM -> SetNDSCart -> Reset -> SetupDirectBoot -> Start`。
  - 每帧执行 `RunFrame()`，抓取 `GPU.Framebuffer[FrontBuffer]`。
  - 上屏绘制到左侧 512x384，下屏绘制到右侧 512x384。
  - 退出前调用 `runtime.stop()`，保存 SRAM 并停止 NDS。
  - `Reset Game` 已连接到真实 NDS reset。
  - Plus/Minus 保持菜单触发；临时将 NDS Start/Select 额外映射到右摇杆按下/左摇杆按下。

### 当前限制

- 暂未接音频。
- 暂未接触摸。
- 菜单里的保存状态/读取状态仍是占位，当前只做 SRAM 自动保存。
- 目前是 x1 软件渲染路径，用于先验证真实游戏启动、画面、退出和 SRAM。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.50 MB
```

- 输出路径：

```text
build_switch/GBAStation.nro
build_switch/GBAStationNDSStub.nro
build_switch/GBAStation/core/GBAStationNDSStub.nro
```

## 2026-07-03 阶段F：Stub 补齐音频、触摸与 TabFrame 风格菜单

### 目标

- 在独立 `GBAStationNDSStub.nro` 内补齐基础游戏音频。
- 接入 Switch 触摸屏到 NDS 下屏触摸。
- 将占位右侧滑入菜单改为尽量接近 `GameMenuView` 的 TabFrame 结构。

### 已实施

- 音频：
  - 新增 Stub 内部 `NdsAudioOutput`。
  - 使用 libnx `audout`，4 个 1024-frame 双声道 PCM16 buffer。
  - 每帧从 `m_nds->SPU.GetOutputSize()` / `ReadOutput()` 抽取 48kHz stereo PCM。
  - 音频线程绑定到核心 2。
  - 退出时停止音频线程、等待本 Stub 自己提交的 audout buffer 释放，然后 `audoutStopAudioOut()` / `audoutExit()`。
- 触摸：
  - 初始化 `hidInitializeTouchScreen()`。
  - 使用 `hidGetTouchScreenStates()` 读取第一个触点。
  - 按当前右侧下屏绘制区域 512x384 反算到 NDS 下屏 256x192。
  - 菜单打开时不向 NDS 发送触摸，避免误触。
- 菜单：
  - 替换原先右侧滑入菜单。
  - 改为全屏半透明遮罩 + 居中大面板。
  - 左侧为 tab 栏，右侧为当前 tab 内容区，结构对齐 `GameMenuView` / `TabFrame`。
  - 保留菜单项：

```text
Resume Game
Save State
Load State
Cheats
Display Settings
Reset Game
Exit Game
```

  - 当前 Stub 字体仍是内置 ASCII 点阵，因此菜单文字暂用英文；后续若要完全中文化，需要接入字体位图或移植 Borealis/NVG 字体渲染。

### 当前限制

- 音频为基础直出版，尚未做动态延迟统计和复杂重采样；输入/输出当前均按 48kHz 处理。
- 即时存档/读取状态仍未接真实 savestate。
- 金手指与显示设置面板仍是内容占位，但 TabFrame 结构已搭好。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.52 MB
```

- 输出路径：

```text
build_switch/GBAStation.nro
build_switch/GBAStationNDSStub.nro
build_switch/GBAStation/core/GBAStationNDSStub.nro
```
