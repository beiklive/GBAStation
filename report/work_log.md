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

---

## 2026-06-27 数据管理与按键映射拆分

### 任务分析

- **任务目标**:
  - 在 `DataManagementPage` 的“数据处理”标签页新增“清空游戏库”入口，并通过二次确认后清空 `GBAStation/data` 下的游戏库数据，但不删除 ROM 和存档。
  - 在 `GameLibraryPage` 的 X 键侧边栏中，为“多选”下方新增“全选”，进入多选后自动标记当前列表中的全部游戏，便于后续整体删除。
  - 在 `SettingPage` 中将原本共用的 `GBA/GBC/GB` 按键映射拆分为三套独立配置，其中 `GBA` 继续使用无前缀旧键，`GBC/GB` 使用新增前缀键。
- **输入输出**:
  - 输入为现有 `DataManagementPage`、`GameLibraryPage`、`SettingPage`、`GameView` 与输入映射默认值注册逻辑。
  - 输出为新的 UI 入口、确认流程、选择逻辑以及可被运行时正确读取的独立映射配置。
- **可能挑战**:
  - 游戏库页的“多选删除”逻辑已经建立，新增“全选”时需要复用现有删除通路，同时兼容分页加载和已过滤列表。
  - `GBA/GBC/GB` 之前共用无前缀配置，拆分后如果只改设置页而不改运行时读取逻辑，会出现界面可配但实际不生效的问题。
  - 已有用户可能已使用无前缀键自定义了 GBC/GB 映射，拆分后需要尽量兼容旧配置，避免升级后表现为“丢映射”。
- **解决方案**:
  - 为网格视图补充“全选删除标记”能力，由 `GameLibraryPage` 在进入多选时统一调用，删除逻辑继续沿用现有 `deleteSelection` 流程。
  - 为 GBC/GB 新增前缀，并同步修改设置页入口、默认值注册以及 `GameView` 的读取逻辑。
  - 为 GBC/GB 的读取增加对旧无前缀键的回退，这样旧配置可继续生效，用户修改后再逐步写入独立前缀键。

### 实现结果

- **数据管理页**:
  - 在“数据处理”标签页新增“清空游戏库”按钮。
  - 新增两级确认对话框，确认后调用 `GameDB->clearAll()` 并清理 `GBAStation/data` 目录下全部条目。
  - 增加提示文案，明确说明不会删除游戏文件和存档。
- **游戏库页**:
  - 在单游戏 X 键侧边栏的“多选”下方新增“全选”。
  - “全选”进入多选模式后会一次性标记当前列表中的全部游戏，随后再次按 X 即可直接进入批量删除流程。
  - 为 `RecyclingGrid` 新增全量删除选择接口，复用原有多选删除逻辑。
- **设置页与输入映射**:
  - 将原先“映射GBA/GBC/GB游戏”拆分为“映射GBA游戏 / 映射GBC游戏 / 映射GB游戏”三个入口。
  - 运行时平台前缀改为：`GBA=""`、`GBC="gbc."`、`GB="gb."`。
  - 默认值注册中为 `GBC/GB` 首次创建独立键时继承旧的无前缀映射，避免已有用户升级后丢失原先共用映射。

### 验证记录

- **单文件编译通过**:
  - `src/ui/page/DataManagementPage.cpp.o`
  - `src/ui/page/GameLibraryPage.cpp.o`
  - `src/ui/page/SettingPage.cpp.o`
  - `src/ui/view/RecyclingGrid.cpp.o`
- **受现有问题影响未完成的完整验证**:
  - `src/core/common.cpp.o` 编译被现有代码 `src/core/common.cpp:895` 的 `__int128` 输出歧义错误阻断。
  - `cmake --build build_macos` 继续会在第三方 `fceumm` 链接阶段因 `ld: unknown options: --whole-archive --no-whole-archive` 失败。

---

## RetroArch 渲染链对比分析：多通道滤镜左下角拉伸

### 任务目标

对比 `third_party/RetroArch-1.22.2/gfx` 与 `src/game/render` 的渲染器实现，定位以下多通道 GLSLP 滤镜最终输出画面向左下角拉伸的原因：

- `build_switch/GBAStation/shaders/shaders_glsl/phosphor-dot v3.3/2x-Scanline+ScaleFX.glslp`
- `build_switch/GBAStation/shaders/shaders_glsl/phosphor-dot v3.3/CRT+ScaleFX.glslp`
- `build_switch/GBAStation/shaders/hqx/hq4x.glslp`
- `build_switch/GBAStation/shaders/scalefx/scalefx.glslp`

### 输入与输出

- 输入：
  - RetroArch `gfx/drivers_shader/shader_glsl.c` / `shader_gl3.cpp`
  - 本项目 `src/game/render/RetroShaderPipeline.cpp`
  - 本项目 `src/game/render/FullscreenQuad.cpp`
  - 本项目 `src/game/render/ShaderCompiler.cpp`
  - 相关 `.glslp/.glsl` 预设与 pass 文件
- 输出：
  - 渲染链差异说明
  - 问题根因定位
  - 后续修复方向

### 关键差异

#### 1. 本项目中间 FBO 强制扩展为 2 的幂纹理

`src/game/render/RetroShaderPipeline.cpp:290-350`

- 本项目 `allocateFBO()` 会把 pass 输出 `imageWidth/imageHeight` 扩展为 `nextPowerOfTwo()` 后的 `texture width/height`
- 例如 240x160 经 3x 放大后有效画面可能是 720x480，但实际纹理会分配成 1024x512
- 这样有效画面只占中间纹理左下区域，剩余区域是 padding

