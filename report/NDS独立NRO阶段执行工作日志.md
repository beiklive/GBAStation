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

## 2026-07-03 阶段G：Stub 切换到 ArcDelta Deko-only 渲染路径

### 目标

- 解决宝可梦黑 2 大场景 `RUN/EMU` 约 19ms、GPU 仅约 2% 的 CPU 软件 3D 瓶颈。
- 让 `GBAStationNDSStub.nro` 使用 ArcDelta_melonDS 的 Deko GPU2D/GPU3D 路径。
- 保持主程序 `GBAStation.nro`、libretro、mGBA 的 OpenGL/音频路径不参与 NDS Deko 初始化。

### 已实施

- 新增 `src/platform/switch/nds_stub/NdsDekoRuntime.cpp/.hpp`：
  - 初始化 `romfs + Gfx::Init() + NDS::Init() + GPU::InitRenderer(0)`。
  - 在 `DEKOGPU_ENABLED` 下，ArcDelta 的 `GPU::InitRenderer(0)` 会创建 Deko 3D renderer。
  - 使用 `GPU2D::DekoRenderer::GetFramebuffer()` 创建外部 Deko texture。
  - 每帧等待 `FramebufferReady` fence，绘制上下屏，再 signal `FramebufferPresented` fence。
  - 添加基础 FPS/RUN 耗时 overlay。
  - 保留基础音频、触摸、PLUS 菜单、重置、退出回主程序。
- 新增 `src/platform/switch/nds_stub/NdsDekoStubMain.cpp`：
  - `GBAStationNDSStub.nro` 改为 Deko-only 主入口。
  - 继续从启动参数读取 ROM path 和 `--return`。
  - 继续根据 ROM path 查询 `/GBAStation/data/GameData_NDS.json`，读取标题和 `savePath`。
- 新增 Stub 专用 Deko shader romfs 构建：
  - 在根 `CMakeLists.txt` 中生成 ArcDelta 所需的 `romfs:/shaders/*.dksh`。
  - `GBAStationNDSStub.nro` 通过 `--romfsdir=${CMAKE_BINARY_DIR}/nds_stub_romfs` 打包 shader。
- `GBAStationNDSStub` target 改为：
  - 只链接 `arcdelta_melonds_core` 和 `arcdelta_switch_gfx`。
  - 不再链接 upstream `melonds_core`，避免两个 melonDS JIT 同时定义 libnx exception handler。
- 新增 `src/platform/switch/nds_stub/NdsArcDeltaDsiDspStub.cpp`：
  - 临时替换 ArcDelta `DSi_DSP.cpp`，避免引入与当前 upstream Teakra 不兼容的 DSi DSP 依赖。
  - 该 stub 只用于先验证普通 DS 游戏路径，不提供 DSi DSP 功能。

### 关键问题与处理

- 同时链接 upstream melonDS JIT 和 ArcDelta melonDS JIT 会发生：

```text
multiple definition of __libnx_exception_handler
multiple definition of __nx_exception_stack
```

  因此 Stub 不能同时保留两个 NDS core。已切为 Deko-only。

- ArcDelta 的 shader 不是可选资源，缺少 `romfs:/shaders/*.dksh` 会在 `Gfx::LoadShader()` assert。已完整打包。

- ArcDelta `DSi_DSP.cpp` 依赖其自身 Teakra API 的 `GetDspMemory()`。当前阶段先不支持 DSi DSP，避免把 upstream Teakra/Savestate 符号重新带进 Stub。

### 当前限制

- 当前 `GBAStationNDSStub.nro` 已是 Deko-only；旧 x1 软件渲染 Stub 源码仍在仓库中，但不再参与 Stub target。
- 菜单是 Deko runtime 内的简化菜单，不是此前软件路径里的完整 TabFrame 菜单。
- DSi 模式/DSP 暂不支持；普通 NDS ROM 优先。
- 需要真机验证：
  - 是否能正常启动宝可梦黑 2。
  - 大场景 GPU 使用率是否明显上升。
  - FPS 是否回到或接近稳定 60。
  - 音频/触摸/退出回主程序是否稳定。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.16 MB
```

- 输出路径：

```text
build_switch/GBAStation.nro
build_switch/GBAStationNDSStub.nro
build_switch/GBAStation/core/GBAStationNDSStub.nro
```

## 2026-07-03 阶段G：Stub 中文字体、FPS 叠加与 x1 绘制节流优化

### 目标

- 在 `GBAStationNDSStub.nro` 内直接读取 Switch 系统字体，支持中文菜单显示。
- 在游戏层绘制 FPS 和单帧模拟耗时，便于真机区分模拟瓶颈和绘制瓶颈。
- 优化主循环节流，避免固定 sleep 导致有效帧时间被额外拉长。
- 在不触碰 libretro、mGBA、主程序 OpenGL 路径的前提下，降低 Stub x1 软件显示路径的 CPU 开销。

### 已实施

- 字体：
  - 新增 Stub 内部 `FontRenderer`。
  - 使用 `plInitialize(PlServiceType_User)` 和 `plGetSharedFontByType()` 读取 Switch 共享字体。
  - 字体候选包含 Standard、简体中文、扩展简体中文、繁体中文、韩文和 NintendoExt。
  - 将共享字体数据复制到 Stub 自己的 `std::vector<uint8_t>` 后再 `plExit()`，避免字体映射生命周期问题。
  - 使用 `stb_truetype` 按 codepoint/字号缓存 glyph bitmap。
  - 系统字体不可用时，尝试从 SD/romfs 的字体路径 fallback；最终仍保留 ASCII 点阵兜底。
- 中文 UI：
  - 菜单项恢复为中文：返回游戏、保存状态、读取状态、金手指、画面设置、重置游戏、退出游戏。
  - 菜单内容区说明文字改为中文。
- 性能监控：
  - 游戏画面右上角显示 `FPS` 和 `EMU xxMS`。
  - `EMU` 统计范围为 `RunFrame()` + 音频 drain + framebuffer capture，用来判断瓶颈是否在 melonDS 每帧模拟侧。
  - 当单帧模拟超过约 18ms 时写入限量 slow frame 日志，便于和真机 FPS 反馈对齐。
- 帧节流：
  - 主循环从固定每帧 sleep 16.667ms 改为按实际耗时计算剩余预算。
  - 当前帧已耗时低于 16.667ms 时才 sleep 剩余时间，避免“模拟/绘制耗时 + 固定 sleep”造成低帧率。
- 绘制优化：
  - `blitNdsScreen2x()` 增加 framebuffer 内 fast path。
  - 双屏 2x 拷贝在目标区域完全在屏幕内时，不再在每个源像素内做边界判断，也不重复计算行指针。
  - 优化只作用于 Stub 的软件 framebuffer 绘制层，不影响主程序和其他模拟器。
- 线程：
  - Stub 主线程绑定到 core 1。
  - 音频线程仍绑定到 core 2。
  - melonDS 平台线程仍由 `NdsStubMelonPlatform.cpp` 绑定到 core 3。

### 验证记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.55 MB
```

- 输出路径：

```text
build_switch/GBAStation.nro
build_switch/GBAStationNDSStub.nro
build_switch/GBAStation/core/GBAStationNDSStub.nro
```

### 后续真机观察点

- 菜单中文是否正常显示，日志中是否出现 `fonts loaded count=<n>`。
- 游戏右上角 FPS 是否稳定接近 60。
- 若 FPS 低且 `EMU` 持续大于 16ms，瓶颈主要在 melonDS 模拟/软件渲染。
- 若 `EMU` 低于 16ms 但 FPS 低，继续优化 Stub framebuffer 绘制、UI 文字绘制或 pacing。
- 若首次打开菜单短暂卡顿，优先怀疑首次中文字形生成；后续可增加启动时预热常用菜单 glyph。

## 2026-07-03 阶段H：根据黑2真机数据定位 CPU 瓶颈并启用 JIT FastMemory

### 用户观测

- 宝可梦黑2小场景：
  - FPS 稳定 60。
  - `EMU` 在 11ms 到 12ms 波动。
- 宝可梦黑2大场景：
  - FPS 降到约 40。
  - `EMU` 在 16ms 到 20ms 波动。
  - GPU 基本没有明显消耗。
  - 核心 1 和核心 2 都在约 80% 波动。

### 判断

- 小场景 `EMU 11-12ms` 且 FPS 60，说明 Stub framebuffer present、主循环 pacing、x1 显示路径本身不是固定瓶颈。
- 大场景 `EMU 16-20ms` 与 FPS 同步下降，且 GPU 不忙，说明瓶颈主要在 CPU 侧：
  - melonDS `RunFrame()`。
  - x1 软件 3D 渲染线程。
  - 或 Stub 的 `drainAudio()` / `captureFrame()` 辅助路径。
- 需要拆分 `EMU` 指标，避免继续盲目优化显示层。

### 已实施

- JIT：
  - 将 Stub 中手动关闭的 `jitArgs.FastMemory = false` 改为按 `melonDS::ARMJIT_Memory::IsFastMemSupported()` 启用。
  - Switch 平台在当前 melonDS 代码中返回支持 FastMemory。
  - 启动日志新增 `GBAStationNDSStub: JIT fastmem requested=1/0`。
  - 该改动只作用于 `GBAStationNDSStub.nro` 的 melonDS JIT 内存访问路径，不影响主程序、libretro、mGBA。
- 性能指标：
  - `runFrame()` 返回 `NdsFrameTimings`。
  - 右上角显示拆为：

```text
FPS xx  EMU xxMS
RUN xx  AUD xx  CAP xx
```

  - `RUN`：`m_nds->RunFrame()`。
  - `AUD`：SPU 输出 drain 到 Stub audout ring。
  - `CAP`：从 melonDS GPU framebuffer 转换/复制到 Stub x1 显示 buffer。
  - 慢帧日志改为 `slow frame total/run/audio/capture`。
- 音频线程：
  - `NdsAudioOutput::readSamples()` 从“有一对 stereo sample 就唤醒”改为“尽量等待填满一个 audout buffer，12ms 超时后再补齐”。
  - 目标是减少音频线程过早醒来和提交半空 buffer 的概率，降低核心 2 被音频线程占用的风险。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.55 MB
