# mGBA 源码直连迁移方案

## 1. 任务目的

当前项目的 GBA / GBC / GB 模拟通过 libretro 形式接入 mGBA：

- 项目核心包装：`src/game/mgba/GameRun.hpp`
- 项目核心实现：`src/game/mgba/GameRun.cpp`
- libretro 加载层：`src/game/retro/LibretroLoader.hpp`
- libretro 加载层：`src/game/retro/LibretroLoader.cpp`
- mGBA libretro 源码：`third_party/mgba/src/platform/libretro/libretro.c`
- mGBA libretro 头：`third_party/mgba/src/platform/libretro/libretro.h`

本迁移目标是新增一个直接使用 `third_party/mgba` 源码 API 的 mGBA native core，用于替代或逐步替代当前 libretro mGBA core。

迁移后需要满足以下需求：

1. 支持 GBA / GBC / GB 模拟。
2. 支持内存搜索和内存修改。
3. 支持 raw 格式金手指，并继续兼容当前 UI 的金手指开关流程。
4. 支持或为后续实现 GBA 联机能力打好基础。
5. 旧的 libretro mGBA 即时存档需要尽量继续可读。
6. 迁移期间不破坏 NES / SNES / melonDS 等其他核心。

## 2. 总体结论

直接接 mGBA 源码是可行的，并且对内存搜索、原生金手指和 GBA 联机更有利。

| 功能 | 结论 | 依据 |
|---|---|---|
| GBA / GBC / GB 模拟 | 可行 | mGBA `mCore` 同时支持 `mPLATFORM_GBA` 和 `mPLATFORM_GB` |
| 内存搜索 / 修改 | 可行，且比 libretro 更好 | mGBA 已有 `mCoreMemorySearch()` / `mCoreMemorySearchRepeat()` |
| raw 金手指 | 可行 | mGBA 暴露 `mCheatDevice` / `mCheatAddLine()` / `mCheatParseFile()` |
| GBA 联机 | 可行但工作量大 | mGBA 有 `GBASIOLockstep` / `GBASIOLockstepNode` / SIO driver |
| GB/GBC 联机 | 需要后续专项确认 | 本次源码阅读只确认 GBA SIO lockstep 路径明确 |
| 旧 libretro 即时存档 | 大概率兼容 | libretro mGBA 的 `retro_serialize()` 使用 `mCoreSaveStateNamed()` |

推荐策略：新增 `CoreMgbaNative` 并与现有 `CoreMgba` 并存，验证稳定后再切默认核心。不要第一步就删除 libretro mGBA 路径。

## 3. 现有项目入口

### 3.1 mGBA 当前项目封装

当前 mGBA 核心类：

- `src/game/mgba/GameRun.hpp`
- `src/game/mgba/GameRun.cpp`

当前类名：

```cpp
beiklive::gba::CoreMgba
```

当前依赖：

```cpp
beiklive::LibretroLoader m_core;
```

关键职责：

- `_loadCore()`
- `_loadRom()`
- `_loadSram()`
- `_loadRtc()`
- `_loadCheats()`
- `_updateCheats()`
- `_saveSram()`
- `_saveRtc()`
- `Serialize()`
- `Unserialize()`
- `RunFrame()`
- `DrainAudio()`
- `GetVideoFrame()`

迁移后这些能力都需要在 native core 中重新实现。

### 3.2 核心工厂

核心创建入口：

- `src/emulator/EmulatorCoreFactory.cpp`

当前 GBA / GB / GBC 平台返回：

```cpp
return new beiklive::gba::CoreMgba();
```

迁移建议：

1. 先新增 `CoreMgbaNative`。
2. 增加配置项决定使用 native 还是 libretro。
3. 默认初期仍使用 libretro。
4. native 验证稳定后再切默认。

### 3.3 通用核心接口

接口文件：

- `src/emulator/IEmulatorCore.hpp`

native mGBA 必须继续实现：