RetroArch 本身允许 `TextureSize != InputSize`，但它会同步给每个采样器传递匹配的纹理坐标，不会直接拿同一套 UV 去采所有纹理。

#### 2. 本项目只给所有纹理共用主 `TexCoord`

`src/game/render/ShaderCompiler.cpp:203-206`
`src/game/render/FullscreenQuad.cpp:60-72, 147-152`
`src/game/render/RetroShaderPipeline.cpp:905-907`

- 本项目固定只绑定了：
  - `VertexCoord`
  - `COLOR`
  - `TexCoord`
- 每次绘制只调用：
  - `m_quad.draw(uvMax(currentImageW, currentTexW), uvMax(currentImageH, currentTexH));`
- 这意味着当前 pass 的主输入纹理会收到裁剪后的 `TexCoord`
- 但所有额外纹理，包括：
  - `OrigTexture`
  - `Pass1Texture/PassPrevNTexture`
  - `FeedbackTexture`
  - `PrevTexture`
  都没有独立的 `OrigTexCoord/PassPrevNTexCoord/...`

#### 3. RetroArch 会为每个额外采样器单独传入匹配的 TexCoord attribute

`third_party/RetroArch-1.22.2/gfx/drivers_shader/shader_glsl.c:698-728`
`third_party/RetroArch-1.22.2/gfx/drivers_shader/shader_glsl.c:1431-1557`

RetroArch 不只传 `TextureSize/InputSize`，还会对以下对象分别查找并填充独立坐标：

- `OrigTexCoord`
- `FeedbackTexCoord`
- `Pass1TexCoord / PassPrevNTexCoord / <alias>TexCoord`
- `PrevTexCoord / Prev1TexCoord / ...`

也就是说，RetroArch 的每个采样器都会拿到“与自己实际纹理尺寸匹配”的坐标范围，而不是简单复用当前 pass 主输入的 `TexCoord`。

### 为什么这些滤镜更容易出问题

#### `scalefx.glslp`

`scalefx-pass4.glsl` 明确依赖：

- `PassPrev5Texture`
- `PassPrev5TextureSize`
- `PassPrev5InputSize`

该 pass 还会在 3x 子像素网格中重建输出。当前 pass 的主输入和 `PassPrev5Texture` 并不是同一尺寸语义：

- 主输入是前一 pass 的 POT 纹理裁剪坐标
- `PassPrev5Texture` 实际应按它自己的有效区域采样

但本项目没有给 `PassPrev5TexCoord` 独立坐标，shader 内只能复用 `vTexCoord`。  
当 `PassPrev5Texture` 的实际纹理尺寸大于有效画面尺寸时，采样只会命中左下角有效区域，最后再被放大到整个输出，看起来就是“最终画面向左下角缩成一块并拉伸”。

#### `hq4x.glslp`

`hqx-pass2.glsl` 依赖：

- `Texture`（pass1 输出）
- `OrigTexture`（原始输入）
- `LUT`

这个 pass 同时混用当前 pass 输入和 `OrigTexture`。  
本项目给了 `OrigTextureSize` / `OrigInputSize`，但没有给 `OrigTexCoord`。一旦主输入 pass1 使用了 POT 纹理裁剪 UV，而 `OrigTexture` 仍是原始非 POT 语义，两个采样器的坐标系就错位，最终混合区域会收缩到左下角。

#### `2x-Scanline+ScaleFX.glslp` / `CRT+ScaleFX.glslp`

这两套 preset 前半段都先走 `ScaleFX` 多 pass，后半段再叠加 CRT / phosphor dot pass。

其中：

- `ScaleFX` 部分已经引入了 `PassPrevNTexture` 采样
- `phosphor-dot.glsl` 又会同时使用
  - `TextureSize`
  - `InputSize`
  - `OrigInputSize`
  - `OutputSize`

特别是 `phosphor-dot.glsl` 中这句：

- `vec2 pix_co = vTexCoord * TextureSize.xy / InputSize.xy * OutputSize.xy;`

它默认假设当前传入的 `vTexCoord` 与当前 `TextureSize/InputSize` 语义严格匹配。  
但当前渲染链里，只有主输入拿到了裁剪过的 UV，其他引用的历史 pass / 原始纹理并没有独立坐标，结果就是上游已经错位，后续 CRT pass 再按照错误坐标做居中和放大，视觉上更明显地表现为左下角拉伸。

### 直接根因

问题不是单一的 `OutputSize` 或 `viewport` 计算错误，而是两个实现差异叠加：

1. 本项目将中间 pass FBO 扩展为 POT 纹理，导致 `TextureSize` 大于 `InputSize`
2. 本项目没有像 RetroArch 那样为 `OrigTexture`、`PassPrevNTexture`、`FeedbackTexture`、`PrevTexture` 等额外采样器提供各自独立的 `*TexCoord`

因此，多通道 shader 在引用非主输入纹理时，会错误复用当前主输入的 `TexCoord`。  
当这些被引用纹理存在 padding 区域时，shader 实际只采到左下角有效区域，最后 pass 再把这块区域拉伸到整个输出，于是出现“最终画面向左下角拉伸”。

### 次要差异

#### 本项目 viewport 只设到有效画面尺寸

`src/game/render/RetroShaderPipeline.cpp:657-665`

- FBO 实际分配为 POT 尺寸
- 但 `glViewport(0, 0, outW, outH)` 只绘制有效画面区域

这本身不是 bug，RetroArch 也允许纹理大于有效区域。  
真正的问题是：绘制只覆盖左下有效区域后，后续又没有为引用这些纹理的采样器传独立坐标。

### 结论

以下滤镜出现左下角拉伸的根因一致：

