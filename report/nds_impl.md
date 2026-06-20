# NDS melonDS 原生核心集成实施记录

## 1. 修改文件列表

- `CMakeLists.txt`
- `switchbuild.sh`
- `third_party/CMakeLists.txt`
- `third_party/melonDS/src/ARMJIT_A64/ARMJIT_Compiler.cpp`
- `third_party/melonDS/src/ARMJIT.cpp`
- `third_party/melonDS/src/ARMJIT_Global.cpp`
- `third_party/melonDS/src/ARMJIT_Memory.h`
- `third_party/melonDS/src/ARMJIT_Memory.cpp`
- `third_party/melonDS/src/CP15.cpp`
- `third_party/melonDS/src/FATStorage.cpp`
- `third_party/melonDS/src/GPU.cpp`
- `third_party/melonDS/src/GPU3D.cpp`
- `third_party/melonDS/src/GPU3D_Soft.cpp`
- `third_party/melonDS/src/GPU3D_Soft.h`
- `third_party/melonDS/src/NDS.cpp`
- `third_party/melonDS/src/NDS.h`
- `src/core/enums.h`
- `src/core/constexpr.h`
- `src/core/common.cpp`
- `src/core/Tools.cpp`
- `src/core/game_database.cpp`
- `src/emulator/EmulatorCoreFactory.cpp`
- `src/game/audio/AudioManager.cpp`
- `src/network/ApiRouter.cpp`
- `src/ui/page/DataManagementPage.cpp`
- `src/ui/page/FileListPage.cpp`
- `src/ui/page/GameLibraryPage.cpp`
- `src/ui/page/GameLibraryPage.hpp`
- `src/ui/page/StartPage.cpp`
- `src/ui/view/GameView.cpp`
- `src/ui/view/GameView.hpp`
- `src/ui/view/GameMenuView.cpp`
- `src/ui/view/GameMenuView.hpp`
- `src/ui/view/RecyclingGridItem.hpp`
- `src/ui/widget/GridItem.cpp`
- `resources/web/app.js`
- `resources/web/index.html`
- `resources/web/style.css`

## 2. 新增文件列表

- `src/emulator/melonds/MelonDSCore.h`
- `src/emulator/melonds/MelonDSCore.cpp`
- `src/emulator/melonds/MelonDSPlatform.h`
- `src/emulator/melonds/MelonDSPlatform.cpp`
- `src/emulator/melonds/MelonDSAudio.h`
- `src/emulator/melonds/MelonDSAudio.cpp`
- `src/emulator/melonds/MelonDSInput.h`
- `src/emulator/melonds/MelonDSInput.cpp`
- `src/emulator/melonds/MelonDSVideo.h`
- `src/emulator/melonds/MelonDSVideo.cpp`
- `src/emulator/IEmulatorTouchInput.hpp`
- `src/emulator/IEmulatorStopRequest.hpp`

## 3. CMake 改动

