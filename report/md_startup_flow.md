# MD 游戏启动流程（PicoDrive 核心）

## 总览调用链

```
用户点击游戏
  └→ GamePage 构造函数
       ├─ GameEntryInitialize()       // 数据库补全条目
       └─ _setupGame()
            ├─ PageInit()
            ├─ GameViewInitialize()   // new GameView(entry)
            │    └→ _registerGameRuntime()        ← 核心入口
            │         ├─ CreateEmulatorCore(EmuGenesis)
            │         └─ CorePicoDrive::SetupGame()
            │              ├─ _loadCore()          → libretro 符号绑定 + retro_init
            │              ├─ _initConfig()        → 写入 8 项 PicoDrive 配置默认值
            │              ├─ _loadRom()           → retro_load_game
            │              ├─ m_core.reset()       → retro_reset (Fix D)
            │              ├─ _loadSram()          → core_utils::loadSram
            │              ├─ _loadCheats()        → core_utils::loadCheats
            │              └─ m_ready = true
            │
            │    └→ AudioManager::init()           // 创建音频线程
            │    └→ _startGameThread()             // 启动游戏主循环线程
            │
            ├─ GameMenuInitialize()   // 菜单视图
            └─ RewindSelectorViewInitialize()       // 倒带视图
```

---

## 阶段 1：创建核心实例

```
GameView::_registerGameRuntime()              [GameView.cpp:535]
  │
  ├─ CreateEmulatorCore(EmuPlatform::EmuGenesis)    [EmulatorCoreFactory.cpp:21-22]
  │     └→ return new beiklive::picodrive::CorePicoDrive()
  │
  └─ m_core->SetupGame(gameEntry)                    [CorePicoDrive.cpp:11]
```

`EmulatorCoreFactory` 通过 `EmuPlatform` 枚举分发，MD/Genesis 路由到 `beiklive::picodrive::CorePicoDrive`。

---

## 阶段 2：_loadCore() — 静态符号绑定 + retro_init

```
_loadCore()                                    [CorePicoDrive.cpp:41]
  │
  └─ m_core.load(CoreType::Genesis)            [LibretroLoader.cpp:280]
       │
       ├─ unload()                               // 清理前一次实例（新实例无操作）
       │
       ├─ 静态符号绑定 (CoreType::Genesis 分支)    [LibretroLoader.cpp:372-396]
       │    fn_init                   = picodrive_retro_init
       │    fn_deinit                 = picodrive_retro_deinit
       │    fn_run                    = picodrive_retro_run
       │    fn_reset                  = picodrive_retro_reset
       │    fn_load_game              = picodrive_retro_load_game
       │    fn_unload_game            = picodrive_retro_unload_game
       │    fn_set_environment        = picodrive_retro_set_environment
       │    fn_set_video_refresh      = picodrive_retro_set_video_refresh
       │    fn_set_audio_sample       = picodrive_retro_set_audio_sample
       │    fn_set_audio_sample_batch = picodrive_retro_set_audio_sample_batch
       │    fn_set_input_poll         = picodrive_retro_set_input_poll
       │    fn_set_input_state        = picodrive_retro_set_input_state
       │    fn_get_system_av_info     = picodrive_retro_get_system_av_info
       │    fn_serialize              = picodrive_retro_serialize
       │    fn_unserialize            = picodrive_retro_unserialize
       │    fn_cheat_reset/set        = picodrive_retro_cheat_reset/set
       │    fn_get_memory_data/size   = picodrive_retro_get_memory_data/size
       │    （共 22 个符号）
       │
       ├─ m_handle = (void*)1                    // 哨兵值：静态链接
       ├─ s_current = this                       // 全局回调路由指针
       ├─ fn_set_environment(s_environmentCallback)
       ├─ fn_set_video_refresh(s_videoRefreshCallback)
       ├─ fn_set_audio_sample(s_audioSampleCallback)
       ├─ fn_set_audio_sample_batch(s_audioSampleBatchCallback)
       ├─ fn_set_input_poll(s_inputPollCallback)
       ├─ fn_set_input_state(s_inputStateCallback)
       └─ return true
       │
  └─ m_core.initCore()                          [LibretroLoader.cpp:517]
       ├─ s_coreInitialized[3]?
       │    false → fn_init() = picodrive_retro_init()
       │           → s_coreInitialized[3] = true
       │    true  → 跳过（核心已初始化，复用全局状态）
       └─ m_coreReady = true
```