- `scalefx.glslp`
- `hq4x.glslp`
- `2x-Scanline+ScaleFX.glslp`
- `CRT+ScaleFX.glslp`

根因是本项目当前渲染链只兼容“单主输入纹理坐标”，没有完整实现 RetroArch 多通道 GLSL preset 所要求的“每个历史/别名/原始/反馈纹理单独 TexCoord attribute”语义；而中间 FBO 又使用了 POT padding，最终把这个差异放大成左下角缩块并被后续 pass 拉伸的现象。

### 修复方向

优先级最高的修复方向：

1. 参照 RetroArch，为 `OrigTexture`、`FeedbackTexture`、`PassNTexture`、`PassPrevNTexture`、`PrevTexture`、`<alias>Texture` 增加对应的 `*TexCoord` attribute 绑定
2. 绘制每个 pass 时，为不同来源纹理传各自 `uvMax(imageSize / textureSize)` 对应的顶点坐标
3. 保留当前 `TextureSize/InputSize` uniform 逻辑，因为这部分语义基本正确，问题主要出在坐标 attribute 缺失

备选方向：

- 若平台允许，也可以先去掉中间 FBO 的 POT 扩展，改成直接按 `imageWidth/imageHeight` 分配纹理，以降低问题暴露面
- 但这只是缓解，不是完整兼容方案；因为 RetroArch 规范本来就允许 `TextureSize != InputSize`

### 本次实现

本次仅修改 `src/game/render/RetroShaderPipeline.cpp`，未改动任何第三方代码。

已补齐的兼容点：

1. 为以下采样器来源补充独立 `*TexCoord` attribute 绑定：
   - `OrigTexture`
   - `FeedbackTexture`
   - `PassNTexture`
   - `PassPrevNTexture`
   - `PassPrevTexture`（社区 shader 常见无编号别名）
   - `<alias>Texture`
   - `PrevTexture / Prev1Texture / Prev2Texture ...`
   - `OriginalHistory0Texture / OriginalHistory1Texture ...`
   - `LUTTexCoord`
2. 为以下来源补充对应尺寸 uniform：
   - `PassNTextureSize / InputSize / Size`
   - `PassPrevNTextureSize / InputSize / Size`
   - `PassPrevTextureSize / InputSize / Size`
   - `OrigTextureSize / InputSize / Size`
   - `FeedbackTextureSize / InputSize / Size`
   - `PassFeedbackTextureSize / InputSize / Size`
   - `PrevTextureSize / InputSize / Size`
   - `Prev1TextureSize / InputSize / Size ...`
   - `OriginalHistory0TextureSize / InputSize / Size ...`
3. 将 `PrevTexture` 的绑定语义修正为“上一帧原始输入历史”，不再错误复用反馈纹理
4. 增加首帧回退：历史帧尚未建立时，`PrevTexture` / `OriginalHistory0Texture` 回退到当前输入纹理，避免首帧未绑定
5. 调整中间 pass FBO 分配策略：
   - 桌面 / GLES3 路径改为按 `imageWidth/imageHeight` 精确分配
   - 仅 `USE_GLES2` 保留 POT 分配兜底
   - 目的：与 RetroArch `gl3` 渲染链保持一致，避免 `ScaleFX + CRT/Scanline` 末端 pass 因 `TextureSize != InputSize` 的 padding 再次发生主输入拉伸

### 验证

已执行的验证：

- 运行 CMake 配置：成功
- 尝试整仓 macOS 构建：失败，但失败点位于 `third_party/melonDS/src/ARCodeFile.cpp` 的现有 `std::variant` 编译错误，与本次修改无关
- 单独编译 `src/game/render/RetroShaderPipeline.cpp` 对应目标：成功，仅有项目现有第三方头文件告警，无本次改动引入的编译错误
- 在精确尺寸 FBO 调整后再次单独编译 `src/game/render/RetroShaderPipeline.cpp`：成功

### 当前结论

本次修改已经把导致多通道滤镜“右下角拉伸并超出窗口”的主要兼容缺口补上：

- 不再让 `OrigTexture`、`PassPrevNTexture`、`PrevTexture` 等错误复用主输入 `TexCoord`
- 不再缺失 `Prev*` / `PassPrev*` 的尺寸 uniform

后续若仍有个别滤镜异常，需要继续排查的优先方向：

1. 某些 shader 是否还依赖更细的 RetroArch 约定（如外部 LUT 的特殊坐标语义）
2. 中间历史纹理在前几帧的初始化策略是否需要进一步与 RetroArch 对齐

## 2026-06-27 phosphor-line 反射框修复补充

### 新问题

用户反馈以下 phosphor-line 预设仍然显示异常：

- `build_switch/GBAStation/shaders/shaders_glsl/phosphor-line v2.0/F10_PhosphorLineReflex.glslp`
- `build_switch/GBAStation/shaders/shaders_glsl/phosphor-line v2.0/F00_PhosphorLineReflex(Base).glslp`

现象是：

- 预期应该是游戏画面缩小，并在四周形成完整的反射/边框效果
- 实际上反射框显示不完整，很多效果与 RetroArch 不一致

### 这轮对比结论

继续对照 `third_party/RetroArch-1.22.2/gfx/drivers/gl3.c` 与 `shader_glsl.c` 后，确认本项目还有两处语义差异：

1. `GLSLPParser` 之前错误地把“最后一个未显式声明 scale 的 pass”自动补成了 `viewport x 1.0`
2. RetroArch 的真实行为不是这样：
   - 如果最后一个 pass 没有显式 scale，它会作为“最终直接上屏 pass”执行
   - 不会先落到中间 FBO，再额外做一次直通 blit