- 新增 `melonds_core` 静态库，手工列出 `third_party/melonDS/src` 核心源码。
- 未使用 `add_subdirectory(third_party/melonDS)`，避免引入 melonDS frontend/Qt/SDL/libui。
- 保留核心模块：ARM、ARMJIT、GPU、NDS、SPU、DSi、Savestate、Platform API 依赖、fatfs、DSP_HLE、teakra 等。
- 排除 melonDS `frontend/`、Qt、SDL、libui、OpenGL renderer、net/pcap 前端依赖。
- Switch 构建下加入 ARM64 JIT 源码：`ARMJIT_A64/*`、`ARMJIT_Linkage.S`、`dolphin/Arm64Emitter.cpp`。
- Switch 构建下定义 `ENABLE_JIT=1`、`JIT_ENABLED`、`ARCHITECTURE_ARM64=1`，并设置 `-O3 -mcpu=cortex-a57 -ffast-math -flto -fomit-frame-pointer`。
- Switch JIT 平台适配补充了 libnx exception handler、A64 JIT 内存区声明、Switch fastmem 页大小初始化，避免依赖 POSIX `mmap/sysconf` 路径。
- `MelonDSCore` 仅在 Switch 构建中请求并强制验证 ARM64 JIT；Windows 桌面构建显式关闭 JIT，使用解释器运行，避免桌面版被 ARM64 JIT 验收逻辑误拒绝。
- Switch NDS 启动时，`GameView` 将 melonDS `SetupGame()` 延后到游戏线程执行，避免 BIOS/ROM/JIT 初始化阻塞 borealis UI 线程。
- Switch NDS 游戏线程绑定到 core 1，并在帧率限制尾段使用 `svcSleepThread()` 让出 CPU，避免每帧 `yield()` 忙等导致 core 0 满载、UI 卡死。
- `MelonDSCore` 的 ready/paused 状态改为 atomic，支持 Switch NDS 异步初始化期间 UI 线程安全轮询 `IsReady()`。
- 新增 `IEmulatorStopRequest` 扩展接口，`GameView::_stopGameThread()` 在 join 前请求 NDS 核心 `Halt()`，避免 melonDS 卡在内部 `RunFrame()` 时 UI 退出等待线程导致主界面卡死，同时避免 UI 线程直接停止 GPU/SPU。
- `MelonDSCore` 在 `LoadGame()`/`Reset()` 的 direct boot 后按 `melonds-switch` 参考实现显式调用 `GPU.StartFrame()`，用于启动 LCD scheduler 和首帧输出。
- melonDS `NDS::Running` 改为 atomic，保证 UI 线程请求停止后，游戏线程内 `RunFrame()` 的停止条件在 Switch 多核上可见。
- Threaded Renderer 改为通过 `SoftRenderer(true)` 构造启用，避免在 `NDS` 构造后、ROM reset 前手动 `SetThreaded(true)` 提前唤醒渲染线程，降低 `NDS::Reset()` 与渲染线程初始化互等风险。
- `MelonDSCore::LoadGame()` 增加 `SetNDSCart/Reset/SetupDirectBoot/GPU.StartFrame/Start` 阶段日志，用于定位 Switch 真机启动卡住的精确阶段。
- Switch 构建下仍启用 ARM64 JIT，但显式关闭 `JITArgs::FastMemory`，绕开当前最可疑的 Switch fastmem 映射/fault handler 路径；运行日志新增 `melonDS: JIT FastMemory disabled on Switch`。
- `NDS::Reset()`、`ARMJIT::Reset()`、`CP15Reset()`、`GPU::Reset()`、`GPU3D::Reset()`、`SoftRenderer::Reset()/SetupRenderThread()` 增加阶段日志，用于确认 `.nds` 真机启动卡在 JIT reset、CP15/PU 时序更新、GPU reset 还是 threaded renderer 启动。
- 真机日志已确认 `JIT.Reset` 返回，卡点进入 `CP15Reset()` 后、`UpdatePURegions()` 前；`NDS::Reset()` 入口现在设置 `NDS::Current = this`，并为 Switch libnx JIT fault handler 增加 `NDS::Current` 空指针保护，避免 reset 期间异常处理递归卡死。
- `CP15Reset()` 增加 ITCM/DTCM/ICache 单步日志，用于确认下一次真机复测是否停在 DTCM 指针写入或 ICache 清理。
- `NDS::Current` 已从 upstream 的 `thread_local` 改为普通静态指针，匹配 `third_party/melonds-switch` 的 Switch 兼容做法；`NDS` 构造完成和 `Reset()` 入口都会刷新 `Current` 并输出定位日志，用于排除 libnx TLS/JIT exception handler 路径导致的卡死。
- 根据 `GBAStation (3).log`，崩溃点定位为 `CP15Reset()` 清理 DTCM 时写入 `NDS.JIT.Memory.GetARM9DTCM()` 指针触发 JIT fault；Switch 下 `ARMJIT_Memory` 现在保留 `MemoryBaseBacking` 用于 `svcMapProcessCodeMemory`/释放，实际 `MemoryBase` 指向已设置 `Perm_Rw` 的 `MemoryBaseCodeMem` 视图，并避免通用末尾逻辑覆盖 `FastMem9Start/FastMem7Start`。
- `switchbuild.sh` 默认使用 `/opt/devkitpro`，并将 `TMPDIR/TMP/TEMP` 固定到 `build_switch/tmp`，其中 `TMP/TEMP` 使用 Windows 路径供 devkitA64 编译器写入临时文件。
- mGBA 第三方构建关闭 `BUILD_LTO`，避免旧 LTO bytecode 与当前 GCC 16 工具链不兼容。
- 主程序链接 `melonds_core`，并加入 melonDS include 路径。
- 修复 melonDS SPU 输出读取单位：`SPU::ReadOutput()` 的返回值是立体声帧数，推入 `MelonDSAudio` 时转换为 `int16_t` 样本数，避免左右声道和音频节奏被半包写入打碎。
- Switch `AudioManager` 的 audout 后端改为按 `AudioOutBuffer*` 精确追踪自身缓冲区，只有硬件释放且属于本实例的缓冲才进入 free list；复用前重置 `next` 并执行 `armDCacheFlush()`，避免共享 audout 队列中混入 UI 音效完成事件后重写仍在 DMA 播放的缓冲。
- NDS 视频输出在 `MelonDSVideo::Capture()` 内转换为现有 renderer 期望的 RGBA8888，修复红蓝通道互换导致红色显示为蓝色的问题。
- NDS 显示增加 `nds.screenLayout` 布局设置，支持上下屏、左右屏、仅上屏、仅下屏；`GameView` 在上传前按布局重排双屏帧，同时根据当前下屏矩形映射触摸坐标到 NDS 256x192。
- 远程 Web 管理器导入列表加入 `.nds`，平台页签加入 NDS，并补充 NDS 平台样式；后端 `ApiRouter` 已支持 `.nds` 平台识别。
- NDS 快进改为每次游戏循环只运行一帧，并按快进倍率缩短帧间隔，避免一次批量运行多帧后只上传最后一帧造成明显跳帧；非 NDS 核心保持原有批量快进行为。
- NDS 屏幕布局新增 `separate`（分离双屏）模式，在现有单纹理渲染框架内将上下屏组合到带间距的横向画布，下屏触摸矩形随布局同步更新。
- NDS 触摸输入增加原始输入轮询路径：Switch 直接读取 Borealis platform touch state，Windows 可用鼠标左键调试；该路径绕过 `Application::blockInputs()` 对手势识别的影响，仅在 `GameView` 当前获得焦点时向核心投递触摸。
- NDS `separate` 布局改名为 `custom`（旧配置自动兼容并写回），GameDB 新增 `ndsTopScale/OffsetX/OffsetY` 与 `ndsBottomScale/OffsetX/OffsetY` 字段。
- 游戏菜单 NDS 屏幕布局下新增“上屏调整”“下屏调整”；仅在布局为自定义时可用，打开后复用自定义设置样式的侧边栏控制单屏 X/Y 偏移和缩放。
- 自定义双屏布局改为 576x324 组合画布，上下屏可独立缩放和移动；下屏触摸区域根据下屏最终矩形同步映射。
- NDS 自定义布局调整时缓存核心输出的原始双屏帧，菜单内修改布局/单屏 X/Y/scale 会立即重新排版并标记待上传，背景层游戏画面无需关闭菜单即可同步变化。
- NDS 同步画面设置现在会同步上下屏独立缩放和偏移字段；自定义布局组合画布改为 1280x720 直出，单屏缩放以 NDS 原始 256x192 为 1x 基准，避免外层二次缩放影响缩放/偏移语义。
- NDS 自定义布局单屏缩放目标矩形强制保持 4:3；调整上/下屏 X/Y/scale 时取消逐步写盘，改为关闭单屏调整面板时一次保存，提升菜单响应速度。
- NDS 屏幕布局新增 Hybrid（混合）模式：输出 1280x720 固定画布，左侧上屏主屏按 10/3 倍放大并以整数目标矩形 `(0,40,853,640)` 绘制；右侧副屏上/下屏按 5/3 倍放大并以 `(853,40,427,320)`、`(853,360,427,320)` 绘制，同步下屏触摸区域。
- Switch 正常音频缓冲水位提高到环形缓冲 3/4，并将音频线程等待数据窗口拉长到约一个硬件周期，减少轻微 underrun 撕裂。
- NDS 快进改为折中批处理：低倍率保持逐帧平滑，高倍率每轮最多批量运行 2-4 帧并按 `framesRan / multiplier` 节流，以换取更高快进倍率。