- `SetupGame()`
- `Cleanup()`
- `RunFrame()`
- `Reset()`
- `Serialize()`
- `Unserialize()`
- `GetVideoFrame()`
- `DrainAudio()`
- `SetButtonState()`
- `SetButtonsFromSignal()`
- `GameWidth()`
- `GameHeight()`
- `Fps()`
- `SampleRate()`
- `ApplyCheats()`
- `ToggleCheat()`
- `ReloadCheats()`
- `SetCheatPath()`
- `saveSram()`
- `getSramData()`
- `getSramSize()`

## 4. mGBA 源码关键 API

### 4.1 mCore 主接口

文件：

- `third_party/mgba/include/mgba/core/core.h`

关键结构：

```c
struct mCore
```

核心 API：

```c
struct mCore* mCoreFind(const char* path);
struct mCore* mCoreFindVF(struct VFile* vf);
struct mCore* mCoreCreate(enum mPlatform);
bool mCoreLoadFile(struct mCore* core, const char* path);
```

运行相关：

```c
core->init(core);
core->deinit(core);
core->loadROM(core, vf);
core->unloadROM(core);
core->reset(core);
core->runFrame(core);
core->setKeys(core, keys);
```

画面相关：

```c
core->desiredVideoDimensions(core, &width, &height);
core->setVideoBuffer(core, buffer, stride);
core->getPixels(core, &buffer, &stride);
```

音频相关：

```c
core->getAudioChannel(core, 0);
core->getAudioChannel(core, 1);
core->setAudioBufferSize(core, samples);
```

状态相关：

```c
core->stateSize(core);
core->saveState(core, rawState);
core->loadState(core, rawState);
mCoreSaveStateNamed(core, vf, flags);
mCoreLoadStateNamed(core, vf, flags);
```

内存相关：

```c
core->listMemoryBlocks(core, &blocks);
core->getMemoryBlock(core, id, &size);
core->busRead8(core, address);
core->busWrite8(core, address, value);
core->rawRead8(core, address, segment);
core->rawWrite8(core, address, segment, value);
```

金手指相关：

```c
core->cheatDevice(core);
```

### 4.2 内存块信息

文件：

- `third_party/mgba/include/mgba/core/interface.h`

结构：

```c
struct mCoreMemoryBlock {
    size_t id;
    const char* internalName;
    const char* shortName;
    const char* longName;
    uint32_t start;
    uint32_t end;
    uint32_t size;
    uint32_t flags;
    uint16_t maxSegment;
    uint32_t segmentStart;
};
```

flags：

```c
mCORE_MEMORY_READ
mCORE_MEMORY_WRITE
mCORE_MEMORY_RW
mCORE_MEMORY_WORM
mCORE_MEMORY_MAPPED
mCORE_MEMORY_VIRTUAL
```

用途：

- 枚举可搜索内存区域。
- 建立 UI 显示名称。
- 用 `address + segment` 作为内部搜索结果定位。
- 用 `getMemoryBlock()` 获取直接内存指针。

### 4.3 mGBA 自带内存搜索

文件：

- `third_party/mgba/include/mgba/core/mem-search.h`
- `third_party/mgba/src/core/mem-search.c`

API：

```c
void mCoreMemorySearch(
    struct mCore* core,
    const struct mCoreMemorySearchParams* params,
    struct mCoreMemorySearchResults* out,
    size_t limit);

void mCoreMemorySearchRepeat(
    struct mCore* core,
    const struct mCoreMemorySearchParams* params,
    struct mCoreMemorySearchResults* inout);
```

搜索类型：

```c
mCORE_MEMORY_SEARCH_INT
mCORE_MEMORY_SEARCH_STRING
mCORE_MEMORY_SEARCH_GUESS
```

搜索条件：

```c
mCORE_MEMORY_SEARCH_EQUAL
mCORE_MEMORY_SEARCH_GREATER
mCORE_MEMORY_SEARCH_LESS
mCORE_MEMORY_SEARCH_ANY
mCORE_MEMORY_SEARCH_DELTA
mCORE_MEMORY_SEARCH_DELTA_POSITIVE
mCORE_MEMORY_SEARCH_DELTA_NEGATIVE
mCORE_MEMORY_SEARCH_DELTA_ANY
```