这会直接影响 `PP-reflex.glsl` 这类依赖最终 `OutputSize`、`FinalViewportSize` 和最终屏幕语义的后处理 pass。

此外，本项目原先在着色器管线输出到屏幕时，还会强制把最终纹理改成 `GL_NEAREST` 再直绘，这与 RetroArch 的最终输出路径也不一致，会放大某些边框/反射 shader 的误差。

### 本次修改

本次仍然只修改 `src/game/render/`，没有改动任何第三方代码。

已完成的修复：

1. 移除 `GLSLPParser` 对“最后一个无显式 scale 的 pass 自动补成 viewport 缩放”的错误兼容逻辑
2. 在 `RetroShaderPipeline` 中新增“最终直接上屏 pass”分支：
   - 中间 pass 继续输出到 FBO
   - 最后一个未显式声明 scale 的 pass 改为在 `drawToScreen()` 阶段直接绘制到目标屏幕矩形
3. 为直接上屏 pass 补齐与中间 pass 一致的资源绑定语义：
   - `Texture`
   - `OrigTexture`
   - `PassNTexture`
   - `PassPrevNTexture`
   - `FeedbackTexture`
   - `PrevTexture / OriginalHistoryN`
   - 对应 `*TexCoord`
   - 对应 `*TextureSize / *InputSize / *Size`
4. 为 `FullscreenQuad` 增加“主输入自定义 UV”绘制接口，避免最终直接上屏 pass 退化成只能使用固定 `0..1` 坐标
5. 在 `RenderChain::drawToScreen()` 接入这条新分支：
   - 若当前 preset 存在待执行的最终 screen pass，则直接让 `RetroShaderPipeline` 负责最终着色
   - 否则沿用原有 `DirectQuadRenderer`
6. 移除原先“只要加载 shader 就强制把最终纹理改成最近邻采样再上屏”的处理，避免额外破坏 phosphor-line / reflex 类 shader 的最终效果

### 影响判断

这轮修复主要针对两类 preset：

1. 最后一个 pass 就是最终屏幕效果，但 `.glslp` 没写 `scale_type` 的预设
   - 典型就是 `F10_PhosphorLineReflex.glslp`
2. 最终效果强依赖屏幕输出语义、且对最终采样方式较敏感的预设
   - `F00_PhosphorLineReflex(Base).glslp` 也属于这类，需要避免额外直绘链路改变它的最终表现

### 编译验证

已完成最小编译验证，以下目标均通过：

- `src/game/render/RetroShaderPipeline.cpp.o`
- `src/game/render/RenderChain.cpp.o`
- `src/game/render/FullscreenQuad.cpp.o`
- `src/game/render/GameRenderer.cpp.o`

仅有项目现存第三方头文件 warning，无本次改动引入的新编译错误。

## 2026-06-27 screen pass UV 顺序修正

### 用户反馈

在上一轮“最终直接上屏 pass”改造后，用户继续反馈两类问题：

1. `CRT+ScaleFX.glslp`、`2x-Scanline+ScaleFX.glslp` 这类滤镜出现游戏画面上下颠倒
2. `F10_PhosphorLineReflex.glslp` / `F00_PhosphorLineReflex(Base).glslp` 在游戏画面与反射边框连接处，本应存在的黑色过渡遮罩仍然不正确

### 新定位

继续检查 `RenderChain`、`DirectQuadRenderer`、`FullscreenQuad` 和 `RetroShaderPipeline::drawScreenPass()` 后，确认还有一个坐标顺序问题：

1. `DirectQuadRenderer` 接收的 UV 顺序是：
   - 左上、右上、右下、左下
2. `FullscreenQuad` 内部顶点顺序是：
   - 左下、右下、右上、左上
3. 上一轮把“最终直接上屏 pass”改接到 `FullscreenQuad` 时，直接复用了 `RenderChain` 传入的屏幕 UV
4. 结果就是：
   - 对某些 implicit final pass 预设，最终画面被垂直翻转
   - `PP-reflex.glsl` 这类依赖 `TEX0`、屏幕中心和边界关系的 shader，边框连接处遮罩逻辑也会被带偏

### 本次修复

在 `src/game/render/RetroShaderPipeline.cpp` 中新增 UV 顺序转换：

1. 保留 `RenderChain` 对外的 UV 约定不变
2. 在 `drawScreenPass()` 内部，将“左上起始”的屏幕 UV 转为 `FullscreenQuad` 需要的“左下起始”顺序
3. 再把它送入最终 screen pass 的主 `TexCoord`

这次改动很小，但会同时影响：

- `CRT+ScaleFX.glslp`
- `2x-Scanline+ScaleFX.glslp`
- `F10_PhosphorLineReflex.glslp`
- `F00_PhosphorLineReflex(Base).glslp`

### 编译验证

再次完成最小编译验证：

- `src/game/render/RetroShaderPipeline.cpp.o`

通过，仍然只有项目现存第三方 warning。

---

## 2026-06-27 游戏库删除确认与截图热键

### 任务分析

- **任务目标**:
  - 游戏库 X 键侧边栏的多选删除、全选删除改为两段确认：先确认是否从游戏库移除，再确认是否删除 ROM 文件。
  - 为每个机型补上截图热键，截图以 `screenshot_时间戳.png` 保存到存档目录。
- **解决方案**:
  - 多选/全选共用同一个删除入口，先复制当前选择集合，避免弹窗期间选择状态变化。
  - 第二段确认中选择“否”时只删除数据库记录，不删除 ROM 文件；选择“是”时同时删除 ROM 文件。
  - 将“截图”加入各机型热键默认表，注册到 `GameView` 输入处理，通过 `GameSignal` 交给游戏线程保存 PNG。

### 实现结果