```

- 输出路径：

```text
build_switch/GBAStation.nro
build_switch/GBAStationNDSStub.nro
build_switch/GBAStation/core/GBAStationNDSStub.nro
```

### 下一轮真机判读

- 如果大场景 `RUN` 接近 `EMU`，主要瓶颈就是 melonDS 核心/软件 3D，需要继续研究 ArcDelta 的 deko3d/渲染器移植或更激进的 threaded/software renderer 优化。
- 如果 `AUD` 偶尔升高，继续扩大 audout buffer 或改为更低唤醒频率。
- 如果 `CAP` 明显升高，继续优化 framebuffer 转换和双屏拷贝。
- 如果启用 FastMemory 后出现崩溃，优先回退 `jitArgs.FastMemory`，并记录崩溃日志。

## 2026-07-03 阶段I：确认 SoftRenderer CPU 瓶颈并加入 Stub CPU Boost

### 用户观测

- 宝可梦黑2大场景：
  - FPS 约 45。
  - `EMU` 在 19ms 波动。
  - `RUN` 在 19ms 波动。
  - `AUD` 和 `CAP` 一直为 0。
  - 核心 0/1/2/3 占用约为 60-90% / 80-90% / 0-33% / 26%。
  - GPU 占用始终约 2%。

### 判断

- `RUN` 基本等于 `EMU`，而 `AUD/CAP` 为 0，说明瓶颈不在 Stub 的音频输出、framebuffer capture 或 UI 绘制。
- 当前 Stub 使用的是 upstream melonDS `SoftRenderer(true)`：
  - NDS 3D 仍由 CPU 软件渲染。
  - GPU 只参与最终系统 framebuffer present，2% 占用符合预期。
- ArcDelta_melonDS 的高性能路径不是一个简单开关，而是完整的 Switch/Deko 渲染体系：
  - `GPU2D_Deko.cpp/.h`
  - `GPU3D_Deko.cpp/.h`
  - `frontend/switch/Gfx.cpp/.h`
  - `GpuMemHeap`
  - `UploadBuffer`
  - deko shader romfs
  - GPU framebuffer fence/present 链路
- 因此根治方向是把 Stub 从 `SoftRenderer` 切到 ArcDelta 的 Deko renderer，或将 Stub 重建到 ArcDelta core/frontend 架构上。

### 已实施：CPU Boost 验证版

- 新增 Stub 内部 `SwitchCpuBoost`。
- 游戏启动前将 CPU bus 从默认 1020MHz 提升到 1224MHz。
- 退出 Stub 或返回主程序前恢复到 1020MHz。
- 支持旧系统 `pcv` 和新系统 `clkrst` 两套 API。
- 日志新增：

```text
GBAStationNDSStub: CPU clock set setting=<n> hz=<hz>
```

- 游戏底部状态显示当前 CPU 频率：

```text
CPU: 1224MHz
```

### 目的

- 如果 1224MHz 后黑2大场景 `RUN` 从约 19ms 降到 16ms 以内，说明当前 x1 软件渲染可以先靠轻度 CPU boost 稳住。
- 如果仍然高于 16ms，则说明 CPU 频率也无法兜住，必须进入 Deko renderer 移植阶段。
- 该改动不影响主程序、libretro、mGBA，也不改变当前 audout 和 framebuffer capture 链路。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.56 MB
```

- 输出路径：

```text
build_switch/GBAStation.nro
build_switch/GBAStationNDSStub.nro
build_switch/GBAStation/core/GBAStationNDSStub.nro
```

### 下一步

- 真机验证 1224MHz 下黑2大场景 `RUN` 和 FPS。
- 若 `RUN <= 16ms`，保留 1224MHz 作为临时性能档，并后续做菜单可调。
- 若 `RUN > 16ms`，进入 ArcDelta Deko renderer 接入阶段：
  - 优先尝试构建独立 `ArcDelta core + Gfx + Deko renderer` 的 Stub 专用目标。
  - 让 `GBAStationNDSStub.nro` 直接使用 Deko framebuffer，而不是当前 software framebuffer。
  - 保持主程序仍不链接 melonDS，不影响 libretro 和 mGBA。

### 追加：CPU Boost 已撤销

- 用户已在系统层将 CPU 超频到 1785MHz，不需要 Stub 内部再修改 CPU clock。
- Stub 内部 `SwitchCpuBoost` 已删除。
- 不再调用 `pcvInitialize` / `clkrstInitialize` / `clkrstSetClockRate`。
- 退出 Stub 时也不会恢复 CPU 频率，避免干扰用户外部超频配置。
- 底部 `CPU: 1224MHz` 显示已移除。
- 后续性能优化方向回到 ArcDelta Deko renderer 接入。

### 撤销构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.55 MB
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

  - 初版使用 ARM64 JIT，`FastMemory=false`；阶段 H 已改为 Switch 支持时启用 FastMemory。
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

## 2026-07-03 阶段G：Stub 根目录独立化与崩溃断点日志

### 目标

- 将 `GBAStationNDSStub` 的业务代码从主程序 `src/platform/switch/nds_stub` 迁移到根目录 `nds_stub`。
- 让 Stub 以后可以更自然地拆成独立子项目。
- 保持主程序 `GBAStation.nro` 不链接 NDS 运行时。
- 解决 Stub 日志重复写入，补充 `LoadROM` 后的崩溃断点。

### 已实施

- 新增根目录 Stub 子项目：

```text
nds_stub/CMakeLists.txt
nds_stub/include/nds_stub/NdsDekoRuntime.hpp
nds_stub/include/nds_stub/StubLog.hpp
nds_stub/src/NdsDekoStubMain.cpp
nds_stub/src/NdsDekoRuntime.cpp
nds_stub/src/NdsArcDeltaDsiDspStub.cpp
```

- 根 `CMakeLists.txt` 中启用：

```cmake
add_subdirectory(nds_stub)
```

- `GBAStationNDSStub` 现在从 `nds_stub/src` 编译，构建日志中可见：

```text
nds_stub/CMakeFiles/GBAStationNDSStub.dir/src/NdsDekoStubMain.o
nds_stub/CMakeFiles/GBAStationNDSStub.dir/src/NdsDekoRuntime.o
```

- ArcDelta 的 DSi DSP stub 改为引用：

```text
nds_stub/src/NdsArcDeltaDsiDspStub.cpp
```

- Stub 日志改为第一个可写路径成功后停止，避免 `sdmc:/...` 和 `/...` 别名导致同一行重复写入。
- 在 Deko runtime 中补充 `LoadROM` 后的断点日志：

```text
audio.start begin/result
first loop begin
first RunFrame begin/ok
first Gfx::StartFrame begin/ok
first Gfx::EndFrame begin/ok
```

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.16 MB
```

- 输出路径：

```text
build_switch/GBAStation.nro
build_switch/GBAStationNDSStub.nro
build_switch/GBAStation/core/GBAStationNDSStub.nro
```

### 当前判断

- `GBAStation.nro` 未链接 `melonds_core` / `arcdelta_melonds_core`。
- `third_party` 里旧 `melonds_core` 目标仍会被构建，但不进入主程序链接链；后续可以继续清理为仅 Stub 需要时构建。
- 若再次在 `LoadROM loaded=1` 后崩溃，新日志应能判断崩在 `audout`、第一帧模拟、Deko 开帧或 Deko 结束帧。

## 2026-07-03 阶段H：Stub 拆分为游戏层与模拟器菜单层

### 目标

- 让 `GBAStationNDSStub` 的个性化修改集中在 Stub 自己内部。
- 将原本混在 `NdsDekoRuntime.cpp` 主循环中的画面布局、触摸映射和菜单 UI 拆成独立层。
- 保持 ArcDelta/melonDS 调用顺序不变，降低引入新崩溃变量的风险。

### 已实施

- 新增通用类型：

```text
nds_stub/include/nds_stub/NdsStubTypes.hpp
```

- 新增游戏层：

```text
nds_stub/include/nds_stub/NdsGameLayer.hpp
nds_stub/src/NdsGameLayer.cpp
```

职责：

```text
NDS 双屏布局
Deko 外部 framebuffer texture 创建/释放
上下屏绘制
Switch 触摸坐标 -> NDS 下屏坐标
```

- 新增菜单层：

```text
nds_stub/include/nds_stub/NdsMenuLayer.hpp
nds_stub/src/NdsMenuLayer.cpp
```

职责：

```text
Plus/Minus 打开菜单
B 返回游戏
X 请求重置游戏
A 请求退出游戏
FPS/RUN 指标绘制
TabFrame 风格菜单绘制雏形
```

- `NdsDekoRuntime.cpp` 现在主要作为调度器：

```text
输入读取
菜单动作分发
NDS::RunFrame
SPU 音频抽取
Gfx::StartFrame
NdsGameLayer::drawScreens
NdsMenuLayer::draw
Gfx::EndFrame
```

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.16 MB
```

### 后续接管方向

- 当前已经接管 Stub 业务层，之后个性化菜单、显示布局、滤镜、按键逻辑应优先修改 `nds_stub`。
- 下一步可以把 `arcdelta_melonds_core` / `arcdelta_switch_gfx` 的 CMake target 从 `third_party/CMakeLists.txt` 搬到 `nds_stub/CMakeLists.txt`，使 NDS 专用三方库只暴露给 Stub 子项目。
- 再下一步如果需要深改 ArcDelta/melonDS，可在 `nds_stub/vendor` 或独立 fork 目录中维护 Stub 专属补丁，避免污染主程序和其他模拟器核心。

## 2026-07-03 阶段I：修复 Deko 首帧绘制阶段闪退

### 现象

用户日志显示首帧已经完成：

```text
first RunFrame ok
first Gfx::StartFrame begin
first Gfx::StartFrame ok
```

随后闪退，说明崩溃点位于：

```text
Gfx::StartFrame
  -> Stub 绘制双屏 / 菜单 / FPS
  -> Gfx::EndFrame
```

之间。

### 原因判断

- ArcDelta 的 `Gfx::DrawRectangle()` / `Gfx::DrawText()` 内部会读取 `ScissorStack[ScissorStack.size() - 1]`。
- ArcDelta 原版前端在每帧 `StartFrame()` 后都会调用：

```cpp
Gfx::PushScissor(0, 0, screenWidth, screenHeight);
```

并在 `EndFrame()` 前调用：

```cpp
Gfx::PopScissor();
```

- Stub 之前直接调用 `DrawRectangle()` / `DrawText()`，但没有初始化 scissor stack，第一次绘制就可能访问空 vector 并崩溃。

### 已实施

- 在 `NdsDekoRuntime.cpp` 的每帧 Deko 绘制阶段补齐：

```cpp
Gfx::PushScissor(0, 0, kScreenWidth, kScreenHeight);
gameLayer.drawScreens();
menuLayer.draw(fps, lastRunMs);
Gfx::PopScissor();
```

- 增加首帧 checkpoint：

```text
first Gfx::PushScissor begin/ok
first gameLayer.drawScreens begin/ok
first menuLayer.draw begin/ok
first Gfx::PopScissor begin/ok
first Gfx::EndFrame begin/ok
```

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.16 MB
```

### 验证重点

- 若新日志能到 `first Gfx::EndFrame ok`，说明首帧 Deko 绘制闪退已修复。
- 若仍闪退，查看最后一条 checkpoint，即可继续定位到双屏绘制、菜单文字绘制或 EndFrame/present。

## 2026-07-03 阶段J：扩展首帧后崩溃定位日志

### 背景

- 最新测试已经能走到：

```text
first Gfx::EndFrame ok
```

- 说明上一阶段的 scissor stack 修复有效，首帧 Deko 绘制和 present 已经通过。
- 但如果仍存在启动后短时间退出，需要定位第 2 帧、第 3 帧或音频线程/后续 present 是否异常。

### 已实施

- 将原来复用 `fpsFrames == 0` 的“首帧日志”改为独立 `totalFrames`。
- 详细记录前 5 帧的关键阶段：

```text
frame=N loop begin
frame=N RunFrame begin/ok
frame=N Gfx::StartFrame begin/ok
frame=N Gfx::PushScissor begin/ok
frame=N gameLayer.drawScreens begin/ok
frame=N menuLayer.draw begin/ok
frame=N Gfx::PopScissor begin/ok
frame=N Gfx::EndFrame begin/ok
```

- 每 60 帧输出一次 heartbeat：

```text
Deko heartbeat frame=60 fps=... run=...ms
```

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.16 MB
```