结果结构：

```c
struct mCoreMemorySearchResult {
    uint32_t address;
    int segment;
    uint32_t guessDivisor;
    uint32_t guessMultiplier;
    enum mCoreMemorySearchType type;
    int width;
    int32_t oldValue;
};
```

迁移建议：

- 第一版直接封装 mGBA 自带搜索，不要自己重写搜索引擎。
- UI 层仍使用项目自己的数据结构，避免 UI 直接依赖 mGBA C 类型。
- 搜索结果保存时使用 `address + segment + width`，不要只保存裸 offset。

### 4.4 mGBA 金手指 API

文件：

- `third_party/mgba/include/mgba/core/cheats.h`
- `third_party/mgba/include/mgba/internal/gba/cheats.h`
- `third_party/mgba/src/core/cheats.c`
- `third_party/mgba/src/gb/cheats.c`
- `third_party/mgba/src/platform/libretro/libretro.c`

通用 API：

```c
struct mCheatDevice* device = core->cheatDevice(core);
struct mCheatSet* set = device->createSet(device, name);
mCheatAddLine(set, line, type);
mCheatAddSet(device, set);
mCheatDeviceClear(device);
mCheatParseFile(device, vf);
mCheatParseLibretroFile(device, vf);
mCheatSaveFile(device, vf);
```

GBA 支持类型见：

- `third_party/mgba/include/mgba/internal/gba/cheats.h`

包括：

```c
GBA_CHEAT_AUTODETECT
GBA_CHEAT_CODEBREAKER
GBA_CHEAT_GAMESHARK
GBA_CHEAT_PRO_ACTION_REPLAY
GBA_CHEAT_VBA
```

当前 libretro `retro_cheat_set()` 的逻辑在：

- `third_party/mgba/src/platform/libretro/libretro.c`

迁移时需要参考其中对 libretro raw 字符串的拆分逻辑。

### 4.5 即时存档 API

文件：

- `third_party/mgba/include/mgba/core/serialize.h`
- `third_party/mgba/src/core/serialize.c`
- `third_party/mgba/src/platform/libretro/libretro.c`

重要 flags：

```c
SAVESTATE_SCREENSHOT
SAVESTATE_SAVEDATA
SAVESTATE_CHEATS
SAVESTATE_RTC
SAVESTATE_METADATA
SAVESTATE_ALL
```

libretro mGBA 当前实现：

```c
retro_serialize_size:
mCoreSaveStateNamed(core, vfm, SAVESTATE_SAVEDATA | SAVESTATE_RTC)

retro_serialize:
mCoreSaveStateNamed(core, vfm, SAVESTATE_SAVEDATA | SAVESTATE_RTC)

retro_unserialize:
mCoreLoadStateNamed(core, vfm, SAVESTATE_RTC)
```

结论：

- 旧 libretro 即时存档不是 RetroArch 私有格式。
- 它本质上是 mGBA `mCoreSaveStateNamed()` 产物。
- native core 必须用 `mCoreSaveStateNamed()` / `mCoreLoadStateNamed()` 兼容旧存档。
- 不要只用 `core->saveState()` / `core->loadState()` 做唯一实现，因为 raw state 与 named state 文件格式不同。

### 4.6 GBA 联机 API

文件：

- `third_party/mgba/include/mgba/internal/gba/sio.h`
- `third_party/mgba/include/mgba/internal/gba/sio/lockstep.h`

关键结构：

```c
struct GBASIOLockstep;
struct GBASIOLockstepNode;
```

关键 API：

```c
void GBASIOLockstepInit(struct GBASIOLockstep*);
void GBASIOLockstepNodeCreate(struct GBASIOLockstepNode*);
bool GBASIOLockstepAttachNode(struct GBASIOLockstep*, struct GBASIOLockstepNode*);
void GBASIOLockstepDetachNode(struct GBASIOLockstep*, struct GBASIOLockstepNode*);
```

结论：