- **游戏库删除**:
  - 删除已选游戏时先弹出“是否从游戏库移除这 N 款游戏？”。
  - 选择“否”会取消删除流程。
  - 选择“是”后继续询问“是否删除 ROM 文件？”。
  - 第二步选择“否”时仅移除库记录，选择“是”时移除库记录并删除 ROM 文件。
- **截图热键**:
  - `GBA/GBC/GB/NES/SFC/NDS` 均会生成独立截图热键配置项。
  - 热键触发后在游戏线程读取当前视频帧并保存为 PNG。
  - 截图保存到 `GameEntry.savePath`，为空时回退到全局存档目录。
  - 同一秒内重复截图会追加数字后缀，避免覆盖已有文件。
- **顺手修正**:
  - 修复 `GetNdsIconCachePath()` 在 macOS libc++ 下将 `file_time_type` 的 `__int128` 计数直接写入 stream 导致的编译歧义。

### 编译验证

- `src/ui/page/GameLibraryPage.o`
- `src/ui/view/GameView.o`
- `src/core/common.o`
- `src/game/control/GameInputManager.o`

以上均编译通过，仅有项目现存第三方/unused warning。

### 追加修正

用户反馈：StartPage 的删除功能也需要二次确认。

本次同步调整主页最近游戏卡片的删除入口：

- 第一步提示“是否从游戏库移除该游戏？”。
- 选择“否”时取消删除流程。
- 选择“是”后继续提示“是否删除 ROM 文件？”。
- 第二步选择“否”时只移除数据库记录，不删除 ROM 文件。
- 第二步选择“是”时移除数据库记录并删除 ROM 文件。
- 同步更新 `resources/changelog` 中 `v0.2.2` 的说明，覆盖主页和游戏库两个入口。

已编译验证：

- `src/ui/page/StartPage.o`

通过，仅有项目现存 warning。

### 追加修正 2

用户继续反馈：游戏库的直接删除也需要二次确认，并且所有二次确认弹窗都需要新增“不删了”按钮。

本次继续统一删除流程：

- 游戏库单个“删除游戏”改为两段确认，支持只移除库记录或同时删除 ROM 文件。
- StartPage 单个删除、游戏库单个删除、游戏库多选/全选删除的两段弹窗均增加“不删了”按钮。
- “不删了”只取消当前删除流程，不移除数据库记录，也不删除 ROM 文件。
- 同步更新 `resources/changelog` 的 `v0.2.2` 说明。

---

## 2026-06-27 LPL 导入类型校验

### 任务分析

- **任务目标**:
  - 修改“数据管理 -> 整合包导入”中的 LPL 导入逻辑。
  - 当用户选择的 LPL 文件内游戏类型与当前按钮对应的平台类型不一致时，立即中断导入并弹出“选择错误”提示。
- **输入输出**:
  - 输入为 `DataManagementPage::startImport()` 中现有的 LPL 解析结果 `lplJson["items"]`。
  - 输出为导入前的平台一致性校验，以及错误提示对话框。
- **可能挑战**:
  - LPL 本身没有单独的强类型字段可直接复用，平台识别需要依赖条目路径中的 ROM 扩展名。
  - 需要把校验插在真正启动导入线程之前，避免选错后仍然出现部分导入。
- **解决方案**:
  - 在 `items` 解析成功后，遍历每个条目的 `path` 扩展名并映射为平台类型。
  - 只要发现任一可识别平台与当前按钮传入的平台不同，就直接停止流程并弹出错误对话框，不进入导入线程。

### 实现结果

- 在 `DataManagementPage.cpp` 中新增 LPL 条目平台校验辅助函数。
- 在 `startImport()` 中加入导入前检查：
  - 若 LPL 内条目类型和当前按钮平台不一致，立即隐藏进度层并弹出“选择错误”对话框。
  - 若一致，则保持原有导入流程不变。

### 编译验证

- `src/ui/page/DataManagementPage.cpp.o`
  - 通过，仅有项目现存第三方 warning。

## 2026-06-27 反射类滤镜参数归属修正

### 用户反馈

用户继续反馈 `F10_PhosphorLineReflex.glslp` / `F00_PhosphorLineReflex(Base).glslp` 仍然显示异常：

1. 游戏画面没有按预期缩小
2. 反射边框左右两侧跑出可视范围
3. 游戏画面与反射边框连接处缺少黑色过渡

### 新定位

继续对照 RetroArch `gl3` 最终 pass 坐标后，确认当前最终 pass 的主 UV 方向不应再加 `PP-reflex.glsl` 特例；`PP-reflex.glsl` 期望屏幕顶部对应 `TexCoord.y = 0`，当前统一转换后的方向与 RetroArch 行为更接近。

随后转向参数链路检查，发现项目只按“参数数量与名称”判断是否应用旧参数。`F10/F00` 这类 phosphor-line 预设拥有大量同名参数，但不同预设会覆盖不同默认值，例如：

- `F10_PhosphorLineReflex.glslp`: `PIC_SCALE_X = 0.9`、`PIC_SCALE_Y = 0.9`
- `F00_PhosphorLineReflex(Base).glslp`: `PIC_SCALE_X = 0.88`、`PIC_SCALE_Y = 0.88`

如果之前保存过同一组参数名但值为 `1.0` 的配置，启动或切换滤镜时仍会被应用，导致 `PP-reflex.glsl` 里的缩小和边框过渡计算被覆盖。

### 本次修复

为游戏条目新增 `shaderParaPath`，让保存的着色器参数明确归属于某个预设路径：