### 验证重点

- 如果能看到 `Deko heartbeat frame=60`，说明 Stub 已经稳定运行至少约 1 秒。
- 如果只能停在某个 `frame=1..4` 的某一步，下一步直接针对最后一条 checkpoint 所在模块修复。

## 2026-07-03 阶段K：验证第三帧 acquireImage 卡死的 framebuffer fence 互锁

### 现象

最新日志停在：

```text
frame=2 Gfx::StartFrame begin
```

未打印：

```text
frame=2 Gfx::StartFrame ok
```

`Gfx::StartFrame()` 第一行是：

```cpp
SwapchainSlot = PresentQueue.acquireImage(Swapchain);
```

这说明前两帧已经 submit/present，但第三帧获取 swapchain image 时卡住或崩溃。结合 Deko 双缓冲，较像前两帧 present 队列中存在未完成等待，导致 swapchain image 没释放。

### 判断

- Stub 原先按 ArcDelta 原版顺序提交：

```text
WaitForFenceReady(FramebufferReady[FrontBuffer])
DrawRectangle(...)
SignalFence(FramebufferPresented[FrontBuffer])
```

- 如果 `FramebufferReady` 没有在当前 Stub 调度下正确 signal，present 队列会等住。
- present 队列一旦等住，前两个 swapchain image 被占满，第三帧 `acquireImage()` 就会卡住。

### 已实施

- 在 `NdsGameLayer` 增加 `setWaitForFramebufferReady(bool)`。
- 当前验证模式设置为：

```cpp
gameLayer.setWaitForFramebufferReady(false);
```

- 即：

```text
暂不等待 FramebufferReady
仍然 SignalFramebufferPresented
```

- 启动日志会显示：

```text
Deko display fence mode=signal-presented-only
```

### 验证目的

- 如果此模式能跑到：

```text
Deko heartbeat frame=60
```

则说明第三帧卡死基本由 `FramebufferReady` wait 互锁造成。
- 下一步需要实现更正确的同步策略，例如只在 ready fence 确认已可等待后再加入 present 队列，或改用 CPU/队列 idle 方式验证后再优化。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.16 MB
```

## 2026-07-03 阶段L：修复音频电流声、菜单键与中文字体

### 现象

- Deko Stub 画面运行流畅，但声音存在严重电流声。
- 模拟器菜单触发键需要改为 `ZR`。
- 菜单中文显示不完整，判断为未加载 Switch 中文系统字体。

### 音频修复

- 原 Stub 使用 `audout`，实际按 48kHz 输出。
- ArcDelta/melonDS 的 `SPU::ReadOutput()` 输出约为 32823Hz。
- 直接把 32823Hz PCM 塞到 48kHz audout，会造成采样率错配、补零和爆音。
- 已改为贴近 ArcDelta 原版的 `audren` + `audrv`：

```text
AudioRenderer output: 48kHz
Voice sample rate:    32823Hz
Buffer frame size:    768 stereo frames
```

- 音频线程现在直接从 `SPU::ReadOutput()` 读取，不再由主循环抽样后 push 到 audout ring。

### 菜单按键

- 菜单打开/关闭键改为：

```text
ZR
```

- `ZR` 不再映射为 NDS 的 `R` 键，避免关闭菜单瞬间误触发游戏内 R。
- `R` 仍映射为 NDS 的 `R`，`ZL` 仍映射为 NDS 的 `L`。

### 中文字体

- 在 ArcDelta Switch Gfx 层新增：

```cpp
Gfx::SystemFontChinese
```

- 加载顺序：

```text
PlSharedFontType_ChineseSimplified
PlSharedFontType_ExtChineseSimplified
fallback: Standard
```

- Stub 菜单中文文本改用 `Gfx::SystemFontChinese` 绘制。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.16 MB
```

### 验证重点

- 进入游戏后听音频是否还存在明显电流声/爆音。
- `ZR` 是否打开/关闭模拟器菜单。
- 菜单中文是否完整显示。

## 2026-07-03 阶段M：整理主程序 NDS 功能迁移清单

### 背景

后续需要在 `GBAStationNDSStub.nro` 中复刻主程序 `GameView` 和 `GameMenuView` 中与 NDS 相关的运行时、渲染、触摸、菜单和 GameDB 设置能力。

### 产物

已新增报告：

```text
report/NDS主程序GameView_GameMenuView功能梳理.md
```

### 主要结论

- 主程序 NDS 功能由 `GameView` 运行时逻辑、`GameMenuView` TabFrame 菜单、以及 `GamePage` callback 注入共同组成。
- 需要迁移的核心字段包括：
  - `ndsScreenLayout`
  - `ndsScreenOrientation`
  - `ndsIntegerScale`
  - `ndsTopOffsetX/Y/Scale`
  - `ndsBottomOffsetX/Y/Scale`
  - `ndsInternalResolution`
  - `cheatPath`
  - `savePath`
- Switch 版主程序已强制 NDS `ndsInternalResolution = 1`，stub 当前也应继续保持 x1 优先。
- 主程序 OpenGL 渲染链、CPU 重排整张画布、以及 NDS 快进只跑 1 帧的保守策略不建议照搬到 Deko stub。
- 建议先在 stub 内新增独立 `NdsLayout` 纯计算模块，再让 `NdsGameLayer` 和 `NdsMenuLayer` 共享布局、UV 和触摸映射。

### 后续建议

下一阶段优先补：

1. `NdsLayout`：布局矩形、旋转 UV、触摸坐标映射。
2. `NdsGameLayer`：支持七种屏幕布局。
3. `NdsMenuLayer`：还原 TabFrame 风格菜单结构。
4. 保存/读取状态 10 槽位。
5. 金手指读取、列表、开关和应用。

## 2026-07-03 阶段N：修复 Stub 菜单重置游戏崩溃

### 现象

- 在 `GBAStationNDSStub.nro` 的模拟器菜单中执行“重置游戏”会导致程序崩溃。

### 原因判断

- 原实现直接在运行中的 Deko 主循环里调用：

```cpp
NDS::Reset();
NDS::SetupDirectBoot();
```

- `NDS::Reset()` 会重置 GPU、SPU、卡带、Wifi、JIT 等全局模块。
- Stub 当前有独立音频线程持续调用 `SPU::ReadOutput()`，直接 Reset 可能与音频线程并发访问 SPU 音频缓冲。
- 单独调用 `NDS::SetupDirectBoot()` 也不等价于完整 ROM 重新加载，卡带侧 direct boot setup、SRAM 路径和 ROM 装载状态不够完整。

### 已实施

- `DekoAudioOutput` 新增 reset 暂停栅栏：

```cpp
pauseForCoreReset()
resumeAfterCoreReset()
```

- 音频线程读取 SPU 前增加 `m_spuReadMutex`，重置前会等待当前 `SPU::ReadOutput()` 退出，并阻止新的 SPU 读取。
- 菜单 Reset 动作改为安全重载当前 ROM：

```text
关闭菜单
释放 NDS 按键和触摸
暂停音频线程读取 SPU
等待 PresentQueue / EmuQueue idle
Flush SRAM
NDS::LoadROM(currentRom, currentSave, true)
恢复音频线程
```

- 重置后会写日志：