- 本地多 GBA 实例联机可行。
- 网络联机不是本阶段目标。
- GB / GBC link cable 需要后续单独分析，不纳入第一阶段承诺。

## 5. 迁移目标设计

### 5.1 新增 native mGBA core

建议新增文件：

- `src/emulator/mgba_native/MgbaNativeCore.hpp`
- `src/emulator/mgba_native/MgbaNativeCore.cpp`
- `src/emulator/mgba_native/MgbaNativeCheats.hpp`
- `src/emulator/mgba_native/MgbaNativeCheats.cpp`
- `src/emulator/mgba_native/MgbaNativeMemory.hpp`
- `src/emulator/mgba_native/MgbaNativeMemory.cpp`
- `src/emulator/mgba_native/MgbaNativeLink.hpp`
- `src/emulator/mgba_native/MgbaNativeLink.cpp`

类名建议：

```cpp
beiklive::mgba_native::MgbaNativeCore
```

不要直接覆盖当前：

```cpp
beiklive::gba::CoreMgba
```

原因：

- 方便 A/B 测试。
- 方便回退。
- 避免迁移中影响当前可用的 GBA/GBC/GB 模拟。

### 5.2 CMake 集成

当前根 `CMakeLists.txt` 已经：

- `set(MGBA_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mgba)`
- `add_subdirectory(third_party)`
- 针对 `mgba` 和 `mgba_libretro` 加过平台宏

需要确认 `third_party` 是否已经创建 `mgba` 静态库 target。

如果存在 `TARGET mgba`：

```cmake
target_link_libraries(${PROJECT_NAME} PRIVATE mgba)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mgba/include
)
```

如果没有，则需要调整 `third_party/mgba` 的构建选项，生成可链接的 mGBA core library。

注意：

- 当前桌面平台只复制 `mgba_libretro` DLL。
- native 迁移后桌面平台也需要链接 `mgba` 静态库或共享库。
- 不要影响 `mgba_libretro`，初期两者并存。

### 5.3 工厂切换策略

在 `src/emulator/EmulatorCoreFactory.cpp` 增加配置分支：

```cpp
if (platform is GBA/GBC/GB) {
    if (GET_SETTING_KEY_STR("core.mgba_backend", "libretro") == "native")
        return new beiklive::mgba_native::MgbaNativeCore();
    return new beiklive::gba::CoreMgba();
}
```

新增设置项：

```text
core.mgba_backend = libretro | native
```

阶段验收前默认：

```text
libretro
```

native 稳定后再切：

```text
native
```

## 6. 功能实现方案

### 6.1 ROM 加载

native core 初始化流程建议：

```cpp
core = mCoreFind(romPath.c_str());
if (!core) core = mCoreCreate(mPLATFORM_GBA or mPLATFORM_GB);
core->init(core);
mCoreInitConfig(core, "BeikLiveStation");
mCoreLoadConfig(core);
mCoreLoadFile(core, romPath.c_str());
core->reset(core);
```

注意：

- `mCoreFind()` 可以根据 ROM 判断 GBA 或 GB。
- GBC 归入 `mPLATFORM_GB`。
- 需要确认 `mCoreLoadFile()` 是否包含 preload/loadROM 完整流程。
- 如果手动打开 `VFile`，要正确 close。

### 6.2 视频输出

当前项目 `GameView` 需要：

```cpp
LibretroLoader::VideoFrame GetVideoFrame() const
```

native mGBA 应维护一个 RGBA frame buffer：

```cpp
std::vector<color_t> m_videoBuffer;
core->desiredVideoDimensions(core, &w, &h);
core->setVideoBuffer(core, m_videoBuffer.data(), stride);
```

每帧：

```cpp
core->runFrame(core);
core->getPixels(core, &pixels, &stride);
```

然后转换为项目现有 `VideoFrame` 结构。

注意：

- mGBA 的 `color_t` 受编译宏影响，可能是 16 位或 32 位。
- 当前渲染链多用 RGBA，需要确认颜色通道。
- 若颜色不对，参考 mGBA `mColorFrom555()` 和 libretro 输出路径。

### 6.3 音频输出

