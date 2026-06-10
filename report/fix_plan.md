# BeikLiveStation 多核心隔离修复计划书

> 制定日期: 2026-06-10
> 基于 GPT 建议的多核心 libretro 整合检查结果

---

## 核心原则

每个核心包装类独立拥有自己的 `LibretroLoader` 实例，所有修复遵循「修改不影响其他核心类型」原则。当前架构仅支持单核心运行（同一时刻只有一个核心 active），修复目标是在生命周期安全、错误恢复、跨核心切换三个维度保证隔离。

---

## Phase 1 — 关键 Bug 修复（低风险，高影响）

### Fix A: `deinitCore()` 未重置 `s_coreInitialized` 导致第二次同类型加载跳过 `retro_init()`

- **根因**: `LibretroLoader.cpp:534-540` — `deinitCore()` 调用 `fn_deinit()` 释放核心内部状态后，未将 `s_coreInitialized[idx]` 置 `false`。下一次 `initCore()` 检测到 `true` 跳过 `fn_init()`，核心在已释放状态下运行 → UB/崩溃。
- **触发路径**: `_loadRom()` 失败时（ROM 不存在、加载失败），所有 4 个核心包装类都调用 `deinitCore()` → `unload()`。随后用户修正 ROM 路径重试，同类型核心跳过 `retro_init()`。
- **修改文件**: `src/game/retro/LibretroLoader.cpp`
- **改动**: `deinitCore()` 末尾添加 `s_coreInitialized[static_cast<int>(m_coreType)] = false;`
- **风险**: 无。这是 bug 修复，不改变正常路径行为。
- **测试**: 对每个核心类型，故意加载不存在的 ROM，然后加载正确 ROM，确认不崩溃。

---

### Fix B: 移除失败路径中的 `deinitCore()` 调用

- **根因**: 与 Fix A 相关联。`_loadRom()` 失败时不应调用 `deinitCore()`，因为核心的 `retro_init()` 状态仍然有效，下次重试可直接复用。
- **修改文件** (4 个，每文件 3 处):
  - `src/game/mgba/GameRun.cpp:149,157,165` — 移除 `m_core.deinitCore();`
  - `src/emulator/CoreGenesis.cpp` — `_loadRom()` 失败分支
  - `src/emulator/CoreFceumm.cpp` — `_loadRom()` 失败分支
  - `src/emulator/CoreSnes9x.cpp` — `_loadRom()` 失败分支
- **隔离性**: 各核心包装类独立修改，不涉及交叉依赖。

---

### Fix C: `s_audioSampleBatchCallback` 返回 0 而非 `frames`

- **根因**: `LibretroLoader.cpp:905-912` — 当 `!s_current` 或 `!data` 时返回 `0`。libretro 规范中返回 `0` 意味着"未消费任何采样"，核心会重试同一批数据。
- **修改文件**: `src/game/retro/LibretroLoader.cpp`，`s_audioSampleBatchCallback`
- **改动**: 将 `return 0;` 改为 `return frames;`
- **风险**: 无。当 `s_current` 为 null 时丢弃音频是安全行为。

---

### Fix D: `reset()` 移至 SRAM 加载之前

- **根因**: 所有 4 个核心包装类的 `SetupGame()` 流程为 `_loadSram()` → `m_core.reset()`。`retro_reset()` 可能清零刚载入内存的 SRAM 区域。
- **修改文件** (4 个):
  - `src/game/mgba/GameRun.cpp`
  - `src/emulator/CoreGenesis.cpp`
  - `src/emulator/CoreFceumm.cpp`
  - `src/emulator/CoreSnes9x.cpp`
- **改动**: 将 `m_core.reset()` 移到 `_loadSram()` 之前。
- **正确顺序**: `_loadRom() → m_core.reset() → _loadSram() → _loadCheats() → m_ready = true`

---

### Fix E: CoreGenesis 在 Switch 平台跳过配置初始化

- **根因**: `CoreGenesis.cpp:34` — `#ifndef __SWITCH__` 包裹了整个 `_initConfig()` 调用，导致 Switch 上 `setConfigManager()` 和 `setSystemDirectory()` 也未执行。
- **修改文件**: `src/emulator/CoreGenesis.cpp`
- **改动**: 移除 `#ifndef __SWITCH__` / `#endif` 守卫。

---

## Phase 2 — LibretroLoader 鲁棒性增强（中风险）

### Fix F: 为 `s_current` 添加实例 ID 保护

- **修改文件**: `src/game/retro/LibretroLoader.hpp`, `src/game/retro/LibretroLoader.cpp`
- **设计**: 新增 `std::atomic<uint64_t> s_currentId` 和 `uint64_t m_instanceId`，在 `load()` 中设置，在回调中验证。

---

### Fix G: LibretroLoader 音频缓冲区大小上限

- **修改文件**: `src/game/retro/LibretroLoader.cpp`, `s_audioSampleBatchCallback`
- **改动**: 添加最大容量检查（8192 采样 ≈ 250ms），超过上限时丢弃最旧采样。

---

### Fix H: 修复头文件缩进异常

- **修改文件**: `src/emulator/CoreGenesis.hpp`, `CoreFceumm.hpp`, `CoreSnes9x.hpp`
- **改动**: 修正 `Fps()` 和 `SampleRate()` 的缩进。

---

## Phase 3 — 音频基础设施（需要测试资源，暂不执行）

### Fix I: 桌面平台音频重采样（暂缓）

### Fix J: PLL 音频同步对称化（暂缓）

---

## Phase 4 — 存档版本化（新功能，暂不执行）

### Fix K: 存档头添加核心元数据（暂缓）

---

## Phase 5 — 代码质量（重构）

### Fix L: 提取共享 SRAM/作弊码工具函数

- **新建**: `src/core/CoreUtils.hpp`, `src/core/CoreUtils.cpp`
- **修改**: `CoreGenesis.cpp`, `CoreFceumm.cpp`, `CoreSnes9x.cpp` 委托给工具函数

---

## 核心间隔离保证总结

| 共享点 | 修复后状态 | 隔离方式 |
|--------|-----------|---------|
| `s_current` | 全局指针 + 实例 ID 验证 | 原子 ID 比较，旧实例回调自动丢弃 |
| `s_coreInitialized` | 全局数组，deinit 正确重置 | 每次 deinit 后下次 init 可正常执行 |
| `AudioManager` | 单例，每次 init 重置 | deinit()/init() 完整重置所有状态 |
| ConfigManager | 键名前缀隔离 | core.mgba_* / core.picodrive_* 等前缀天然隔离 |
| 存档文件 | 按游戏文件名隔离 | 各核心独立管理 |