```text
GBAStationNDSStub: Deko reset begin
GBAStationNDSStub: Deko reset LoadROM loaded=1
```

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.16 MB
```

### 验证重点

- 进入 NDS 游戏后打开菜单，执行重置游戏。
- 预期：
  - 程序不崩溃。
  - 菜单关闭。
  - 游戏重新从启动流程开始。
  - 日志出现 `Deko reset LoadROM loaded=1`。
- 如果仍崩溃，优先查看最后一条日志是否停在 `Deko reset begin`、队列 idle、还是 `LoadROM loaded` 之后。

## 2026-07-03 阶段O：Stub 画面过滤、菜单输入屏蔽、快进与分辨率入口

### 目标

- 当前游戏画面默认改为 Nearest 输出，避免 Linear 放大导致像素发糊。
- 关闭模拟器菜单后，菜单确认/返回按键不能穿透给游戏。
- 在 Stub 模拟器菜单内加入快进倍率和内部分辨率倍率设置，默认均为 x1，档位为 x1/x2/x3/x4。

### 已实施

- `NdsGameLayer` 增加画面过滤状态：

```text
Nearest 默认
Linear 可在菜单内切换
```

- 绘制 NDS 双屏时根据菜单设置选择 Deko sampler：

```text
Nearest + ClampToEdge
Linear  + ClampToEdge
```

- `NdsMenuLayer` 新增菜单项和设置状态：

```text
画面过滤：Nearest / Linear
快进倍率：x1 / x2 / x3 / x4
内部分辨率：x1 / x2 / x3 / x4
```

- 关闭/切换菜单后启用输入屏蔽：

```text
等待所有按键释放
等待触摸屏释放
之后才恢复 NDS 按键和触摸输入
```

- 右摇杆按下不再映射为 NDS Start，避免和 Stub 快进/菜单扩展键位冲突。
- 非菜单状态下菜单倍率大于 x1 时，主循环每次运行多帧 `NDS::RunFrame()`：

```text
x1：正常速度
x2：每轮 2 帧
x3：每轮 3 帧
x4：每轮 4 帧
```

- 快进激活时跳过 60FPS sleep 限速，日志 heartbeat 会输出当前快进、分辨率和过滤状态。

### 分辨率说明

- Stub 侧已经接通 `GPU::SetRenderSettings(0, RenderSettings{true, scale, false})`。
- 但 ArcDelta Deko 3D renderer 当前 `GPU3D::DekoRenderer::SetRenderSettings()` 为空实现，且 Deko 2D/3D 路径大量固定为 `256x192`。
- 因此本阶段完成的是菜单入口与设置链路；真正 x2/x3/x4 内部高分辨率渲染需要后续改造 ArcDelta 的 Deko renderer framebuffer、tile buffer、compute dispatch 和 2D 合成尺寸。

### 验证重点

- 进入 NDS 游戏后画面默认应为 Nearest 像素风格。
- 菜单内切换 Linear 后画面应变为平滑放大。
- 用 A/B/ZR 关闭菜单后，关闭菜单用到的按键不应立刻传入游戏。
- 触摸屏按住菜单关闭区域时，释放前不应传入 NDS 下屏触摸。
- 设置快进倍率为 x2/x3/x4 后，关闭菜单回到游戏应立即触发快进。
- 内部分辨率菜单项和日志应能变化；实际清晰度提升等待后续 Deko renderer 改造。

## 2026-07-03 阶段P：修正快进倍率切换不生效

### 现象

- 菜单内快进倍率可以切到 x2/x3/x4，但回到游戏后没有明显加速。

### 原因

- 阶段O实现为“菜单倍率 + 按住右摇杆”双条件触发：

```text
fastForwardActive = multiplier > 1 && StickR held
```

- 用户实际需要的是菜单切换倍率后立即生效，因此只切倍率但不按右摇杆时不会加速。

### 已实施

- 快进触发条件改为：

```text
非菜单状态
输入屏蔽已解除
快进倍率 > x1
```

- `x1` 表示关闭快进，`x2/x3/x4` 表示持续按对应倍率运行。
- 菜单提示改为“快进倍率高于 x1 后立即生效”。

### 验证重点

- 菜单中把快进倍率改为 x2，返回游戏后应立即加速。
- 再改回 x1，返回游戏后应恢复正常 60FPS 限速。
- heartbeat 日志中的 `ff=` 应随实际倍率变化。

## 2026-07-03 阶段Q：修复快进状态下音频爆音

### 现象

- 快进倍率生效后，游戏速度提升，但声音出现明显爆音/电流声。

### 原因判断

- 快进时主循环一轮会连续执行多次 `NDS::RunFrame()`。
- SPU 每个模拟帧都会产生音频，音频生成速度随快进倍率上涨。
- audren/audrv 音频线程仍按正常播放节奏读取 768 帧缓冲。
- 如果不处理 SPU 输出队列，旧样本会积压并发生 FIFO 裁剪/错位，表现为爆音。

### 已实施

- `DekoAudioOutput` 增加快进音频模式：

```text
setFastForwardActive(true/false)
```

- 进入快进时调用 `SPU::TrimOutput()`，先把输出队列裁到安全长度。
- 快进中音频线程每次 `SPU::ReadOutput()` 前调用：

```cpp
SPU::Sync(false);
```

- 快进主循环每跑完一个额外模拟帧后也调用 `SPU::Sync(false)`。
- 退出快进时调用 `SPU::DrainOutput()`，避免残留的快进音频在恢复正常速度后继续播放。

### 策略说明

- 本阶段没有选择“快进静音”，而是采用“快进时丢弃积压音频”的策略。
- 预期效果是快进期间声音可能不连续，但不应再出现严重爆音/电流声。

### 验证重点

- x1 正常速度下声音应保持原状。
- x2/x3/x4 快进时应明显减少或消除爆音。
- 从 x4 改回 x1 后，不应继续播放快进期间积压的旧声音。

## 2026-07-03 阶段R：评估 Deko3D 多倍分辨率实现

### 目标

- 实现基于 Deko3D 加速的 NDS 多倍内部分辨率。
- 参考 `third_party/melonDS` 和 `third_party/melonDS-switch` 中的多倍分辨率代码。

### 结论

- `third_party/melonDS` 和 `third_party/melonDS-switch` 中存在 OpenGL / OpenGL Compute 的多倍分辨率实现。
- 当前使用的 `third_party/ArcDelta_melonDS` Deko renderer 没有对应实现。
- `GPU3D::DekoRenderer::SetRenderSettings()` 为空。
- Deko 3D compute shader、3D buffer、2D compositor、最终 framebuffer 都固定 `256x192`。

### 关键判断

- 只改 Stub 菜单或调用 `GPU::SetRenderSettings()` 不会产生真实多倍分辨率。
- 只把 3D framebuffer 放大也不够，因为最终 `GPU2D_Deko::FinalFramebuffers` 仍是 `256x192`，高分辨率 3D 会被压回原生尺寸。
- 真正可见的多倍分辨率必须同时改：

```text
GPU3D_Deko runtime dimensions
GPU3D_Deko buffer allocation
GPU3D_Deko shader scale variants
GPU2D_Deko high-res 3D framebuffer
GPU2D_Deko high-res final framebuffer
ComposeBGOBJ scale-aware compositor
NdsGameLayer external texture recreation
```

### 产出

- 已制定完整分阶段实现方案：

```text
report/NDS_Deko3D多倍分辨率实现方案.md
```

### 下一步建议

- 先实施方案中的阶段 1：Deko 3D 尺寸抽象，但保持 x1 行为不变。
- 阶段 1 通过实机验证后，再进入 shader scale variant 和高分辨率 compositor 改造。

## 2026-07-03 阶段S：实施 Deko3D 多倍分辨率阶段 1

### 目标

- 开始实施 `report/NDS_Deko3D多倍分辨率实现方案.md`。
- 本阶段只做 Deko 3D 尺寸抽象，保持 x1 渲染行为不变。
- 不改 libretro、mGBA、主程序 OpenGL 渲染和音频逻辑。

### 已实施

- `GPU3D_Deko.h` 增加运行期尺寸状态：

```cpp
RequestedScaleFactor
ScaleFactor
ScreenWidth
ScreenHeight
RuntimeTileSize
RuntimeTilesPerLine
RuntimeTileLines
RuntimeMaxWorkTiles
RuntimeMaxYSpanIndices
RuntimeMaxYSpanSetups
```

- CPU 侧固定数组改为运行期容器：

```cpp
std::vector<SetupIndices> YSpanIndices;
std::vector<SpanSetupY> YSpanSetups;
std::vector<RenderPolygon> RenderPolygons;
```

- 新增尺寸配置入口：

```cpp
ConfigureScale(int scale)
BinResultSize()
TileMemorySize()
FinalTileMemorySize()
```

- `GPU3D_Deko::Init()` 的 GPU buffer 分配改为运行期尺寸计算：

```text
YSpanSetupMemory      -> RuntimeMaxYSpanSetups
XSpanSetupMemory      -> RuntimeMaxYSpanIndices
YSpanIndicesTexture   -> RuntimeMaxYSpanIndices
BinResultMemory       -> BinResultSize()
TileMemory            -> TileMemorySize()
FinalTileMemory       -> FinalTileMemorySize()
```

- RenderFrame 上传路径改为 vector `.data()`。
- span 容量断言改为运行期容量。
- compute dispatch 中的部分 `256/192` 替换为 `ScreenWidth/ScreenHeight` 和 runtime tile 值。
- `GPU3D::DekoRenderer::SetRenderSettings()` 现在会接收 x1/x2/x3/x4 请求，但阶段 1 仍安全钳制实际渲染为 x1。
- Stub 日志改为：

```text
Deko resolution scale request accepted xN (stage1 renderer output remains x1)
```

### 当前状态

- x1 行为应保持原样。
- 菜单 x2/x3/x4 会被 Deko renderer 接收并记录请求。
- 真实高分辨率输出尚未开启，因为 Deko shader scale variants 和 GPU2D high-res compositor 尚未完成。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.17 MB
```

### 下一步

- 阶段 2：为 Deko compute shader 生成 x1/x2/x3/x4 变体。
- 阶段 3：让 `GPU3D_Deko` 按 scale 重新分配 3D buffer，并输出 high-res 3D framebuffer。

## 2026-07-03 阶段T：实施 Deko3D 多倍分辨率阶段 2

### 目标

- 为 Deko 3D compute shader 增加 x1/x2/x3/x4 编译期尺寸变体。
- 继续保持当前运行路径为 x1，不在本阶段启用 high-res 渲染。

### 已实施

- `GPU3D_Comp.glsl` 增加尺寸宏：

```glsl
NDS_DEKO_SCREEN_WIDTH
NDS_DEKO_SCREEN_HEIGHT
NDS_DEKO_TILE_SIZE
NDS_DEKO_WORK_TILE_MULTIPLIER
```

- 默认值保持原生 DS 尺寸：

```text
ScreenWidth  = 256
ScreenHeight = 192
TileSize     = 8
WorkTileMultiplier = 48
```

- shader 内部以下尺寸改为使用编译期参数：

```text
FramebufferStride
TilesPerLine
TileLines
MaxWorkTiles
ColorResult / DepthResult / AttrResult
DepthBlend / FinalPass result offset stride
```

- `nds_stub/CMakeLists.txt` 新增 `nds_stub_compile_gpu3d_shader_scaled()`。
- 现在会额外生成所有 Deko 3D compute shader 的 scale 变体：

```text
*_x1.dksh
*_x2.dksh
*_x3.dksh
*_x4.dksh
```

### x3/x4 的特殊处理

- 首次尝试让 x3/x4 沿用 x1 的 `MaxWorkTiles * 48` 时，`uam` 报错：

```text
shader storage block TilesBuffer larger than maximum allowed 134217728
```

- 因此阶段 2 对 high scale 变体先使用保守 work tile multiplier：

```text
x1: 48
x2: 48
x3: 16
x4: 8
```

- 这保证 shader 能通过 Deko 编译，但后续真正启用 x3/x4 时必须加入运行时 work overflow 保护和 fallback。

### 当前状态

- RomFS 中已包含 x1/x2/x3/x4 shader 变体。
- `GPU3D_Deko` 当前仍加载原始 shader 名称，实际渲染仍为 x1。
- 下一阶段需要让 renderer 按 scale 加载/选择这些 shader，并按 scale 分配 high-res 3D buffer。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 3.12 MB
```

### 下一步

- 阶段 3A：把 `GPU3D_Deko` 的 shader 成员改成 scale-aware shader set，并按当前 scale 选择 shader。
- 阶段 3B：让 x2 先真实分配 high-res 3D buffers，并只开放 x2 实验路径。

---

## 阶段 3：接通 Deko3D 多倍分辨率运行路径

### 目标

解决菜单中 x2/x3/x4 分辨率切换后画面没有变化的问题。上一阶段只生成了 shader 变体，但运行时仍固定加载和绑定 x1 shader，最终 framebuffer 也仍是 256x192。

### 已实施

- `GPU3D_Deko` 改为 scale-aware shader set：

```text
ShaderInterpXSpans[scale][z/w]
ShaderBinCombined[scale]
ShaderDepthBlend[scale][z/w]
ShaderRasterise...[scale][z/w]
ShaderFinalPass[scale][variant]
```

- 运行时按当前 `ScaleFactor` 绑定对应的 x1/x2/x3/x4 compute shader。
- `GPU3D_Deko::ConfigureScale()` 现在会真正设置：

```text
ScreenWidth  = 256 * scale
ScreenHeight = 192 * scale
TilesPerLine / TileLines / RuntimeMaxWorkTiles
RuntimeMaxYSpanIndices / RuntimeMaxYSpanSetups
```

- 多边形扫描转换阶段改为按 scale 生成坐标：

```text
FinalPosition * ScaleFactor
或 BetterPolygons 时 HiresPosition * ScaleFactor >> 4
```

- GPU3D 工作缓冲初始化时按保守最大需求预分配，避免菜单切换倍率时重建 GPU 资源。
  - Tile buffer 按 x2 最大 work tile 需求分配；
  - BinResult buffer 按 x4 最大 mask 需求分配；
  - FinalTile buffer 按 x4 输出尺寸分配。
- `GPU2D_Deko` 最终 framebuffer 和 3D framebuffer 预留到 x4 尺寸：

```text
max output = 1024x768
current output = 256x192 / 512x384 / 768x576 / 1024x768
```

- `ComposeBGOBJ_fsh.glsl` 增加 `NDS_DEKO_COMPOSE_SCALE`：
  - 2D BG / OBJ / Window / DirectBitmap 仍按原生 256x192 坐标采样；
  - 3D BG 按 high-res 坐标采样；
  - final framebuffer 按当前 scale 输出。
- `nds_stub/CMakeLists.txt` 生成 2D compose shader 的 x1-x4 变体。
- `NdsGameLayer` 创建外部纹理时使用实际 image 最大尺寸，绘制时只采样当前有效输出区域。
- `NdsDekoRuntime` 的分辨率切换同时通知 GPU3D 与 GPU2D。

### 注意事项

- x2/x3/x4 提升的是 3D 图层分辨率；纯 2D 游戏或纯 2D 场景只会按原生图层放大，不会变成真正高清。
- x3/x4 的 `MaxWorkTiles` 为了通过 Deko shader 编译限制做了保守下调，复杂 3D 场景仍需要后续加入 overflow 检测和 fallback。
- Display Capture 路径目前沿用原生 256x192 复制逻辑，高倍率下如果游戏大量使用 capture，后续可能需要专门做 downsample/copy 适配。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 3.16 MB
```