mGBA 音频通过 blip buffer 输出。

需要使用：

```c
core->getAudioChannel(core, 0);
core->getAudioChannel(core, 1);
core->setAudioBufferSize(core, samples);
```

实现 `DrainAudio(std::vector<int16_t>& out)`：

- 从左右声道 blip buffer 读取样本。
- 交错写入 `LRLRLR`。
- 返回给现有 `AudioManager`。

参考文件：

- `third_party/mgba/src/platform/sdl`
- `third_party/mgba/src/platform/qt`
- `third_party/mgba/src/platform/libretro/libretro.c`

### 6.4 输入

当前项目通过：

```cpp
SetButtonsFromSignal()
```

把按键 bitmask 传给核心。

native mGBA 可直接：

```cpp
core->setKeys(core, keys);
```

需要建立项目按钮到 mGBA key bit 的映射。注意 GBA 与 GB 键位基本类似，但 GB 没有 L/R。

### 6.5 SRAM / RTC

当前 libretro 路径通过：

```cpp
RETRO_MEMORY_SAVE_RAM
RETRO_MEMORY_RTC
```

native mGBA 应优先使用：

```c
core->savedataClone(core, &sram);
core->savedataRestore(core, sram, size, writeback);
```

或者使用 mGBA 标准 save load API：

```c
core->loadSave(core, vf);
```

保存路径继续沿用当前项目规则：

- `.sav`
- `.rtc`

当前项目路径逻辑在：

- `src/game/mgba/GameRun.cpp`
- `_loadSram()`
- `_saveSram()`
- `_loadRtc()`
- `_saveRtc()`
- `_rtcFilePath()`

迁移时目标是保持外部文件路径不变。

### 6.6 即时存档兼容

必须实现：

```cpp
bool Serialize(std::vector<uint8_t>& outBuf) const;
bool Unserialize(const std::vector<uint8_t>& buf);
```

实现要求：

- 使用内存 `VFile`。
- 保存使用 `mCoreSaveStateNamed()`。
- 加载使用 `mCoreLoadStateNamed()`。

保存 flags 与 libretro 对齐：

```c
SAVESTATE_SAVEDATA | SAVESTATE_RTC
```

加载 flags 与 libretro 对齐：

```c
SAVESTATE_RTC
```

不要只使用 raw：

```c
core->saveState(core, raw);
core->loadState(core, raw);
```

原因：

- 旧 libretro state 是 `mCoreSaveStateNamed()` 格式。
- raw state 只适合内部高速状态，不保证读取旧文件。

验收：

- 使用旧版本 libretro mGBA 保存的 state 文件。
- 切 native mGBA 后能读取。
- 读取后画面、声音、SRAM、RTC 状态正常。

### 6.7 raw 金手指

当前项目金手指系统：

- `src/core/cheat/CheatSystem.hpp`
- `src/core/cheat/CheatSystem.cpp`
- `src/core/enums.h`
- `src/ui/view/GameMenuView.cpp`
- `src/ui/page/GamePage.cpp`

当前 libretro mGBA 应用方式：

- `CoreMgba::_updateCheats()`
- `m_core.cheatReset()`
- `m_core.cheatSet(index, true, code)`

native mGBA 应改为：

1. 通过 `core->cheatDevice(core)` 获取 device。
2. 清空或重建 `mCheatSet`。
3. 对每个启用的 raw cheat，将项目保存的 code 拆成 mGBA 可接受的行。
4. 调 `mCheatAddLine(set, line, type)`。
5. 设置 `set->enabled = true`。
6. 调 `set->refresh(set, device)`。

必须参考：

- `third_party/mgba/src/platform/libretro/libretro.c`
- `retro_cheat_reset()`
- `retro_cheat_set()`

原因：

- libretro raw 格式可能是 `AAAAAAAA+BBBBBBBB+...`。
- mGBA `mCheatAddLine()` 更接近普通 `XXXXXXXX YYYYYYYY` 或单行 code。
- GBA 和 GB 的拆分长度不同。

迁移要求：