1. `GameEntry` 增加 `shaderParaPath`
2. 数据库序列化和反序列化保存该字段
3. 远程 API 可编辑字段补充 `shaderParaPath`
4. `GameView::_applySavedShaderParams()` 仅在 `shaderParaPath == shaderPath` 时应用保存参数
5. 切换着色器路径时清空旧参数和旧参数归属
6. 着色器参数面板重建时，如果路径不匹配，使用当前预设的 `#pragma` / `.glslp` 默认值重新初始化

这样旧数据库里没有 `shaderParaPath` 的历史参数会被自动跳过，避免继续污染当前滤镜；之后用户手动调整同一个滤镜的参数仍会随路径一起保存。

### 编译验证

完成最小编译验证：

- `src/ui/view/GameView.cpp.o`
- `src/ui/view/GameMenuView.cpp.o`
- `src/core/game_database.cpp.o`
- `src/network/ApiRouter.cpp.o`
- `src/game/render/RetroShaderPipeline.cpp.o`

均通过，仅有项目现存第三方 warning。

## 2026-06-27 最终 pass 额外纹理坐标原点修正

### 用户反馈

用户确认 `F10_PhosphorLineReflex.glslp` / `F00_PhosphorLineReflex(Base).glslp` 的反射框已经恢复正常，但游戏画面与反射框连接处的处理仍然不对，预期应有黑色过渡遮住上下和左右对边像素。

### 新定位

继续对照 RetroArch `gl3` 的最终 pass 坐标后，发现当前还有一个细节不一致：

1. 最终 screen pass 的主 `TexCoord` 已经转换成“屏幕顶部 `v=0`”
2. 但 `PassN/PassPrevN/Orig/Feedback/LUT` 这些额外 `TexCoord` 属性仍沿用中间 FBO pass 的默认方向
3. RetroArch 最终 pass 会把这些坐标与最终屏幕顶点一起提交，因此它们也需要和主 `TexCoord` 保持同一个顶部原点

这个问题对普通 blit 类滤镜不明显，但会影响依赖多路纹理混合、边缘过渡或历史 pass 坐标的后处理滤镜。

### 本次修复

在 `src/game/render/RetroShaderPipeline.cpp` 中调整：

1. `makeUvCoords()` 增加 `topOrigin` 参数
2. `addTexCoordAttribIfUsed()` 增加 `topOrigin` 参数
3. 中间 FBO pass 保持原来的 FBO 坐标方向
4. 最终 screen pass 绑定的所有额外纹理坐标统一使用顶部原点方向

这样最终 pass 内的主纹理坐标和额外纹理坐标不再一个顶部原点、一个底部原点，减少连接处混合时采到对边像素的机会。

### 编译验证

完成最小编译验证：

- `src/game/render/RetroShaderPipeline.cpp.o`

通过，仅有项目现存第三方 warning。

## 2026-06-27 收窄 FBO padding 作用范围

### 用户反馈

上一轮恢复 RetroArch 风格的全局 POT FBO 后，用户反馈 `CRT+ScaleFX.glslp`、`Scanline+ScaleFX.glslp` 这类 ScaleFX 滤镜又出现游戏画面向右下拉伸并超出画面范围的问题。

### 新定位

这说明黑色 padding 不能全局启用：

1. `PP-reflex.glsl` 需要 RetroArch 风格的 FBO padding 来避免反射连接处采到对边像素
2. ScaleFX / hqx 这类滤镜对中间 pass 的有效纹理尺寸更敏感，全局 POT FBO 会重新引入右下方向拉伸
3. 因此需要按预设能力收窄，而不是全局模拟 RetroArch 行为

### 本次修复

1. `GLSLPParser` 默认 `wrap_mode` 恢复为 `clamp_to_edge`
2. `ShaderPassDesc` 增加 `hasExplicitWrap`，记录预设是否显式写了 `wrap_mode`
3. `RetroShaderPipeline` 增加 `m_usePaddedFBO`
4. 仅当预设通道中包含 `PP-reflex.glsl` 时：
   - FBO 纹理扩展到 2 的幂尺寸，提供黑色 padding
   - 未显式声明 `wrap_mode` 的 pass 自动使用 `clamp_to_border`
5. 普通 ScaleFX / hqx / CRT+ScaleFX 等预设继续使用精确 FBO 尺寸，避免右下拉伸回归

### 编译验证

完成最小编译验证：

- `src/game/render/RetroShaderPipeline.cpp.o`
- `src/game/render/GLSLPParser.cpp.o`

通过，仅有项目现存第三方 warning。

## 2026-06-27 恢复 RetroArch FBO 黑色 padding

### 用户反馈

用户确认上一轮后反射框仍然正常，但游戏画面和反射框之间依然没有预期的黑色连接/过渡区域。

### 新定位

继续对照 RetroArch `gl3` renderchain 后，发现更关键的差异：

1. RetroArch 为每个 FBO 分配的纹理尺寸是 `next_pow2(img_width/img_height)`
2. 有效画面只绘制在 `img_width/img_height` 区域内
3. 超出有效画面但仍在纹理内的区域会因为 `glClear()` 保持黑色
4. `PP-reflex.glsl` 中大量使用 `fract()` 做反射采样，预期会在部分边界采到这圈黑色 padding，从而遮住上下和左右对边像素
5. 项目之前为了修复拉伸问题把 FBO 改成精确尺寸，导致 `InputSize == TextureSize`，黑色 padding 消失，`fract()` 更容易直接卷到对边像素

同时发现 `.glslp` 默认 `wrap_mode` 也与 RetroArch 不一致：

- RetroArch 默认：`clamp_to_border`
- 项目旧默认：`clamp_to_edge`

这会进一步增加边缘像素被钳住/重复的概率。

### 本次修复