---

## 阶段 4.6：存档槽截图、截图热键与菜单显示细节

### 需求

- 保存/读取状态页面的槽位显示对应 `ssN.png` 缩略图。
- 保存状态后同步生成并刷新缩略图。
- 如果截图功能缺失，补全截图输出。
- 修正画面设置 LR 选择器中间选项文字的居中方式。
- 加深菜单层背景，降低与底层游戏画面的混淆。

### 已实施

- 在 `GPU2D_Deko` 增加 `ReadFramebufferRGBA()`：
  - 从当前前台 top/bottom framebuffer 读回 RGBA；
  - 读回前检查命令缓冲状态，读回后等待 `EmuQueue` idle；
  - 输出两个 256x192 RGBA buffer。
- 在 `NdsGameLayer` 增加 `captureCurrentFrameRgba()`：
  - 组合为 512x192 横向双屏截图；
  - 遵循当前上下屏交换状态，截图顺序与实际显示一致。
- 在 NDS Stub runtime 增加 PNG 写入：
  - 保存状态时写入 `{romName}.ssN.png`；
  - 截图热键写入 `screenshot_YYYYMMDD_HHMMSS.png`；
  - 写入前等待 `PresentQueue` 和 `EmuQueue`，减少读到半帧或 GPU 未完成资源的风险。
- 在保存/读取槽位加载 PNG 缩略图：
  - 使用 `stbi_load(..., 4)` 读取 RGBA；
  - 创建 Deko UI texture 并上传；
  - 删除存档或重新加载槽位时释放旧 texture，避免纹理泄漏。
- `GBAStationNDSStub` target 单独加入 `stb_image.c`，让 PNG 读取 implementation 只存在于 NDS Stub 目标中。
- LR 选择器的当前值改为以中点对齐绘制，避免文字起点落在中间导致视觉偏右。
- 菜单 overlay 加深：
  - 黑色底遮罩 alpha 提升到约 `0.70`；
  - 渐变 alpha 提升到约 `0.48 -> 0.82`。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.32 MB
```

### 真机验证重点

- 打开保存/读取状态页面，已有 `ssN.png` 应显示在对应槽位。
- 保存状态后，对应槽位截图应立即刷新，不应继续显示旧缓存。
- 删除状态后，对应槽位截图应消失。
- 截图热键应在当前游戏 savePath 下生成 `screenshot_*.png`。
- 打开菜单后背景应明显更暗，文字与卡片层次更清楚。

---

## 阶段 4.7：修复缩略图启动崩溃与任意尺寸纹理上传对齐

### 现象

真机日志停在：

```text
GBAStationNDSStub: Deko checkpoint audio.start result=1 ms=0
```

之后游戏画面尚未显示即崩溃。此位置之后原先会立即扫描存档槽并加载 `ssN.png` 缩略图，包含 PNG 解码、Deko texture 创建和 `Gfx::TextureUpload()`。

### 判断

原实现有两个风险：

- 启动首帧前就加载缩略图，导致游戏进入画面前介入额外 Deko 纹理上传路径。
- `Gfx::TextureUpload()` 写入 staging buffer 后没有按 `DK_IMAGE_LINEAR_STRIDE_ALIGNMENT` 推进 offset；而 `Gfx::StartFrame()` 处理 pending upload 时会按对齐后的 GPU 地址推进并断言最终 offset 相等。任意尺寸 PNG 只要 `width * 4 * height` 不是对齐大小，就可能在首帧上传时崩溃。

### 已实施

- 启动阶段只加载即时存档元数据，不再加载 PNG 缩略图。
- 缩略图改为菜单可见后懒加载：
  - 游戏至少完成数帧后才启动；
  - 每帧最多上传 1 张缩略图；
  - 解码失败、文件过大或文件不存在时只显示占位，不阻断游戏。
- 保存/删除状态后只刷新存档元数据并标记缩略图 dirty，后续由懒加载刷新图片。
- `Gfx::TextureUpload()` 的 staging offset 改为按 `DK_IMAGE_LINEAR_STRIDE_ALIGNMENT` 对齐推进，支持任意尺寸 RGBA PNG 上传。
- 增加缩略图加载实时日志：

```text
state thumbnail lazy load begin
state thumbnail load begin slot=N path=...
state thumbnail upload queued slot=N size=WxH
state thumbnail load failed slot=N decode
state thumbnail skipped slot=N too_large=WxH
state thumbnail lazy load done
```

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.32 MB
```

### 真机验证重点

- 打开 NDS 游戏后，应先进入游戏画面，不应再停在 `audio.start` 后崩溃。
- 首次打开菜单时，日志应出现 `state thumbnail lazy load begin`，随后逐张加载。
- 如果某个旧截图尺寸异常或损坏，应只显示占位并继续运行。

---

## 阶段 4.8：移除菜单运行时 PNG 解码，改用固定 RGBA 缩略图缓存

### 现象

真机打开菜单后崩溃，日志停在：

```text
state thumbnail lazy load begin
state thumbnail load begin slot=0 path=...ss0.png
```

没有后续 `decode failed` 或 `upload queued`，说明崩溃发生在 `stbi_load()` 内部或其文件读取路径中，还没有进入 Deko texture 创建。

### 判断

继续在菜单打开时解码外部 PNG 风险过高：

- 旧 `ssN.png` 可能来自不同版本或格式参数；
- `stbi_load()` 内部不可控，崩溃时无法优雅降级；
- 菜单打开是高频路径，不应依赖复杂图片解码。

### 已实施

- 菜单不再读取或解码 `ssN.png`。
- NDS Stub 不再链接 `stb_image.c`，只保留 `stb_image_write` 用于输出 PNG。
- 保存状态时同时写两份截图：
  - `{rom}.ssN.png`：给外部查看；
  - `{rom}.ssN.thumb`：菜单专用固定 RGBA 缓存。
- 菜单缩略图懒加载只读取 `.thumb`：
  - 固定 header：magic/version/width/height/bytes；
  - 固定 RGBA8 像素；
  - 不进入 PNG decoder；
  - 文件不存在或校验失败时只显示占位。
- 删除状态时同步删除 `.ssN`、`.ssN.png`、`.ssN.thumb`。

### 兼容说明

已有旧 `ssN.png` 不会再被菜单显示。对对应槽位重新保存一次状态后，会生成新的 `.thumb`，菜单即可显示缩略图。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.26 MB
```

---

## 阶段 4.9：柔化聚焦框、缩略图状态提示与 NDS 多布局实装

### 需求

- 聚焦框四角不能有明显像素感，需要更平滑。
- 保存/读取槽位仍然看不到图片时，需要能区分是未生成缩略图还是加载失败。
- 画面布局增加并实装：
  - 纵向对称；
  - 横向对称；
  - 上屏优先；
  - 下屏优先；
  - 混合横向；
  - 自定义占位。
- 布局状态保存到 `nds.screenLayout` 字段。

### 已实施

- `drawGradientBorder()` 改为柔和圆角 halo：
  - 移除原先逐段矩形拼接边框；
  - 使用多层 `coolTransparency` 圆角矩形形成外发光和柔和内层高亮；
  - 避免四角出现硬像素折线。
- 存档槽缩略图状态更明确：
  - 没有 `.thumb` 显示 `NO THUMB`；
  - `.thumb` 加载失败显示 `LOAD FAIL`；
  - 成功加载后按比例居中显示，不拉伸变形。
- `NdsGameLayer` 实装多布局绘制：
  - 纵向对称：上屏上半区域居中，下屏下半区域居中；整数倍为 1x/1x。
  - 横向对称：上屏左半区域居中，下屏右半区域居中；整数倍为 2x/2x。
  - 上屏优先：上屏靠左主画面，下屏占右侧剩余区域；整数倍为 3x/2x。
  - 下屏优先：与上屏优先相反；整数倍为 2x/3x。
  - 混合横向：左侧 0.7 宽度为上屏主画面，右侧上下堆叠上屏和下屏；整数倍为 3x/1x/1x。
  - 自定义：暂时保留为左右对称占位。
- 触摸映射改为查找当前显示中的下屏区域，适配交换屏幕和多布局。
- 菜单布局选项更新为：

```text
纵向对称 / 横向对称 / 上屏优先 / 下屏优先 / 混合横向 / 自定义
```

- 配置读写：
  - 启动读取 `nds.screenLayout`、`nds.integerScale`、`display.filter`、`fastforward.multiplier`；
  - 菜单修改后写回 `sdmc:/GBAStation/config/config.cfg`；
  - `nds.layout.next` 热键改为循环切换布局，不再与交换上下屏混用。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.27 MB
```

---

## 阶段 4.4：NDS Stub 菜单子页面滚动与存档两列布局

### 需求

- 保存状态、读取状态、画面设置等右侧子页面内容过多时需要可滚动。
- 保存/读取页面的即时存档槽改为两列布局。
- 继续保持当前安全策略：不重新引入菜单 Tab 图标 PNG 加载，避免图片上传路径再次导致黑屏或崩溃。

### 已实施

- 右侧内容区标题和分割线固定，正文区域使用 `Gfx::PushScissor/PopScissor` 裁剪。
- 保存/读取页面改为 2 列 x 5 行：
  - 每个槽位为左右布局；
  - 左侧保留缩略图占位区；
  - 右侧显示槽位名称和存档时间；
  - 焦点仍按两列逻辑移动，与输入逻辑保持一致。