- 当前 UI 仍使用 `std::vector<CheatEntry>`。
- UI 开关仍走 `ToggleCheat(idx, enabled)`。
- 连续打开多个金手指不能丢请求。当前 `GameSignal` 已改为队列，native core 只需实现 `ToggleCheat()`。
- invalid/category 条目不能传给 mGBA。

### 6.8 内存搜索 / 修改

native mGBA 内存搜索优先使用 mGBA 自带 API。

建议新增项目封装：

- `src/emulator/mgba_native/MgbaNativeMemory.hpp`
- `src/emulator/mgba_native/MgbaNativeMemory.cpp`

封装职责：

- 枚举 `mCoreMemoryBlock`
- 转换为项目 UI 使用的 memory region
- 调用 `mCoreMemorySearch()`
- 调用 `mCoreMemorySearchRepeat()`
- 对搜索结果执行读取
- 对搜索结果执行写入
- 对搜索结果执行冻结

内部地址格式：

```cpp
struct MgbaMemoryAddress {
    uint32_t address;
    int segment;
    int width;
};
```

不要只保存裸 offset。

修改写入可用：

```c
core->rawWrite8(core, address, segment, value);
core->rawWrite16(core, address, segment, value);
core->rawWrite32(core, address, segment, value);
```

或使用 bus write：

```c
core->busWrite8(core, address, value);
```

推荐：

- 搜索结果来自 `mCoreMemorySearchResult`，优先使用 `rawRead/rawWrite + segment`。
- UI 显示地址时显示 `address` 和 `segment`。

冻结：

- 每帧 `core->runFrame()` 前执行一次。
- 必要时后续增加 `runFrame()` 后执行一次。

### 6.9 GBA 联机

本阶段目标是打基础，不建议第一阶段直接承诺完整 UI 联机。

建议分三步：

#### 阶段 A：本地双 GBA 实例验证

新增实验类：

- `src/emulator/mgba_native/MgbaNativeLink.hpp`
- `src/emulator/mgba_native/MgbaNativeLink.cpp`

使用：

```c
GBASIOLockstep link;
GBASIOLockstepInit(&link);

GBASIOLockstepNode node1;
GBASIOLockstepNodeCreate(&node1);
GBASIOLockstepAttachNode(&link, &node1);

GBASIOLockstepNode node2;
GBASIOLockstepNodeCreate(&node2);
GBASIOLockstepAttachNode(&link, &node2);
```

每个 GBA `mCore` 需要挂接对应 SIO driver。具体挂接点需要阅读：

- `third_party/mgba/src/gba/sio/lockstep.c`
- `third_party/mgba/src/gba/sio.c`
- `third_party/mgba/include/mgba/internal/gba/sio.h`

#### 阶段 B：GameView 多实例运行

需要解决：

- 多 core 同步推进。
- 多玩家输入。
- 多画面显示。
- 音频混音或只输出玩家 1。
- 每个实例独立 save path。
- 暂停 / 快进 / reset 的多实例行为。

#### 阶段 C：正式 UI

UI 需要：

- 选择玩家数。
- 每个玩家 ROM。
- 每个玩家 save。
- 屏幕布局。
- 手柄绑定。

网络联机暂不纳入。

GB/GBC link cable 需要后续专项分析，不在此方案第一阶段实现范围内。

## 7. 实施阶段

### 阶段 0：准备与开关

目标：

- 保留现有 libretro mGBA。
- 新增 native backend 配置。
- 项目能同时编译 libretro mGBA 和 native mGBA。

修改文件：

- `CMakeLists.txt`
- `src/emulator/EmulatorCoreFactory.cpp`
- `src/core/common.cpp`
- `src/ui/page/SettingPage.cpp`

新增配置：

```text
core.mgba_backend = libretro | native
```

验收：

- 默认 libretro 行为不变。
- 切 native 后能创建新 core，哪怕功能还未完全实现。

### 阶段 1：native mGBA 单机运行

目标：

- GBA / GB / GBC ROM 能启动。
- 视频能显示。
- 音频能输出。
- 输入可用。
- Reset 可用。