**关键点**：
- PicoDrive 是纯静态链接，所有符号有 `picodrive_` 前缀，通过 `apply_retro_symbol_rename(picodrive_core picodrive)` CMake 脚本注入
- `s_coreInitialized[3]` 全程保护，首调用后不再调用 `retro_init()`，避免 PicoDrive 全局状态被重复初始化

---

## 阶段 3：_initConfig() — 注册配置默认值

```
_initConfig()                                  [CorePicoDrive.cpp:114]
  │
  ├─ ConfigManager::SetDefault(...)
  │    "core.picodrive_region"         → "Auto"
  │    "core.picodrive_sound_output"   → "stereo"
  │    "core.picodrive_frameskip"      → "0"       (跳帧禁用)
  │    "core.picodrive_render"         → "single field"
  │    "core.picodrive_aspect"         → "PAR"
  │    "core.picodrive_overclock"      → "disabled"
  │    "core.picodrive_audio_filter"   → "low-pass"
  │    "core.picodrive_lowpass_range"  → "60"
  │
  ├─ cfg->Save() → 持久化到 .\GBAStation\config\config.cfg
  │
  ├─ m_core.setConfigManager(SettingManager)
  │    → 核心通过 GET_VARIABLE 读取用户配置值
  │
  └─ m_core.setSystemDirectory(biosPath)
       → 指向 ./GBAStation/bios/（SEGA CD BIOS 等）
```

---

## 阶段 4：_loadRom() — 加载 ROM

```
_loadRom(m_gameEntry.path)                     [CorePicoDrive.cpp:59]
  │
  ├─ 空路径检查  → unload() → return false
  ├─ 文件存在检查 → unload() → return false
  │
  └─ m_core.loadGame(romPath)                  [LibretroLoader.cpp:559]
       │
       ├─ retro_game_info info{}               // 仅传 path，不传 data
       │    info.path = romPath.c_str()
       │    info.data = nullptr
       │    info.size = 0
       │
       ├─ fn_load_game(&info)
       │    → picodrive_retro_load_game()
       │    → PicoDrive 自行 fopen 读取 ROM
       │    → 失败 → return false → CorePicoDrive::unload()
       │
       ├─ fn_get_system_av_info(&m_avInfo)     // 获取分辨率/帧率/音频采样率
       │
       └─ m_gameLoaded = true
```

**PicoDrive ROM 格式自动检测**：支持 `.md`, `.bin`, `.gen`, `.smd`, `.sms`, `.gg`, `.cue`（SEGA CD），核心内部根据文件头判断。

---

## 阶段 5：retro_reset + SRAM + 金手指

```
m_core.reset()                                 [CorePicoDrive.cpp:20]
  → fn_reset() = picodrive_retro_reset()
  → 将模拟器置为开机状态，清理 loadGame 可能残留的临时状态

_loadSram()                                    [CorePicoDrive.cpp:134]
  → core_utils::loadSram(m_core, savePath, romName)
     → core.getMemorySize(RETRO_MEMORY_SAVE_RAM)
     → 如存在 <romName>.sav → 读取并 memcpy 到核心 SRAM 区域
     → PicoDrive 支持 MD 电池存档 + SEGA CD BRAM

_loadCheats()                                  [CorePicoDrive.cpp:146]
  → core_utils::loadCheats(m_core, cheatPath, m_cheats)
     → parseChtFile(<romName>.cht) → 解析 GameShark/ActionReplay 码
     → picodrive_retro_cheat_reset() + picodrive_retro_cheat_set()
```

---

## 阶段 6：返回 Frontend — 音频 + 游戏线程

```
SetupGame 返回 true
  │
  ├─ m_core->Fps()          → PicoDrive 报告的实际帧率(NTSC≈60/PAL≈50)
  ├─ m_core->SampleRate()   → PicoDrive 音频采样率(44100/48000)
  ├─ AudioManager::init(srate, 2)
  │    → 创建音频环形缓冲区 + 平台后端线程(ALSA/WinMM/CoreAudio)
  │
  ├─ GameSignal::resetAll()  // 清零暂停/快进/倒带等信号
  ├─ _initPlayTimeTracking()
  │
  └─ _startGameThread()
       └→ m_gameThread = new thread(_gameLoop)
            │
            while (m_running) {
              ├─ 消费 GameSignal(pause/ff/rewind/save/load/cheat...)
              ├─ SetButtonsFromSignal() → 写入 retro 按键状态
              ├─ m_core->RunFrame()  → picodrive_retro_run()
              │    → PicoDrive 执行一帧,触发回调:
              │        s_videoRefreshCallback → m_videoFrame(RGBA8888)
              │        s_audioSampleBatchCallback → m_audioBuffer
              ├─ _captureVideoFrame()  → 从 m_videoFrame 拷贝到 m_pendingFrame
              ├─ _pushFrameAudio()     → DrainAudio → AudioManager
              └─ _throttleFrameRate()  → 基于 PicoDrive FPS 的自旋节拍
            }
```