- 保存/读取页面根据当前焦点自动计算滚动偏移，底部槽位获得焦点时会滚入可视区域。
- 画面设置页面根据当前焦点自动计算滚动偏移，超出区域的设置项会滚入可视区域。
- 对正文裁剪区域增加屏幕边界保护，避免菜单底部滑出/滑入动画期间提交越界 scissor。

### 截图显示说明

当前即时存档截图仍未接入真实 PNG 解码和 Deko 纹理缓存，菜单中显示的是缩略图占位区。后续若要显示真实 `ss0.png` / `ss1.png`，建议单独做稳定图片管线：

- PNG 解码：`stb_image` 或 libpng；
- PNG 写入：`stb_image_write` 或现有最小 PNG 写入器；
- 纹理上传：在菜单打开前或存档列表刷新时集中加载，禁止在每帧绘制过程中懒加载。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.24 MB
```

---

## 阶段 4.5：游戏层状态徽标、平滑滚动与 Borealis 风格高亮

### 需求

- 去掉 NDS Stub 游戏层左上角的调试型 `FPS/RUN/FF/filter` 文本。
- 游戏层的 FPS、快进、暂停状态改成与主程序 `GameView` / `GameOverlayRenderer` 类似的徽标。
- 菜单子页面滚动过于生硬，需要平滑滚动。
- scissor 裁剪不应切掉焦点高亮框顶部和底部。
- 焦点高亮参考 Borealis `View::drawHighlight` 的阴影和流动渐变边框效果。

### 已实施

- 移除 `NdsMenuLayer::draw()` 中直接绘制的调试状态文字。
- 在游戏层绘制状态徽标：
  - FPS：左上角，受 `display.showFps` 控制；
  - 快进：右上角，受 `display.showFfOverlay` 控制；
  - 暂停：顶部居中，仅运行时暂停且非快进时显示；
  - 菜单打开时不额外显示游戏层徽标，避免和菜单 overlay 干扰。
- `InputConfig` 增加读取 `display.*` 配置项。
- 子页面滚动改为根据焦点目标进行指数平滑追踪，避免瞬间跳动。
- 内容区 scissor 增加上下左右 padding，并保留屏幕边界保护，避免高亮框边缘被裁掉。
- `drawGradientBorder()` 改为 Borealis 风格近似：
  - 外侧阴影；
  - 环形渐变流动边框；
  - 内部保持透明；
  - 取消旧的左侧光柱样式。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.24 MB
```

---

## 阶段 3.5：NDS Stub 菜单 Hint 自适应与 Tab 图标资源

### 目标

- 底部 hint 根据文字实际宽度动态排布，避免 `返回列表 / 返回 / 删除 / 确定` 等字符串互相重叠。
- 左侧 tab 图标使用主程序 `GameMenuView` 同一组图片资源。

### 已实施

- `drawFooter()` 改为从右向左动态布局：
  - 使用 `Gfx::MeasureText()` 测量中文文本宽度；
  - 每个 hint 组按 `图标宽度 + 间距 + 文本宽度` 计算实际占用；
  - 支持 `X 删除` 出现/隐藏时自动让位。
- `drawLeftMenu()` 改为优先绘制 PNG 图标：
  - 返回游戏：`img/ui/menu/back.png`
  - 保存状态：`img/ui/menu/save.png`
  - 读取状态：`img/ui/menu/load.png`
  - 金手指设置：`img/ui/menu/cheat.png`
  - 画面设置：`img/ui/menu/display.png`
  - 重置游戏：`img/ui/menu/reset.png`
  - 退出游戏：`img/ui/menu/exit.png`
- 新增轻量 PNG 纹理缓存：
  - 首次绘制时通过 `stb_image` 加载；
  - 上传为 Deko `RGBA8_Unorm` 纹理；
  - 加载失败时自动回退到原来的文字占位图标，不影响菜单可用性。
- `nds_stub/CMakeLists.txt` 增加菜单资源打包：
  - 将 `resources/img/ui/menu/*.png` 复制到 NDS Stub romfs 的 `img/ui/menu/`；
  - `GBAStationNDSStub.nro` 构建时已确认写入这些 PNG。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.32 MB