1. `RetroShaderPipeline::allocateFBO()` 恢复 RetroArch 风格：所有平台都将 FBO 纹理尺寸扩展到 2 的幂
2. 保留 `imageWidth/imageHeight` 为有效画面尺寸，`width/height` 为真实纹理尺寸
3. 继续通过 `InputSize/TextureSize` 和 `uvMax()` 限制有效区域，避免恢复 POT 后再次出现拉伸
4. `GLSLPParser` 默认 `wrapMode` 改为 `ClampToBorder`
5. 外部纹理默认 wrap 也同步改为 `ClampToBorder`

### 编译验证

完成最小编译验证：

- `src/game/render/RetroShaderPipeline.cpp.o`
- `src/game/render/GLSLPParser.cpp.o`

通过，仅有项目现存第三方 warning。

### 追加修正

用户随后反馈：`F10_PhosphorLineReflex.glslp` 的反射框跑到显示范围外，比上一版更差。

继续确认后发现：

1. 普通最终 pass 需要把 `RenderChain` 的左上起始 UV 转成 `FullscreenQuad` 的左下起始 UV，才能避免上下颠倒
2. 但 `PP-reflex.glsl` 不是普通 blit pass，它直接使用 `TEX0` 计算屏幕空间中的缩放画面、边框与反射区域
3. 因此对 `PP-reflex.glsl` 做同样的 UV 翻转，会把它的屏幕坐标系翻乱，导致反射框位置跑出显示范围

本次在 `RetroShaderPipeline::drawScreenPass()` 中加入更窄的判断：

- 普通最终 screen pass：继续执行 UV 顺序转换
- `PP-reflex.glsl`：保持 shader 空间的 `TexCoord` 方向，不做最终直绘式翻转

已再次编译验证：

- `src/game/render/RetroShaderPipeline.cpp.o`

通过，仍然只有项目现存第三方 warning。

### Switch 发布工作流与更新说明拉取调整

- `build-switch.yml` 调整为新的发布方式：
  - 仍在 tag 触发后编译 `build_switch/GBAStation.nro`
  - 将 `GBAStation.nro` 压缩为 `GBAStation-版本号.zip`
  - 通过 `softprops/action-gh-release` 上传到当前项目的 GitHub Release
  - 从 tag 注释中读取更新说明，并保存为 `版本号.txt`，例如 `0.2.0.txt`
  - 将 `版本号.txt` 提交到 `GBAStation_Release` 仓库根目录
  - 继续自动更新 `GBAStation_Release` 仓库中的 `README.md` 更新说明部分
  - CDN 刷新目标改为新生成的 `版本号.txt` 和 `README.md`
- 程序侧更新说明拉取逻辑调整：
  - 版本检查仍然先从 `version.ini` 读取 `GBAStation` 对应版本号
  - 当判断存在新版本时，再根据该版本号请求
    `https://cdn.jsdelivr.net/gh/beiklive/GBAStation_Release@main/版本号.txt`
  - 读取成功后将 txt 内容作为更新弹窗中的更新说明
  - 若远程 txt 获取失败，则回退为一条简短提示文案
  - 对版本号额外做了前缀兼容处理：若带 `v/V`，会先去掉再拼接 txt 文件名

### 本次验证

- 单独重新编译 `src/core/AppUpdater.cpp.o`
- 编译通过，仅有项目现存 warning

### 更新包目录结构调整

- 发布包结构调整为：
  - `GBAStation.zip`
  - zip 内部包含 `switch/GBAStation.nro`
- `build-switch.yml` 已同步修改：
  - 打包前先创建 `switch/` 目录
  - 将编译产物复制到 `switch/GBAStation.nro`
  - 再将整个 `switch` 目录压缩进 zip
- 程序侧解压逻辑已同步修改：
  - `AppUpdater::extractNroFromZip()` 现在优先查找 `switch/GBAStation.nro`
  - 若遇到旧包格式，仍兼容回退到任意路径下的 `GBAStation.nro`

### 本次验证补充

- 单独重新编译 `src/core/AppUpdater.cpp.o`
- 编译通过，仅有项目现存 warning

### 更新流程梳理

- 入口分为两处：
  - `src/main.cpp` 启动后 2 秒会在后台线程自动检查更新，受 `SettingKey::KEY_EMU_UPDATE` 控制。
  - `src/ui/page/AboutPage.cpp` 的“检查更新”按钮会手动触发检查。
- 本地版本号来源：
  - 优先读取 `config/version.json` 中的 `version`
  - 若不存在则回退到编译时的 `APP_VERSION`
- 远程元数据流程：
  - `AppUpdater::checkSync()` 从 `https://cdn.jsdelivr.net/gh/beiklive/GBAStation_Release@main/version.json` 拉取版本信息
  - 解析 `version`、`changelog`、`size`、`download`
  - 若发现新版本，会把远程 `version.json` 写入 `cache/version.json`
- 自动检查行为：
  - 仅在发现新版本时弹出通知
  - 当前不会直接进入更新页，而是提示用户前往“关于”页面更新
- 手动检查行为：
  - 检测中先显示 borealis dialog
  - 有更新时弹出 `UpdateDialog` 展示版本号和更新日志
  - 用户确认后打开 `UpdatePage` 开始下载
  - 无更新时弹出“已是最新版本”
- 下载流程：
  - `AppUpdater::download()` 使用 libcurl 下载更新包到内存
  - 之后写入 `cache/update.zip`
  - 再从 zip 中解压出 `GBAStation.nro` 到 `cache/update.nro`
  - `UpdatePage` 负责显示百分比、速度、已下载大小和剩余时间