---

## PicoDrive 特有行为

| 特性 | 说明 |
|------|------|
| ROM 加载方式 | `info.path` 传路径，核心自行 `fopen`（存在中文路径编码风险） |
| Region 检测 | NTSC/PAL 自动检测，通过 `core.picodrive_region` 配置可覆写 |
| 多格式支持 | .md .bin .gen .smd .sms .gg .cue(SEGA CD) |
| 视频输出 | XRGB8888/RGB565 → LibretroLoader 统一转为 RGBA8888 |
| 音频输出 | 44100Hz/48000Hz int16 立体声，无前端重采样(桌面平台) |
| SRAM | RETRO_MEMORY_SAVE_RAM → 写入 .sav 文件 |
| State 序列化 | `retro_serialize_size()` → `retro_serialize()` ，状态大小随版本变动 |
| 金手指 | GameShark / Action Replay 格式，通过 .cht 文件加载 |
| BIOS | `setSystemDirectory(biosPath)` → SEGA CD 需要 bios_CD_U.bin 等 |

---

## 失败路径

```
_loadRom 失败时的清理链:
  romPath.empty()         → m_core.unload()  (清零函数指针, s_current=nullptr)
  文件不存在               → m_core.unload()
  loadGame 失败            → m_core.unload()
     ↓
  SetupGame 返回 false
     ↓
  _registerGameRuntime 中:
    delete m_core; m_core = nullptr;
    (CorePicoDrive 析构 → 不调用 Cleanup, 因为 m_ready=false)
```

> **注意**: 失败时调用 `unload()` 清零函数指针但不调用 `deinitCore()`，因此 `s_coreInitialized[3]` 保持 `true`，核心全局状态保留。下次加载同类型核心时 `initCore()` 将跳过 `retro_init()` 复用已有状态。这是 Fix B 的设计意图。

---

## Cleanup 路径（正常退出）

```
GameView 析构
  → _stopGameThread()  (m_running=false, join 游戏线程)
  → delete m_core
     → CorePicoDrive::~CorePicoDrive()
        → Cleanup()
           → _saveSram()    (core_utils::saveSram)
           → m_core.unloadGame()  (picodrive_retro_unload_game)
           (不调用 deinitCore, s_coreInitialized[3] 保持 true)
```

---

## 涉及文件一览

| 文件 | 职责 |
|------|------|
| `src/emulator/CorePicoDrive.hpp` | `beiklive::picodrive::CorePicoDrive` 类声明 |
| `src/emulator/CorePicoDrive.cpp` | 完整实现，含 `SetupGame`/`_loadCore`/`_loadRom`/`_initConfig` |
| `src/emulator/EmulatorCoreFactory.cpp` | 工厂路由 `EmuGenesis` → `CorePicoDrive` |
| `src/game/retro/LibretroLoader.hpp` | `LibretroLoader` 类（符号绑定/生命周期/回调） |
| `src/game/retro/LibretroLoader.cpp:372-396` | PicoDrive 静态符号绑定 (`picodrive_retro_*`) |
| `src/game/retro/LibretroLoader.cpp:517-531` | `initCore()` — `s_coreInitialized` 控制 |
| `src/game/retro/LibretroLoader.cpp:559-580` | `loadGame()` — `retro_load_game` 调用 |
| `src/core/CoreUtils.hpp/cpp` | 共享 SRAM/作弊码工具函数 |
| `src/ui/utils/GameView.cpp:535-580` | `_registerGameRuntime()` — 创建核心并启动游戏线程 |
| `src/ui/page/GamePage.cpp` | 页面构造，组装 GameView + 菜单 + 倒带 UI |
| `src/core/common.h:144` | `GetCorePath(EmuGenesis)` → `""` (静态链接) |
| `third_party/CMakeLists.txt:307-502` | `picodrive_core` 静态库构建 + 符号重命名 |