```

---

## 阶段 3.8：移除 NDS Stub 菜单图片图标链路

### 目标

用户决定菜单不再使用图片图标，左侧 tab 只显示文字，避免 PNG 解码、Deko 纹理上传、RomFS 图片资源带来的不稳定因素。

### 已实施

- `drawLeftMenu()` 移除 tab 图片/字母图标绘制，只保留菜单文字。
- 删除 `itemIcon()`、`itemIconPath()`、`preloadMenuImages()`。
- 删除 `UiPrimitives` 中的 PNG 图片缓存、`preloadImage()`、`drawImage()`。
- 移除 `stb_image` include 与 `stb_image.c` 链接。
- 移除 NDS Stub 菜单图片资源复制 target。
- CMake 配置阶段清理旧的 `nds_stub_romfs/img/ui/menu` 目录，避免历史构建残留继续进入 NRO。

### 当前状态

- NDS Stub 菜单不再读取、解码、上传任何 tab 图片资源。
- `GBAStationNDSStub.nro` 的 RomFS 不再包含 `img/ui/menu/*.png`。
- 底部 hint 仍保留按键字符提示，这是字体 glyph，不走图片/纹理文件加载链路。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.24 MB
```

---

## 阶段 3.7：修复启动阶段菜单图片预热导致黑屏

### 现象

真机反馈：打开 `GBAStationNDSStub.nro` 后一直黑屏，日志停在：

```text
GBAStationNDSStub: Deko checkpoint preload menu images begin
```

### 判断

阶段 3.6 把菜单 PNG 预加载放在 `Gfx::Init()` 之后，并额外调用了一个启动阶段预热帧：

```cpp
Gfx::StartFrame();
preloadMenuImages();
Gfx::EndFrame(...);
PresentQueue.waitIdle();
```

这个位置早于 NDS/GPU renderer/game layer 的正常运行循环，真机上可能卡在 swapchain acquire/present 相关路径，导致游戏画面还没出现就黑屏。

### 已实施

- 移除 `Gfx::Init()` 后的启动阶段预热帧。
- 启动链只记录：

```text
GBAStationNDSStub: Deko menu images preload deferred to render loop
```

- 新增 `menuImagesPreloadPending`，把菜单图片预加载延迟到正常渲染循环中：
  - 先完成 ROM 加载、音频启动和模拟器主循环；
  - 第 3 个正常渲染帧中，在稳定的 `Gfx::StartFrame()` 之后执行一次 `ui::preloadMenuImages()`；
  - 之后 `drawImage()` 只绘制已加载纹理，失败仍回退到文字占位。

### 当前状态

- 启动阶段不再等待菜单 PNG 预加载，不应再卡在黑屏。
- 菜单图片上传发生在原本稳定的渲染循环内。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.32 MB
```

---

## 阶段 3.6：修复打开菜单时 PNG 图标路径导致崩溃

### 现象

真机反馈：游戏运行稳定，按菜单热键后日志停在：

```text
GBAStationNDSStub: menu hotkey toggle visible=0->1
```

随后程序崩溃。

### 判断

崩溃点与阶段 3.5 新增的 `drawLeftMenu()` PNG 图标绘制高度相关。原实现会在菜单第一次绘制时执行：

- RomFS 文件读取；
- PNG 解码；
- Deko 纹理创建；
- `TextureUpload()` 提交。

这些操作混在菜单第一帧绘制路径里，容易与字体 atlas 上传、Deko staging buffer 当前 swapchain slot 发生时序问题。尤其 `TextureUpload()` 使用当前 `SwapchainSlot` 的 staging buffer，因此不应在没有明确 `Gfx::StartFrame()` 的时机懒加载。

### 已实施

- `drawImage()` 改为只绘制已经预加载的纹理，不再在绘制时触发文件 IO / PNG 解码 / 纹理上传。
- 新增 `preloadImage()` 与 `preloadMenuImages()`。
- 在 `Gfx::Init()` 后增加菜单图片预加载检查点：
  - 先调用 `Gfx::StartFrame()` 获取明确的 swapchain slot；
  - 预加载并上传所有菜单 PNG；
  - 调用 `Gfx::EndFrame()` 完成上传；
  - `PresentQueue.waitIdle()` 等待预热帧完成。
- 增加实时日志：

```text
GBAStationNDSStub: Deko checkpoint preload menu images begin
GBAStationNDSStub: Deko checkpoint preload menu images result=...
```

### 当前状态

- 菜单打开时只绘制已存在的 Deko 纹理，不再动态加载图片。
- 如果某个图标预加载失败，`drawLeftMenu()` 会自动退回原文字占位图标，保证菜单可用。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.32 MB
```

---

## 阶段 3.4：菜单按键图标统一为 Borealis Hint 字符码

### 目标

用户要求 NDS Stub 菜单不再依赖 `GFX_NINTENDOFONT_*` 系列定义，改为使用 Borealis `Hint::getKeyIcon()` 中同一套私有区 Unicode 字符码。

### 已实施

- 在 `nds_stub/include/nds_stub/ui/UiPrimitives.hpp` 中新增 `NDS_STUB_KEYICON_*` 宏：
  - A/B/X/Y、L/R、ZL/ZR、左右摇杆、START/BACK、方向键、UNKNOWN。
- `nds_stub/src/ui/UiComponents.cpp` 中底栏提示与 LR 选择器全部改用 `NDS_STUB_KEYICON_*`。
- LR 选择器左右提示从普通文本 `L` / `R` 改为 Switch 按键图标字符。

### 当前状态

- `nds_stub/include` 与 `nds_stub/src` 下已经没有 `GFX_NINTENDOFONT` 引用。
- 菜单按键图标现在与 Borealis `Hint::getKeyIcon()` 的字符码保持一致。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.24 MB
```

---

## 阶段 4.1：NDS Stub 菜单暂停、配置输入与热键接入

### 目标

- 菜单打开期间真正暂停 NDS 核心，打开时从底部滑出，关闭时滑入底部。
- 左侧 tab 保留“图片 + 文字”接口，当前无图片资源时使用稳定占位图标。
- tab 点击后焦点进入右侧子页面第一个控件。
- Stub 独立读取 `/GBAStation/config/config.cfg` 中的 `nds.*`、`fastforward.*`、`save.*`、`turbo.rate` 配置，补齐 NDS 按键、摇杆方向和常用热键。

### 已实施

- `NdsMenuLayer` 增加：
  - `open()` / `close()` / `toggle()`；
  - `active()` 用于菜单动画期间暂停游戏和拦截残余按键；
  - tab 焦点与子页面焦点分离；
  - 保存/读取状态动作携带 slot；
  - 底部滑入/滑出动画。
- `UiComponents` 增加：
  - `itemIconPath()` 图片路径接口；
  - tab 图标占位框；
  - 子页面焦点渲染；
  - 菜单整体 `offsetY` 绘制参数。
- `NdsDekoRuntime` 增加独立配置解析：
  - 读取 `sdmc:/GBAStation/config/config.cfg` 或 `/GBAStation/config/config.cfg`；
  - 支持 `|` 多绑定、`+` 组合键；
  - 支持实体按键与左右摇杆方向虚拟按键；
  - 支持 NDS 普通键、快进、快速存档、快速读档、截图请求、菜单、静音、暂停、指针模式、指针点击、上下屏交换、A/B 连发。
- 菜单活跃期间不再调用 `NDS::RunFrame()`，只绘制最后一帧与菜单动画。
- 状态保存/读取改为使用 ArcDelta melonDS 的 `Savestate` + `NDS::DoSavestate()`。
- 执行保存/读取/重置时暂停音频线程并等待 Deko 队列空闲。
- `NdsGameLayer` 增加上下屏交换，触摸区域跟随交换后的下屏。

### 当前限制

- 截图热键已接入并实时写日志，但当前 Deko Gfx 层没有暴露 framebuffer readback，暂不生成 PNG。
- tab 图片加载只留接口，等待后续把图片资源放入 NDS Stub romfs 后接入纹理加载。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.22 MB
```

---

## 阶段 4.2：菜单存档列表、删除确认与画面设置控件

### 已实施

- 左侧 tab：
  - `返回游戏`、`重置游戏`、`退出游戏` 不再显示右侧子页面；
  - 当前选中 tab 始终绘制蓝色选中底色；
  - tab 焦点仍使用流光边框。
- 存档页：
  - 保存状态 / 读取状态改为 10 个槽位；
  - 槽位文件路径优先使用 GameDB `savePath`；
  - 文件命名为 `{romStem}.ss0` 到 `{romStem}.ss9`；
  - 缩略图路径为 `{romStem}.ss0.png` 到 `{romStem}.ss9.png`；
  - 每个槽位显示缩略图区域、槽位名称、存档文件修改时间；
  - 保存后刷新槽位列表；
  - 读取成功后自动关闭菜单返回游戏；
  - 聚焦已有槽位时底栏显示 `X 删除`；
  - X 打开删除确认遮罩，A 确认删除状态文件和缩略图，B 取消。
- 画面设置页：
  - 增加 LR 选择器、开关选择器、子页面按钮、普通按钮四类绘制样式；
  - 快进倍率支持 `0.1 / 0.5 / 1 / 1.25 / 1.5 / 1.75 / 2 / 3 / 4 / 5`；
  - 画面过滤支持 `Nearest / Linear`；
  - 整数倍缩放、画面布局、自定义画面布局、画面方向、遮罩选择、滤镜选择、同步项已做 UI 占位；
  - 自定义画面布局仅在画面布局为 `自定义` 时可聚焦；
  - L/R 长按会加速切换。

### 当前限制

- ArcDelta 当前 `Gfx` 层没有稳定的 framebuffer readback / PNG decode 接口，本阶段只完成 `{romStem}.ssN.png` 文件链路并写入占位 PNG；菜单左侧缩略图区域显示占位状态，尚未显示真实游戏截图。
- `GFX_NINTENDOFONT` 当前没有 L/R button 宏，本阶段使用 NintendoExt 字体中的普通 `L` / `R` 字符作为兼容占位。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.24 MB
```

---

## 阶段 3.6：移除多倍分辨率并恢复 x1-only

### 目标

用户确认暂时放弃 NDS 多倍分辨率功能，只保留默认 1 倍分辨率，优先恢复稳定启动和稳定运行。

### 已实施

- 移除 NDS Stub 菜单中的“内部分辨率”项目。
- 移除 `NdsDekoRuntime` 中的分辨率切换逻辑，`GPU::RenderSettings` 固定为 `GL_ScaleFactor = 1`。
- 移除 `GPU2D_Deko` 的 `SetScaleFactor`、多倍率 compose shader 加载、scale 下标访问和高倍 3D 采样参数。
- 移除 `GPU3D_Deko` 的倍率状态、动态工作缓冲 sizing、`*_xN` shader 加载和高倍 dispatch 路径。
- 恢复 compose 和 GPU3D shader 为固定 256x192 原生路径。
- 在 `nds_stub/CMakeLists.txt` 中移除 `_x1/_x2/_x3/_x4` shader 生成目标，并在配置阶段清理旧构建目录中的 `_x1.._x4` 残留 shader，避免被 RomFS 继续打包。
- 保留实时日志、菜单、音频、触摸、快进、过滤方式和之前的标题界面红屏修复。

### 验证

- `git diff --check` 通过。
- 关键残留搜索无命中：`BGIs3DMask`、`Source3DScale`、`SetScaleFactor`、`LoadShadersForScale`、`ConfigureScale`、`*_x2/_x3/_x4`。
- `build_switch/nds_stub_romfs/shaders` 中无 `_x*.dksh` 残留。
- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 2.17 MB
```

### 当前状态

NDS Stub 只走原生 1 倍 Deko 渲染路径。后续性能优化应基于 x1 稳定性继续推进，不再引入多倍分辨率相关资源或运行时切换。

---

## 阶段 3.4：重新实装 Deko x2 内部分辨率并强化实时日志

### 目标

只推进 x2，不继续扩展 x3/x4。要求每一步都留下可定位日志，避免真机如果再次黑屏或卡死时只剩现象，无法判断卡在资源分配、shader 加载、3D compute、2D compose 还是 display capture。

### 已实施

- Stub 日志系统改为实时落盘：
  - 每条日志带 `armGetSystemTick()` tick；
  - 每次写入后执行 `fflush + fsync + fclose`；
  - 启动时尝试创建 `sdmc:/GBAStation/log`；
  - 暴露 `extern "C" GBAStationNDSStubLogLine()`，供 ArcDelta Deko 内部直接写入同一份日志。
- NDS stub 默认以 x2 启动：
  - 启动时写入 `Deko resolution initial x2 forceX1=0`；
  - 如需临时救援，可放置 `sdmc:/GBAStation/config/nds_stub_x1.flag` 强制 x1；
  - 菜单内部分辨率限制为 x1/x2，当前不开放 x3/x4。
- `GPU2D_Deko` 开放 `MaxScaleFactor = 2`：
  - Final framebuffer 与 3D framebuffer 按 512x384 分配；
  - 记录 framebuffer layout size、3D framebuffer size、compose shader scale 加载、scale 应用结果；
  - `ComposeBGOBJ()` 添加前 24 次和之后每 240 次的关键日志。
- `GPU3D_Deko` 开放 `MaxScaleFactor = 2`：
  - 初始化按 x2 最大资源分配，再运行时切回请求倍率；
  - 预加载 x1/x2 compute shader，避免运行中首次切换加载；
  - 记录 configure、各类 buffer 分配大小、shader 加载、SetRenderSettings。
- `ComposeBGOBJ_fsh.glsl` 恢复 scale-aware 3D BG 采样：
  - 2D BG/OBJ 仍按原生 256x192 采样；
  - 只有 BG0 作为 3D 层时使用 `Source3DScale` 采样 512x384 3D framebuffer。
- `GPU2D_Deko::ComposeBGOBJ()` 对 display capture 做拆分：
  - capture pass 使用 native viewport/scissor 写 256x192 `BGOBJTexture`；
  - display pass 再写 x2 final framebuffer；
  - palette/uniform 上传只保留一次，降低重复 pass 对资源状态的扰动。
- `GPU3D_Comp.glsl` 修正 x2 final pass 的横向滚动和边缘判断：
  - `XScroll` 按 `ScreenWidth / 256` 缩放；
  - `srcX`、右边界、下边界不再写死 256/255/191。

### 新增关键日志

```text
GBAStationNDSStub: Deko resolution initial x2 forceX1=0
GBAStationNDSStub: Deko checkpoint GPU2D::SetScaleFactor begin scale=2
GBAStationNDSStub: GPU2D_Deko ctor begin maxScale=2 maxFb=512x384
GBAStationNDSStub: GPU2D_Deko compose begin seq=... unit=... scale=2 capture=...
GBAStationNDSStub: GPU3D_Deko init begin
GBAStationNDSStub: GPU3D_Deko configure scale=x2 screen=512x384 ...
GBAStationNDSStub: GPU3D_Deko alloc tileMemory=...
GBAStationNDSStub: GPU3D_Deko shaders loaded scale=x2
```

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 3.17 MB
```

### 真机验证重点

- 首次打开 NDS 是否能直接进入游戏画面；
- 宝可梦黑2、黄金太阳标题界面是否仍有整屏偏红；
- x2 下复杂 3D 场景 GPU 占用是否上升，画面是否比 x1 更清晰；
- 若黑屏或卡死，直接查看 `sdmc:/GBAStation/log/GBAStationNDSStub.log` 最后一条日志。

---

## 阶段 3.5：根据真机日志降低 x2 GPU3D TileMemory 压力

### 真机日志

本次日志停在：

```text
GPU3D_Deko configure scale=x2 screen=512x384 tiles=64x48 maxWork=147456 ...
GPU3D_Deko alloc ySpanTexture layoutSize=2097152 align=8
```

下一步应进入 `TileMemory` 分配，但没有出现 `alloc tileMemory` 日志。因此高概率卡在 x2 TileMemory 分配处。

### 判断

上一版 x2 沿用了 x1 的 `WorkTileMultiplier=48`。x2 下 tile 数变为 64x48，导致：

```text
RuntimeMaxWorkTiles = 64 * 48 * 48 = 147456
TileMemory ~= 147456 * 8 * 8 * 3 * 4 = 108 MB
```

这个分配量对 Switch DataHeap 压力过高，容易在初始化阶段直接卡死或崩溃。

### 已实施

- `GPU3D_Deko::WorkTileMultiplierForScale()` 调整：
  - x1 保持 48；
  - x2 改为 16；
  - x3/x4 暂按 8，但当前 runtime 仍只开放 x1/x2。
- `nds_stub/CMakeLists.txt` 同步 shader 常量：
  - x1 `NDS_DEKO_WORK_TILE_MULTIPLIER=48`；
  - x2 `NDS_DEKO_WORK_TILE_MULTIPLIER=16`；
  - x3/x4 `NDS_DEKO_WORK_TILE_MULTIPLIER=8`。
- GPU3D 初始化日志改为分配前后都记录：

```text
GPU3D_Deko alloc tileMemory begin bytes=... maxWork=... multiplier=...
GPU3D_Deko alloc tileMemory ok bytes=...
GPU3D_Deko alloc binResult begin bytes=...
GPU3D_Deko alloc binResult ok bytes=...
GPU3D_Deko alloc finalTile begin bytes=...
GPU3D_Deko alloc finalTile ok bytes=...
```

### 预期变化

x2 的 TileMemory 从约 108MB 降到约 36MB：

```text
RuntimeMaxWorkTiles = 64 * 48 * 16 = 49152
TileMemory ~= 49152 * 8 * 8 * 3 * 4 = 36 MB
```

这不是回退 x2，而是把 x2 的 GPU work buffer 预算从“原生比例直接放大”改为 Switch 更可能稳定承载的规模。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 3.17 MB
```

---

## 阶段 3.6：回收 x2 实验路径，恢复 x1 稳定包

### 现象

真机反馈阶段 3.5 的 x2 首轮实现会导致：

- 核心 1 占用约 80% 后程序崩溃；
- 或核心 1 长时间 100%，黑屏卡住。

### 判断

x2 路径仍存在启动期资源/同步问题。风险点包括：

- `GPU2D_Deko / GPU3D_Deko` 的最大 scale 提升后，启动初始化会重新触发更大的 framebuffer / compute buffer / shader 资源路径；
- `ComposeBGOBJ()` 的 native capture pass + scaled display pass 会让同一批 compose region 里的 palette copy 和 render target 绑定被重复执行，可能踩到 Deko 命令同步或资源状态问题；
- 当前阶段继续保留 x2 会破坏 x1 稳定性，因此先回收实验入口。

### 已实施

- `GPU2D_Deko::MaxScaleFactor` 恢复为 `1`。
- `GPU3D_Deko::MaxScaleFactor` 恢复为 `1`。
- 移除 x2 实验新增的 `CaptureColorTexture`。
- `ComposeBGOBJ_fsh.glsl` 移除 `BGIs3DMask / Source3DScale`，恢复稳定 x1 shader uniform 布局。
- `GPU2D_Deko::ComposeBGOBJ()` 恢复为单 pass 合成。
- NDS stub 菜单中的内部分辨率临时锁定为 `x1`。
- `NdsDekoRuntime` 中分辨率切换请求会记录日志并保持 x1。

### 保留内容

没有回退已验证有效的红屏修复：

- `BGOBJTexture` 继续使用 `intermedFbLayout` / `R32_Uint`；
- `DoCapture()` 中 `srcBaddr` 初始化修复保留；
- `DoCapture()` 中 source A 分支 G/B 通道使用错误修复保留。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 3.16 MB
```

---

## 阶段 3.5：开放 Deko x2 多倍分辨率首轮验证

### 目标

在不破坏 x1 稳定性和 display capture 颜色正确性的前提下，让 NDS stub 的多倍分辨率开始真正生效。首轮只开放 x1 / x2，x3 / x4 暂缓，避免再次触发高资源初始化导致的黑屏长卡。

### 已实施

- `GPU2D_Deko::MaxScaleFactor` 从 `1` 提升到 `2`。
- `GPU3D_Deko::MaxScaleFactor` 从 `1` 提升到 `2`。
- NDS stub 菜单中的“内部分辨率”暂时限制为 `x1 / x2`。
- `NdsDekoRuntime` 中实际应用分辨率时最大限制为 `x2`，日志会记录：

```text
GBAStationNDSStub: Deko resolution scale request xN apply xM
```

- `ComposeBGOBJ_fsh.glsl` 重新加入 `BGIs3DMask`，但这次同时加入 `Source3DScale`，只对 3D BG 层使用高分辨率采样坐标，普通 2D BG/OBJ 仍使用 NDS 原生坐标。
- `GPU2D_Deko::ComposeBGOBJ()` 拆分为两类 pass：
  - display pass：按当前 scale 输出到最终屏幕帧缓冲；
  - capture pass：如果 NDS display capture 生效，额外用 256x192 native pass 写入 `BGOBJTexture`，保证 capture 写回 VRAM 的格式和尺寸不被 x2 污染。
- 新增 `CaptureColorTexture` 作为 capture pass 的原生颜色输出目标，避免 x2 最终帧缓冲与 native capture target 混合作为双 render target。

### 当前限制

- x3 / x4 仍未开放。
- x2 下 display capture 帧会多一次 native compose pass，优先保证兼容性；性能数据需要真机继续观察。
- 主程序、libretro、mGBA 渲染与音频路径未修改。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 3.16 MB
```

---

## 阶段 3.4：恢复 DisplayCapture / BGOBJ 中间纹理格式

### 现象

阶段 3.3 修正 shader uniform 后，真机反馈宝可梦黑2、黄金太阳等标题/开始界面仍然整屏偏红，但游戏内普通场景基本正常。

### 判断

异常仍集中在标题界面，说明更可能是 `DirectBitmap / DisplayCapture` 使用的中间纹理格式错误。高分辨率改造期间 `BGOBJTexture` 被改成了最终输出用的 `RGBA8_Unorm` 布局，但 capture 路径会把该纹理作为 `R32_Uint` 风格的 BG/OBJ 合成结果复制回内存，再交给 NDS capture 逻辑处理。格式不一致会导致 direct bitmap / capture 场景颜色串位。

### 已实施

- `GPU2D_Deko.cpp` 中 `BGOBJTexture` 分配和初始化恢复使用 `intermedFbLayout`：

```cpp
BGOBJTextureMemory = Gfx::TextureHeap->Alloc(intermedFbLayout.getSize(), intermedFbLayout.getAlignment());
BGOBJTexture.initialize(intermedFbLayout, Gfx::TextureHeap->MemBlock, BGOBJTextureMemory.Offset);
```

### 当前状态

- high-res runtime 仍保持 x1 安全锁定。
- 本次只恢复 capture 中间纹理格式，不影响主程序、libretro、mGBA 渲染路径。

### 追加修复

- `DoCapture()` 中 `srcBaddr` 改为显式初始化为 `0`，避免 source B 地址在部分 capture 模式下读取未定义值。
- 修复 source A 单独参与 capture 混合时的 NEON 通道错误：

```cpp
uint8x16_t finalG = vshrn_high_n_u16(vshrn_n_u16(finalGLo, 4), finalGHi, 4);
uint8x16_t finalB = vshrn_high_n_u16(vshrn_n_u16(finalBLo, 4), finalBHi, 4);
```

此前 G/B 使用了 R 的低半累加器，可能导致标题界面、开场画面等大量使用 display capture 的场景整体偏红。

### 下一步

- 真机验证 x2/x3/x4：
  - 是否能看到 3D 场景清晰度变化；
  - 菜单切换后是否黑屏/闪退；
  - x3/x4 在复杂场景是否出现 work overflow 或异常画面。
- 根据真机日志继续补 runtime overflow fallback。

---

## 阶段 3.1：优化打开 NDS 游戏时单核 100% 的启动卡顿

### 现象

真机打开 NDS 游戏时，核心 1 会长时间 100%，核心 0 和核心 2 基本为 0%。这说明当前瓶颈集中在单线程初始化路径，而不是多线程运行时负载。

### 原因分析

阶段 3 为多倍分辨率接通了 x1/x2/x3/x4 shader set，但初版在启动时一次性加载了：

- GPU3D compute shader 的 x1-x4 全套变体；
- GPU2D compose shader 的 x1-x4 变体。

这些 shader 加载和 deko shader 初始化都发生在主启动线程上，因此会表现为单核长时间满载。

### 已实施

- `GPU3D_Deko` 改为按需加载倍率 shader：
  - 启动只加载 x1；
  - 首次切换到 x2/x3/x4 时再加载对应倍率；
  - 加载后缓存，后续切换复用。
- `GPU2D_Deko` 的 compose shader 同样改为按需加载：
  - 启动只加载 x1；
  - 首次切换倍率时加载对应 xN compose shader。
- 增加启动/切换日志：

```text
DekoRenderer loaded GPU3D shader scale xN
GPU2D::DekoRenderer loaded compose shader scale xN
```

### 预期效果

- 打开 NDS 游戏时，核心 1 长时间 100% 的阶段应明显缩短。
- 第一次从菜单切到 x2/x3/x4 时可能出现一次短暂停顿，这是懒加载的成本转移；第二次切换同倍率不应再卡。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 3.16 MB
```

---

## 阶段 3.2：启动黑屏单核 100% 的安全回退与诊断

### 现象

真机反馈：打开 NDS 游戏后仍长时间黑屏，核心 1 持续 100%，等待几分钟仍不进入画面。

### 判断

这已经不是 shader 懒加载能够解释的短卡顿，更像启动路径中某个初始化调用卡住。高风险点是阶段 3 中引入的 high-res runtime 预分配：

- GPU3D 工作缓冲曾按 x2/x4 规模预分配；
- GPU2D final / 3D framebuffer 曾按 x4 尺寸预分配；
- 这些资源在 Switch 上可能导致 deko heap 分配、映射或 GPU 同步卡住。

### 已实施

- 临时把 `GPU3D_Deko::MaxScaleFactor` 锁回 `1`。
- 临时把 `GPU2D_Deko::MaxScaleFactor` 锁回 `1`。
- 菜单中 x2/x3/x4 的请求会记录日志并保持 x1：

```text
GBAStationNDSStub: Deko resolution scale xN temporarily disabled; runtime stays x1
```

- 在启动路径加入毫秒级耗时日志：

```text
config done ms=...
Gfx::Init ok ms=...
NDS::Init ok ms=...
GPU::InitRenderer ok ms=...
GPU::SetRenderSettings ok ms=...
gameLayer.init ok ms=...
LoadROM loaded=... ms=...
audio.start result=... ms=...
```

### 当前目的

先恢复 x1 启动稳定性，避免 high-res 资源参与启动。下一份真机日志应能直接定位黑屏卡在：

- `Gfx::Init`
- `NDS::Init`
- `GPU::InitRenderer`
- `GPU::SetRenderSettings`
- `LoadROM`
- audio start
- 第一帧 RunFrame / StartFrame / EndFrame

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 3.16 MB
```

---

## 阶段 3.3：修复部分标题界面整屏偏红

### 现象

真机反馈：x1 安全版可以启动，但部分画面整屏变红，例如：

- 宝可梦黑2开始界面；
- 黄金太阳开始界面；
- 进入游戏后普通场景基本正常。

### 判断

异常集中在标题界面，而普通游戏场景正常，说明问题更可能在 `ShowBitmap / DirectBitmap / DisplayCapture` 相关路径，而不是普通 BG/OBJ 合成。阶段 3 为高分辨率合成在 `ComposeUniform` 中插入了 `BGIs3DMask[4]`，会改变 shader 与 C++ 侧 uniform buffer 的布局；即使当前运行时锁回 x1，也可能让部分 compose shader 读取错字段，导致颜色异常。

### 已实施

- `ComposeBGOBJ_fsh.glsl` 移除 `BGIs3DMask0..3`。
- BG0..3 的采样恢复为原始 `position` 坐标。
- `GPU2D_Deko::ComposeUniform` 移除 `BGIs3DMask[4]`。
- `ComposeBGOBJ()` 移除 `BGIs3DMask` 赋值逻辑。

### 当前状态

- high-res runtime 仍保持 x1 安全锁定。
- 2D compose uniform 布局恢复到原始结构，优先保证标题界面、DirectBitmap、DisplayCapture 路径颜色正确。

### 构建记录

- Switch 构建通过：

```text
GBAStation.nro        24.92 MB
GBAStationNDSStub.nro 3.16 MB
```
