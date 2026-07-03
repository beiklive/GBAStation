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