## 4. 未完成项

- 当前真机反馈：`.nds` 已能启动运行，ARM64 JIT 和 Threaded Renderer 日志正常；本轮继续针对“正常音频轻微撕裂”“更高快进倍率”“NDS 自定义双屏布局与上下屏独立调整”补齐适配，仍需真机复测确认。
- 已实现每块屏幕独立 X/Y/scale 的菜单项；现有渲染框架仍是单纹理上传，自定义布局会在上传前进行 CPU 最近邻组合缩放，需真机验证高倍率缩放时的性能影响。
- 未完成性能验证，尚未正式记录《宝可梦 白2》或《马里奥赛车DS》的 FPS、CPU 占用、JIT 状态；用户当前反馈的运行占用为 core0 15%、core1 67%、core2 0.25%、core3 28%。
- 项目现有 Switch 音频后端为 audout。为保持现有 UI/渲染/输入/音频框架不变，本次 melonDS 核心仅实现异步 `RingBuffer<int16_t>` 并走现有 `DrainAudio()` 流程，未替换全局 AudioManager 为 audren。

## 5. 风险项

- `MelonDSCore::Initialize()` 会强制验证 `NDS::IsJITEnabled()`；若 Switch JIT 未编译成功，会记录警告并拒绝静默回退。
- Switch ARM64 JIT 当前显式关闭 FastMemory 以优先验证启动/退出稳定性；这仍然是 JIT 执行路径，但性能可能低于 fastmem 开启状态。若真机复测确认稳定，后续再单独修复 fastmem 映射/fault handler。
- 视频仍需通过现有 `IEmulatorCore::GetVideoFrame()` 返回 `LibretroLoader::VideoFrame`，因此为了兼容现有渲染框架保留了一次稳定帧缓冲拷贝；已使用双缓冲降低锁等待，但不是完全的 framebuffer 直连纹理更新。
- BIOS 路径严格使用 `sdmc:/GBAStation/bios/nds/` 下的 `bios7.bin`、`bios9.bin`、`firmware.bin`，缺失会直接失败，不使用 FreeBIOS 静默替代。
- 触摸输入通过新增 `IEmulatorTouchInput` 扩展接口从 `GameView` 转发到 NDS 核心；Tap/Pan 坐标会按当前游戏画面矩形映射到 NDS 下屏 256x192。
- melonDS 第三方源码补了多处兼容修正：Switch JIT fault handler 调用、A64 JIT 内存区声明、Switch fastmem 页大小、C++20 `path::u8string()` 转换；后续更新 melonDS 时需重新核对。
- Switch 链接阶段仍有 libnx `__nx_exception_stack` LTO size discrepancy 警告，但 `GBAStation.elf` 和 `GBAStation.nro` 已成功生成；真机异常处理路径需重点验证。
- Switch NDS 初始化现在异步进行，UI 不会因初始化阻塞，但若初始化失败只会记录日志；后续可加一个 UI 侧失败提示状态。
- Switch audout 仍由 `BKAudioPlayer` 和 `AudioManager` 共享同一输出流；本轮已过滤外来完成事件并在游戏音频运行时跳过 UI 音效播放，但长期看更稳的方案仍是统一后端所有权或迁移到 audren。
- NDS 原始触摸轮询会直接读取 Borealis platform input manager；当前已限制在 GameView 获得焦点时生效，菜单打开时会主动释放触摸，仍需真机确认与系统触屏坐标缩放一致。
- NDS 快进现在在高倍率下允许小批量运行以提高速度，实际最高倍率仍受核心运行性能、Threaded Renderer 和 Switch CPU 余量限制；若用户更偏好极限速度，后续可提供“平滑快进/极速快进”选项。