新增文件：

- `src/emulator/mgba_native/MgbaNativeCore.hpp`
- `src/emulator/mgba_native/MgbaNativeCore.cpp`

验收：

- GBA 游戏可运行。
- GBC 游戏可运行。
- GB 游戏可运行。
- 帧率、画面尺寸、音频正常。

### 阶段 2：存档与 RTC

目标：

- `.sav` 兼容当前路径。
- RTC 兼容当前路径和设置。
- 退出时正常保存。

修改参考：

- `src/game/mgba/GameRun.cpp`

验收：

- 老 `.sav` 可读。
- 新 `.sav` 可被旧 libretro 路径读取。
- RTC 游戏时间行为与当前 libretro 路径一致或有明确说明。

### 阶段 3：即时存档兼容

目标：

- native core 能保存即时存档。
- native core 能读取旧 libretro mGBA 即时存档。

必须使用：

```c
mCoreSaveStateNamed(core, vf, SAVESTATE_SAVEDATA | SAVESTATE_RTC)
mCoreLoadStateNamed(core, vf, SAVESTATE_RTC)
```

验收：

- 使用当前版本保存一个 libretro mGBA state。
- 切 native 后读取成功。
- native 保存的 state 能被 native 读取。
- 视情况测试 native state 是否能回到 libretro 读取。

### 阶段 4：raw 金手指

目标：

- 当前 UI 的金手指列表和开关继续可用。
- raw code 能应用到 mGBA `mCheatDevice`。
- 连续打开多个金手指不丢请求。

新增文件：

- `src/emulator/mgba_native/MgbaNativeCheats.hpp`
- `src/emulator/mgba_native/MgbaNativeCheats.cpp`

参考：

- `third_party/mgba/src/platform/libretro/libretro.c`
- `retro_cheat_set()`

验收：

- 旧 `.cht` 中 GBA code 可用。
- GB/GBC code 可用。
- enable/disable 立即生效。
- 分类/invalid 条目不传入 mGBA。

### 阶段 5：内存搜索与修改

目标：

- 可枚举 mGBA memory blocks。
- 可进行精确值搜索。
- 可再次筛选。
- 可一次性修改。
- 可冻结。
- 搜索结果可保存为项目自有 memory patch。

新增文件：

- `src/emulator/mgba_native/MgbaNativeMemory.hpp`
- `src/emulator/mgba_native/MgbaNativeMemory.cpp`

复用文档：

- `report/libretro内存搜索修改执行方案.md`

但 native mGBA 版本应优先使用 mGBA 自带 `mCoreMemorySearch()`，而不是 libretro memory region 抽象。

验收：

- GBA 金钱/生命等数值可搜索。
- GB/GBC 简单数值可搜索。
- 修改后游戏内生效。
- 冻结后每帧维持目标值。

### 阶段 6：GBA 本地联机实验

目标：

- 两个 GBA mCore 实例通过 lockstep SIO 连接。
- 能运行基础联机测试游戏或已知支持联机的游戏。

新增：

- `src/emulator/mgba_native/MgbaNativeLink.hpp`
- `src/emulator/mgba_native/MgbaNativeLink.cpp`

验收：

- 双实例不崩溃。
- link handshake 有响应。
- 至少一个简单联机场景可验证。

### 阶段 7：默认切换与清理

前置条件：

- 阶段 1-5 全部通过。
- 阶段 6 至少完成技术验证，或明确标记为实验功能。

目标：

- 将 `core.mgba_backend` 默认值改为 `native`。
- 保留 libretro fallback 一段时间。
- 文档记录兼容性差异。

## 8. 验收清单

### 单机模拟

- GBA 商业游戏启动。
- GBC 游戏启动。
- GB 游戏启动。
- 按键正常。
- 声音正常。
- 画面比例和分辨率正常。
- 快进/暂停/重置正常。

### 存档

- 老 `.sav` 可读。
- 新 `.sav` 可保存。
- 退出自动保存正常。
- SRAM 自动保存检测不崩溃。

### RTC