- 安装流程：
  - `AppUpdater::install()` 会先把 `cache/version.json` 写回 `config/version.json`
  - Switch 平台仅校验 `cache/update.nro` 是否存在，成功后由 `finishInstall()` 真正替换 `sdmc:/switch/GBAStation.nro`
  - 非 Switch 平台不会自动替换程序本体，只会提示用户手动替换
- 当前实现注意点：
  - `AppUpdater::abort()` 目前没有接入 `download()` 的 `ProgressCtx.cancelled`
  - 取消下载主要依赖 `UpdatePage` 的进度回调返回 `false`
  - 远程检查失败时，`checkSync()` 会尝试用本地 `config/version.json` 与当前版本比较，作为一种回退判定

### 更新源切换

- 按新需求重做了更新流程：
  - 版本检查改为读取 `https://download.nswiki.cn/hahappify/xlcj/version.ini`
  - 从 ini 中提取 `GBAStation=` 对应版本号
  - 以 `APP_VERSION` 作为本地版本基准进行比较
  - 更新包下载地址固定为 `https://download.nswiki.cn/hahappify/xlcj/nro/GBAStation.zip`
- `AppUpdater` 侧清理：
  - 删除旧的 `version.json` 远程解析、缓存和本地回退逻辑
  - 删除安装前写回 `config/version.json` 的旧流程
  - 保留 zip 下载后解压 `GBAStation.nro` 到缓存，再由 Switch 平台执行替换
  - 将 `abort()` 真正接入下载取消流程
- 页面侧同步调整：
  - `AboutPage` 的当前版本信息改为直接展示 `APP_VERSION`
  - 更新页提示来源改为 `download.nswiki.cn`
  - 自动检查和手动检查统一直接调用新版 `checkSync()`
  - `main.cpp` 中移除同步检查后额外等待 15 秒的陈旧轮询
- 验证情况：
  - 单独重新编译 `src/core/AppUpdater.cpp.o`
  - 单独重新编译 `src/ui/page/AboutPage.cpp.o`
  - 单独重新编译 `src/ui/page/UpdatePage.cpp.o`
  - 单独重新编译 `src/main.cpp.o`
  - 以上均通过
  - 整包 `cmake --build build_macos --target GBAStation -j4` 仍被项目现存第三方目标失败阻断，主要来自 `third_party/melonDS` 与 `third_party/nestopia`，与本次修改无关

---

## 2026-06-27 显示页默认遮罩与着色器调整

### 任务分析

- **任务目标**:
  - 在 `SettingPage` 的显示子页面中，为“遮罩设置”和“着色器设置”补上 `NDS` 默认项。
  - 移除“启用遮罩”和“启用着色器”两个总开关。
  - 在两个 header 下增加提示文案：以下设置会在导入新游戏时自动套用。
- **输入输出**:
  - 输入为 `SettingPage` 的显示页 UI，以及新游戏导入/首次入库时的默认显示配置初始化逻辑。
  - 输出为新的设置页结构，以及和“自动套用”提示一致的默认启用行为。
- **可能挑战**:
  - 现有总开关不仅用于设置页显示，还会影响 `DataManagementPage` 导入新游戏时的 `overlayEnabled/shaderEnabled` 初始值。
  - 若只删除 UI 而不调整初始化逻辑，新游戏仍可能默认关闭，与“自动套用”提示冲突。
- **解决方案**:
  - 设置页中直接移除两个总开关，补上 `NDS` 项和提示文案。
  - 新增共用辅助函数，根据平台默认遮罩/着色器路径是否已配置，决定新游戏是否自动启用对应效果。
  - 让导入流程和 `GamePage` 首次入库流程统一使用同一套自动启用逻辑。

### 实现结果

- **设置页 UI**:
  - `遮罩设置` 下移除了总开关，新增提示文案，并补上 `NDS 遮罩`。
  - `着色器设置` 下移除了总开关，新增提示文案，并补上 `NDS 着色器`。
- **自动套用逻辑**:
  - 新增 `shouldAutoEnableOverlayForPlatform()` 和 `shouldAutoEnableShaderForPlatform()` 共用辅助函数。
  - 导入新游戏时：
    - 若对应平台已配置默认遮罩路径，则新游戏自动启用遮罩。
    - 若对应平台已配置默认着色器路径，或存在全局回退着色器路径，则新游戏自动启用着色器。
  - 从文件浏览器首次打开并写入数据库的新游戏，也改为沿用同一套逻辑。

### 编译验证

- `src/core/Tools.cpp.o`
- `src/ui/page/SettingPage.cpp.o`
- `src/ui/page/DataManagementPage.cpp.o`
- `src/ui/page/GamePage.cpp.o`

以上均编译通过，仅有项目现存 warning。

### 追加修正回收

用户继续反馈：上一条 `PP-reflex.glsl` 特例后，`F10_PhosphorLineReflex.glslp` 出现主体不缩小、上下颠倒、两侧反射框跑出可视范围的问题。

重新对照 RetroArch `gl3` 最终 pass 的坐标后，确认上一条判断过窄：

1. RetroArch 最终 pass 的屏幕左上角对应 `TexCoord = (0, 0)`
2. `PP-reflex.glsl` 虽然使用 `TEX0` 计算屏幕空间边框，但它仍然期待这个“左上为 0,0”的最终 pass 坐标
3. 因此不能给 `PP-reflex.glsl` 跳过 UV 顺序转换

本次回收上一条特例：

- 所有最终 screen pass 统一将 `RenderChain` 的左上起始 UV 转换成 `FullscreenQuad` 的顶点顺序
- `PP-reflex.glsl` 不再单独跳过这一步

已再次编译验证：

- `src/game/render/RetroShaderPipeline.cpp.o`

通过，仍然只有项目现存第三方 warning。