## 6. 后续优化建议

- 若决定满足 audren 硬性要求，需要在保持 AudioManager 公共接口不变的前提下增加 Switch audren 后端，再让所有核心复用该后端。
- 真机验证触摸边界、缩放模式和自定义偏移下的 NDS 下屏坐标映射。
- 若自定义布局 CPU 缩放成本偏高，可后续把每屏独立矩形下沉到渲染端双 draw call，但这会扩大现有 renderer 改动面。
- 为 NDS 增加专用图标、封面默认图和遮罩资源，当前为避免资源扩张暂用现有 GBA 占位资源。
- 真机验证时输出并记录：FPS、CPU 占用、`ARM64 JIT enabled` 日志、Threaded Renderer 状态、BIOS/存档路径。

## 验证记录

- `cmake -S . -B build_melonds_check3 -G Ninja -DPLATFORM_DESKTOP=ON "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"`：通过。
- `cmake --build build_melonds_check3 --target GBAStation -j 4`：通过，生成桌面 `GBAStation.exe`；触摸扩展补丁后复验仍通过。
- `cmd /c windowsbuild.bat`：通过，生成 `build_windows/GBAStation.exe`，大小 23,471,752 bytes。
- `E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'`：通过，生成 `build_switch/GBAStation.nro`，大小 22,509,035 bytes（脚本输出 21.47 MB）。
- 修复 Windows 桌面运行 `.nds` 时误触发 `ARM64 JIT requested but not enabled` 的初始化失败后复验：`cmd /c windowsbuild.bat` 通过，`switchbuild.sh` 通过。
- 修复 Switch NDS 启动卡死/核心 0 满载风险后复验：`cmd /c windowsbuild.bat` 通过，`switchbuild.sh` 通过，生成 `build_switch/GBAStation.nro`。
- 修复 Switch NDS 退出后 UI 卡死风险、补充首帧 `GPU.StartFrame()` 调度后复验：`cmd /c windowsbuild.bat` 通过，生成 `build_windows/GBAStation.exe`，大小 23,475,919 bytes。
- 同次复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，生成 `build_switch/GBAStation.nro`，大小 22,513,131 bytes（脚本输出 21.47 MB）。
- 调整 Threaded Renderer 启动时机并加入 LoadGame 阶段日志后复验：`cmd /c windowsbuild.bat` 通过。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，生成 `build_switch/GBAStation.nro`（脚本输出 21.47 MB）。
- 为定位 `NDS::Reset()` 卡死补充 JIT/CP15/GPU/threaded renderer 阶段日志后复验：`cmd /c windowsbuild.bat` 通过。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，生成 `build_switch/GBAStation.nro`（脚本输出 21.47 MB）。
- Switch ARM64 JIT 保持启用但关闭 `JITArgs::FastMemory` 后复验：`cmd /c windowsbuild.bat` 通过。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，生成 `build_switch/GBAStation.nro`（脚本输出 21.47 MB）。
- 根据 `GBAStation (1).log` 定位 `CP15Reset()` 后，为 `NDS::Reset()` 设置 `NDS::Current` 并加入 Switch fault handler 空指针保护后复验：`cmd /c windowsbuild.bat` 通过。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，生成 `build_switch/GBAStation.nro`（脚本输出 21.47 MB）。
- 将 `NDS::Current` 改为普通静态指针、在构造完成和 `Reset()` 入口补充定位日志后复验：`cmd /c windowsbuild.bat` 通过。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，重新编译 `third_party/melonDS/src/NDS.cpp`，生成 `build_switch/GBAStation.nro`（脚本输出 21.47 MB）。
- 修复 Switch `ARMJIT_Memory` 中 `MemoryBase` 指向不可写 backing 导致 `CP15Reset` 清 DTCM 崩溃后复验：`cmd /c windowsbuild.bat` 通过。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，重新编译 `ARMJIT_Memory.cpp`、`NDS.cpp` 等 melonDS 源码，生成 `build_switch/GBAStation.nro`（脚本输出 21.47 MB）。
- 修复 melonDS SPU 输出帧/样本单位和 Switch audout 缓冲复用追踪后复验：`cmd /c windowsbuild.bat` 通过。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，重新编译 `src/game/audio/AudioManager.cpp` 并生成 `build_switch/GBAStation.nro`（脚本输出 21.47 MB）。
- 修复 NDS 视频红蓝通道、下屏触摸映射和屏幕布局切换后复验：`cmd /c windowsbuild.bat` 通过。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，重新编译 `MelonDSVideo.cpp`、`GameView.cpp`、`GameMenuView.cpp`、`GamePage.cpp` 并生成 `build_switch/GBAStation.nro`（脚本输出 21.48 MB）。
- 补充远程 Web NDS 导入、NDS 平滑快进、分离双屏布局和原始触摸轮询后复验：`cmd /c windowsbuild.bat` 通过。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，重新编译 `GameView.cpp`、`GameMenuView.cpp` 并将 `resources/web/app.js/index.html/style.css` 打包进 RomFS，生成 `build_switch/GBAStation.nro`（脚本输出 21.48 MB）。
- 补充 NDS 自定义双屏上下屏独立 X/Y/scale、GameDB 字段、正常音频缓冲与高倍率快进批处理后复验：`cmd /c windowsbuild.bat` 通过。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，重新编译 `core/game_database.cpp`、`AudioManager.cpp`、`GamePage.cpp`、`GameMenuView.cpp`、`GameView.cpp` 等并生成 `build_switch/GBAStation.nro`（脚本输出 21.49 MB）。
- 修复 NDS 自定义布局菜单内背景画面不同步后复验：`cmd /c windowsbuild.bat` 通过，生成 `build_windows/GBAStation.exe`。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，重新编译 `GameView.cpp` 并生成 `build_switch/GBAStation.nro`（脚本输出 21.49 MB）。
- 修复 NDS 画面同步遗漏上下屏字段、自定义布局默认非整数缩放观感后复验：`cmd /c windowsbuild.bat` 通过，重新编译 `GameMenuView.cpp`、`GameView.cpp` 并生成 `build_windows/GBAStation.exe`。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，重新编译 `GameMenuView.cpp`、`GameView.cpp` 并生成 `build_switch/GBAStation.nro`（脚本输出 21.49 MB）。
- 补充 NDS Hybrid 布局、自定义布局 4:3 约束、单屏调整延迟保存后复验：`cmd /c windowsbuild.bat` 通过，重新编译 `GameMenuView.cpp`、`GameView.cpp` 并生成 `build_windows/GBAStation.exe`。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，重新编译 `GameMenuView.cpp`、`GameView.cpp` 并生成 `build_switch/GBAStation.nro`（脚本输出 21.49 MB）。
- 修正 Hybrid 副屏为右侧上下两个 1x 原始分辨率画面后复验：`cmd /c windowsbuild.bat` 通过，重新编译 `GameView.cpp` 并生成 `build_windows/GBAStation.exe`。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，重新编译 `GameView.cpp` 并生成 `build_switch/GBAStation.nro`（脚本输出 21.49 MB）。
- 修正 NDS 自定义布局缩放基准为单屏 256x192 后复验：`cmd /c windowsbuild.bat` 通过，重新编译 `GameView.cpp` 并生成 `build_windows/GBAStation.exe`。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，重新编译 `GameView.cpp` 并生成 `build_switch/GBAStation.nro`（脚本输出 21.49 MB）。
- 修正 Hybrid 主屏 10/3、副屏 5/3 布局后复验：`cmd /c windowsbuild.bat` 通过，重新编译 `GameView.cpp` 并生成 `build_windows/GBAStation.exe`。
- 同次 Switch 复验：`E:\bin\msys64\usr\bin\bash.exe -lc 'cd /e/MyCode/MyEmuProject/Project/BeikLiveStation && ./switchbuild.sh'` 通过，重新编译 `GameView.cpp` 并生成 `build_switch/GBAStation.nro`（脚本输出 21.49 MB）。
