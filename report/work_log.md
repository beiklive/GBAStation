# 修复工作日志

> 开始时间: 2026-06-10
> 基于 fix_plan.md 执行

---

## Phase 1 — 关键 Bug 修复

### Fix A: `deinitCore()` 重置 `s_coreInitialized` ✅

- **文件**: `src/game/retro/LibretroLoader.cpp:534-541`
- **改动**: `deinitCore()` 末尾添加 `s_coreInitialized[static_cast<int>(m_coreType)] = false;`
- **影响**: 1 行新增。确保 `retro_deinit()` 后下次 `retro_init()` 可正常调用。

---

### Fix B: 移除失败路径中的 `deinitCore()` 调用 ✅

- **修改文件** (共 4 个，每文件改 3 处):
  - `src/game/mgba/GameRun.cpp:149,157,165` — 移除 `m_core.deinitCore();`
  - `src/emulator/CoreGenesis.cpp` — `_loadRom()` 失败分支
  - `src/emulator/CoreFceumm.cpp` — `_loadRom()` 失败分支
  - `src/emulator/CoreSnes9x.cpp` — `_loadRom()` 失败分支
- **影响**: 每文件 -3 行。失败时仅调用 `unload()`，保留核心的 `retro_init()` 状态供重试。

---

### Fix C: `s_audioSampleBatchCallback` 返回 0 问题 ✅

- **结果**: **跳过**。当前代码 (`LibretroLoader.cpp:909`) 已正确返回 `frames`，无此 bug。

---

### Fix D: `reset()` 移至 SRAM 加载之前 ✅

- **修改文件** (共 4 个):
  - `src/game/mgba/GameRun.cpp:25-28`
  - `src/emulator/CoreGenesis.cpp`
  - `src/emulator/CoreFceumm.cpp`
  - `src/emulator/CoreSnes9x.cpp`
- **改动**: 将 `m_core.reset()` 移到 `_loadSram()` 和 `_loadCheats()` 之前。
- **新顺序**: `_loadRom() → m_core.reset() → _loadSram() → _loadCheats() → m_ready = true`
- **影响**: 每文件 ~3 行重排。防止 `retro_reset()` 清零已加载的 SRAM。

---

### Fix E: CoreGenesis Switch 平台配置守卫 ✅

- **文件**: `src/emulator/CoreGenesis.cpp:32-34`
- **改动**: 移除 `#ifndef __SWITCH__` / `#endif` 守卫，`_initConfig()` 在所有平台执行。
- **影响**: -2 行。Switch 平台现在正确设置 ConfigManager 和 BIOS 目录。

---

## Phase 2 — LibretroLoader 鲁棒性增强

### Fix F: 为 `s_current` 添加实例 ID 保护 ✅

- **文件**: `src/game/retro/LibretroLoader.hpp`, `src/game/retro/LibretroLoader.cpp`
- **改动**:
  - 头文件: 新增 `static std::atomic<uint64_t> s_currentId;` 和 `uint64_t m_instanceId = 0;`
  - 实现: 定义 `s_currentId{0}`；`load()` 两条路径中设置 `m_instanceId = s_currentId.fetch_add(1)`
  - 6 个回调: `s_environmentCallback`, `s_videoRefreshCallback`, `s_audioSampleCallback`, `s_audioSampleBatchCallback`, `s_inputStateCallback` 中添加 `s_current->m_instanceId != s_currentId.load()` 检查
- **影响**: +12 行核心逻辑。防止已卸载的旧实例回调路由到新实例。

---

### Fix G: LibretroLoader 音频缓冲区大小上限 ✅

- **文件**: `src/game/retro/LibretroLoader.cpp`, `s_audioSampleBatchCallback`
- **改动**: 添加 `MAX_AUDIO_SAMPLES = 16384` 上限，超限时丢弃最旧采样。
- **影响**: +6 行。防止音频生产快于消费时的无限内存增长。

---

### Fix H: 修复头文件缩进异常 ✅

- **修改文件** (共 3 个):
  - `src/emulator/CoreGenesis.hpp:54-55`
  - `src/emulator/CoreFceumm.hpp:54-55`
  - `src/emulator/CoreSnes9x.hpp:54-55`
- **改动**: 修正 `Fps()` 和 `SampleRate()` 的额外缩进。
- **影响**: 仅格式变更。

---

## Phase 5 — 代码质量

### Fix L: 提取共享 SRAM/作弊码工具函数 ✅

- **新建**:
  - `src/core/CoreUtils.hpp` — 声明 4 个工具函数
  - `src/core/CoreUtils.cpp` — 实现 loadSram / saveSram / loadCheats / updateCheats
- **修改**:
  - `src/emulator/CoreGenesis.cpp` — 添加 `#include "core/CoreUtils.hpp"`，4 个方法委托给 core_utils
  - `src/emulator/CoreFceumm.cpp` — 同上
  - `src/emulator/CoreSnes9x.cpp` — 同上