- 宝可梦等 RTC 游戏时间正常。
- `core.mgba_rtc_mode` 行为有明确处理。

### 即时存档

- 旧 libretro state 可读。
- native state 可读。
- 读档后音频、视频、输入、SRAM 正常。

### 金手指

- GBA raw cheat 生效。
- GB/GBC raw cheat 生效。
- 多个 cheat 连续开启不丢。
- 禁用后重新应用列表正确。

### 内存搜索

- 可枚举内存块。
- 精确搜索可用。
- 变化/增加/减少搜索可用或列入后续。
- 修改可用。
- 冻结可用。

### 联机

- GBA 双实例 lockstep 技术验证通过。
- 若未完成 UI，需标记为实验能力。

## 9. 风险与注意事项

### 9.1 编译风险

当前项目主要链接 `mgba_libretro`，native 需要确认 `mgba` target 是否完整可链接。

需要重点测试：

- Windows MinGW
- Switch
- macOS
- Linux

### 9.2 视频格式风险

mGBA `color_t` 可能随编译宏变化。native 输出可能与 libretro 输出通道不同。

需要用截图确认：

- 红绿蓝通道正确。
- alpha 正确。
- GB/GBC 调色板正确。

### 9.3 音频风险

libretro 路径隐藏了 blip buffer 处理。native 需要自己 drain 音频。

需要确认：

- 采样率。
- 声道顺序。
- buffer 过大/过小导致延迟或爆音。

### 9.4 即时存档风险

旧 state 大概率兼容，但依赖：

- 同一 mGBA 版本。
- 同一 ROM。
- 相近配置。
- 使用 `mCoreLoadStateNamed()` 而不是 raw `loadState()`。

如果失败，应回退到 libretro backend 读取。

### 9.5 金手指格式风险

项目当前 `CheatEntry::code` 可能保存 libretro `+` 格式。mGBA native 不直接吃这个格式，必须转换。

转换逻辑应参考 libretro mGBA，不要重新猜。

### 9.6 联机风险

GBA 本地联机是多 core 同步问题，涉及帧推进、输入、音频、存档、UI。

网络联机暂不承诺。

GB/GBC link cable 暂不承诺。

## 10. 对其他 AI / 开发者的执行提示

如果你接手本任务，请按以下顺序阅读：

1. `src/emulator/IEmulatorCore.hpp`
2. `src/game/mgba/GameRun.hpp`
3. `src/game/mgba/GameRun.cpp`
4. `src/emulator/EmulatorCoreFactory.cpp`
5. `src/game/retro/LibretroLoader.hpp`
6. `src/game/retro/LibretroLoader.cpp`
7. `src/core/cheat/CheatSystem.hpp`
8. `src/core/cheat/CheatSystem.cpp`
9. `third_party/mgba/include/mgba/core/core.h`
10. `third_party/mgba/include/mgba/core/interface.h`
11. `third_party/mgba/include/mgba/core/mem-search.h`
12. `third_party/mgba/include/mgba/core/cheats.h`
13. `third_party/mgba/include/mgba/core/serialize.h`
14. `third_party/mgba/src/platform/libretro/libretro.c`
15. `third_party/mgba/include/mgba/internal/gba/sio/lockstep.h`

不要先改 UI。

先让 `MgbaNativeCore` 在现有 `IEmulatorCore` 下跑通，再接 UI 新能力。

不要删除现有 `CoreMgba`。

不要改变 melonDS 金手指路径。

不要让 native mGBA 影响 NES/SNES libretro 核心。

## 11. 最终目标形态

最终项目应形成：

```text
GBA / GB / GBC:
  默认：MgbaNativeCore
  备用：CoreMgba(libretro)

NES / SNES:
  继续 libretro

NDS:
  继续 melonDS native
  继续 usrcheat.dat / AR Code
```

mGBA native 提供：

- 单机模拟
- SRAM / RTC
- 即时存档兼容
- raw 金手指
- 内存搜索
- 内存修改
- 冻结
- GBA 本地联机实验能力

这就是本迁移任务的完整目标。