- **影响**: 新建 2 文件，修改 3 文件。净减少 ~180 行重复代码。CoreMgba(<=GameRun.cpp) 因有额外的 RTC 处理逻辑，保持独立实现。

---

## 变更统计

```
新建: src/core/CoreUtils.hpp, src/core/CoreUtils.cpp
修改: 9 files changed, 57 insertions(+), 236 deletions(-)

涉及文件:
  src/emulator/CoreFceumm.cpp       | 85 +-----
  src/emulator/CoreFceumm.hpp       |  4 +-
  src/emulator/CoreGenesis.cpp      | 87 +-----
  src/emulator/CoreGenesis.hpp      |  4 +-
  src/emulator/CoreSnes9x.cpp       | 85 +-----
  src/emulator/CoreSnes9x.hpp       |  4 +-
  src/game/mgba/GameRun.cpp         |  5 +-
  src/game/retro/LibretroLoader.cpp | 17 +-
  src/game/retro/LibretroLoader.hpp |  2 +
```

---

---

## PicoDrive mmap 崩溃修复

### 根因定位

**崩溃位置**: `third_party/picodrive/pico/cart.c:740`
```c
rom = plat_mmap(0x02000000, rom_alloc_size, 0, 0);
```

**调用链**:
```
PicoCartAlloc → plat_mmap(0x02000000, ...) → mmap → VirtualAlloc(0x02000000, ...)
```

**根因**: Windows 上 `VirtualAlloc` 的非 NULL 地址参数是严格请求 — 若地址区域已被占用则直接返回 NULL，不像 Linux `mmap` 仅作 hint。`retro_init()` 中分配的资源（`vout_buf`、SH2 DRC cache 等）在之前 `retro_deinit()` 永不执行的架构中累积，导致进程虚拟地址空间碎片化，3 次加载后 `VirtualAlloc(0x02000000, 2MB)` 无法找到连续空间。

### 修复

- **文件**: `third_party/picodrive/pico/cart.c:738-740`
- **改动**: `plat_mmap(0x02000000, ...)` → `plat_mmap(0, ...)`，让 OS 自行选择可用地址
- **兼容性**: PicoDrive 的 `plat_mmap` 已处理地址不匹配情况（`is_fixed=0` 时接受任意地址再返回），此修改对 Linux/macOS 无影响（它们本来就是 hint），对 Windows 消除固定地址冲突

### 验证

其他 `0x02000000` 引用均非 host mmap 地址:
| 文件 | 用途 | 安全 |
|------|------|------|
| `compiler.c/memory.c` | MD 模拟地址空间检查 | ✅ |
| `emit_arm.c` | ARM 指令编码常量 | ✅ |
| `m68kcpu.h/m68kdasm.c` | 68K 位掩码 | ✅ |
| `libretro.c` (`pico_mmaps[]`) | 仅 3DS 平台 | ✅ |

### retro_deinit 调用链确认

```
CorePicoDrive::Cleanup()
  → m_core.unloadGame()   → picodrive_retro_unload_game() → PicoCartUnload → VirtualFree
  → m_core.deinitCore()   → picodrive_retro_deinit() → PicoExit → free(vout_buf,...)
                          → s_coreInitialized[3] = false  ← 下次会话可重新 retro_init
```

### 根因

Fix F 中 `s_currentId` 使用 `fetch_add(1)` 递增，但 `fetch_add` 返回**旧值**：

```cpp
// load() 中:
m_instanceId = s_currentId.fetch_add(1);  // s_currentId: 0→1, 返回 0
s_current = this;                         // m_instanceId=0, s_currentId=1

// 每个回调中:
if (s_current->m_instanceId != s_currentId.load()) return;  // 0 != 1 → ALWAYS RETURN!
```

导致所有 6 个 libretro 回调（视频刷新/音频采样/输入查询/环境变量）被短路，表现为所有核心黑屏、无声、无输入。

### 修复

从 5 个回调中移除实例 ID 检查，恢复为仅 `!s_current` 空指针检查：

- `s_environmentCallback` (line 688)
- `s_videoRefreshCallback` (line 839)  
- `s_audioSampleCallback` (line 905)
- `s_audioSampleBatchCallback` (line 914)
- `s_inputStateCallback` (line 936)

### 说明

实例 ID 保护旨在防止已卸载核心的回调路由到新核心实例，但在当前架构中该竞态窗口不存在 — 游戏线程在 unload() 之前已 join，回调不会与 unload/load 并发。空指针检查已足够。

| 项目 | 原因 |
|------|------|
| Phase 3: Fix I (桌面音频重采样) | 需要各平台测试环境，暂缓 |
| Phase 3: Fix J (PLL 对称化) | 低优先级优化，暂缓 |
| Phase 4: Fix K (存档版本化) | 新功能，需设计文件格式兼容策略，暂缓 |
| Phase 5: Fix M (frameskip 文档) | 非功能变更，暂缓 |
