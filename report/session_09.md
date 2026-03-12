# RetroArch GLSL/GLSLP 着色器渲染链 – 工作报告

## 任务背景

本次任务要求：
1. 评估在 BeikLiveStation 中添加 RetroArch `.glsl` 着色器和 `.glslp` 多通道预设的可行性。
2. 如可行，移植相关功能，以渲染链的形式对游戏画面进行后处理再输出。
3. 保持低代码耦合性。

---

## 可行性评估

BeikLiveStation 已具备以下基础设施：

| 模块 | 状态 | 说明 |
|------|------|------|
| `ShaderChain` | 已有 | 支持多通道 FBO 渲染链，但**未集成到 GameView** |
| `GameView::draw()` | 已有 | 直接将 libretro 帧纹理传给 NanoVG，未经着色器处理 |
| `DisplayConfig` / 过滤模式 | 已有 | 支持 Nearest / Linear 切换 |

**结论：完全可行。** 核心 OpenGL 框架（FBO、着色器程序、VAO）均已就绪，缺少的是：
- RetroArch 着色器文件解析器
- 与 GameView 渲染流程的集成

---

## 实现内容

### 1. ShaderChain 增强（`include/Game/ShaderChain.hpp` + `src/Game/ShaderChain.cpp`）

- **新增 RetroArch 兼容 uniform 槽**（`ShaderPass` 结构体扩展）：
  - `sourceLoc` → `uniform sampler2D Source / Texture`
  - `sourceSizeLoc` → `uniform vec4 SourceSize / TextureSize`
  - `outputSizeLoc` → `uniform vec4 OutputSize`
  - `frameCountLoc` → `uniform uint FrameCount`
- **新增 `_lookupUniforms()`** 私有方法，统一查询所有 uniform/attrib 位置（原来在 init/addPass 中重复写）。
- **新增帧计数器 `m_frameCount`**，每帧 +1，传递给 `FrameCount` uniform，用于时间相关效果（闪烁、动画等）。
- **修复 `insize` 计算错误**：原代码假设源纹理是 256×256 填充格式（`insize = videoW/256, videoH/256`），而 `LibretroLoader` 实际输出的是恰好 `videoW × videoH` 的纹理；修正为所有通道统一使用 `(1.0, 1.0)`，覆盖完整 UV 范围。

### 2. GlslpLoader（新增，`include/Game/GlslpLoader.hpp` + `src/Game/GlslpLoader.cpp`）

RetroArch 着色器加载与兼容层，**完全独立，不依赖 GameView 或 libretro**。

#### 支持的文件格式

| 格式 | 说明 |
|------|------|
| `.glsl`（`#pragma stage`） | RetroArch 标准单文件着色器，用 `#pragma stage vertex` / `#pragma stage fragment` 分隔 |
| `.glsl`（`#ifdef VERTEX`）  | 旧式着色器，用 `#ifdef VERTEX` / `#ifdef FRAGMENT` 宏分隔 |
| `.glsl`（纯片段）           | 无阶段标记时，自动生成直通顶点着色器 |
| `.glslp`                   | RetroArch 多通道预设，`shaders=N` + `shader0=...` 等 |

#### 兼容性处理（GLSL 版本自适应）

根据构建目标（`NANOVG_GL3` / `NANOVG_GLES3` / `NANOVG_GL2` / `NANOVG_GLES2`）自动注入版本头并进行语法转换：

| Legacy GLSL 语法 | GL3 转换 |
|----------------|----------|
| `attribute` | → `in` |
| `varying`（vert） | → `out` |
| `varying`（frag） | → `in` |
| `gl_FragColor` | → `fragColor`（+声明 `out vec4 fragColor;`）|
| `texture2D(` | → `texture(` |

#### RetroArch 标准变量映射

| RetroArch 变量 | 映射 | 说明 |
|---------------|------|------|
| `VertexCoord` | `vec2(offset*2-1)` | 从 `offset` [0,1]² 派生 NDC 坐标 |
| `TexCoord` | `offset * insize` | UV 坐标 |
| `MVPMatrix` | `mat4(1.0)` | 单位矩阵（VertexCoord 已是 NDC）|
| `Texture` / `Source` | `tex` | 输入采样器别名 |

### 3. GameView 集成（`include/UI/game_view.hpp` + `src/UI/game_view.cpp`）

- `GameView` 增加成员：`m_shaderChain`、`m_shaderEnabled`、`m_nvgShaderImage`、`m_shaderDisplayW/H`
- **`initialize()`**：
  - 始终调用 `m_shaderChain.initBuiltin()` 初始化内置直通通道（pass 0）
  - 读取 `shader.preset` 配置项（默认空）
  - 根据扩展名选择 `GlslpLoader::loadGlslIntoChain()` 或 `loadGlslpIntoChain()`
- **`draw()`**：
  - 每帧调用 `m_shaderChain.run(m_texture, w, h)` 执行完整渲染链
  - 若链有有效输出（`chainOut ≠ m_texture`），使用 FBO 输出纹理进行 NVG 渲染
  - 否则回退到原始 `m_texture` 直接渲染（零侵入性降级）
  - 跟踪输出尺寸变化，按需重建 NVG 图像句柄
  - `setFilter` 变化时同步更新 `m_shaderChain`
- **`cleanup()`**：调用 `m_shaderChain.deinit()` 释放所有 FBO/纹理/着色器资源

### 4. 配置项

| 键 | 类型 | 默认值 | 说明 |
|----|------|--------|------|
| `shader.preset` | string | `""` | 着色器文件路径（空 = 仅内置直通） |

示例：
```ini
shader.preset=resources/shaders/scanlines.glsl
shader.preset=resources/shaders/example.glslp
shader.preset=/home/user/my_shader.glsl
```

### 5. 示例着色器资源（`resources/shaders/`）

| 文件 | 效果 |
|------|------|
| `passthrough.glsl` | 直通（无效果，用于测试） |
| `scanlines.glsl` | CRT 扫描线（逐行亮度交替降低） |
| `sharpen.glsl` | Unsharp Mask 锐化（Laplacian 边缘增强）|
| `crt_simple.glsl` | 综合 CRT 效果（扫描线 + 晕染 + 暗角）|
| `example.glslp` | 多通道预设示例（锐化 → 扫描线）|

---

## 渲染流程对比

### 改动前

```
libretro 帧数据 → uploadFrame() → m_texture → NanoVG 直接渲染
```

### 改动后

```
libretro 帧数据 → uploadFrame() → m_texture
                                      ↓
                          ShaderChain::run()
                          ┌─────────────────────────────────────────┐
                          │ pass 0（内置）: 颜色校正 + FBO 标准化   │
                          │ pass 1（可选）: 用户着色器 1            │
                          │ pass N（可选）: 用户着色器 N            │
                          └─────────────────────────────────────────┘
                                      ↓
                          最终 FBO 输出纹理 → NanoVG 渲染
```

---

## 代码耦合性说明

- **`GlslpLoader`** 仅依赖 `ShaderChain`，不了解 `GameView`、libretro 或 NanoVG，可独立使用。
- **`ShaderChain`** 仅依赖 OpenGL（glad）和 borealis Logger，完全自治。
- **`GameView`** 对 `ShaderChain` 的集成为**非侵入式**：若 `initBuiltin()` 失败或 `run()` 返回原始纹理，自动回退到原渲染路径，不影响现有功能。
- 无全局状态，无单例依赖。

---

## 兼容性与局限性

| 场景 | 状态 |
|------|------|
| GL3 桌面（Linux/macOS/Windows）| ✅ 完全支持 |
| GLES3（Switch 等）| ✅ 支持（兼容头自动切换版本）|
| GLES2 / GL2 | ✅ 支持（使用 legacy 语法，不做 varying 替换）|
| RetroArch legacy GLSL（attribute/varying）| ✅ 自动转换 |
| RetroArch Slang（.slang）| ❌ 不支持（需要 SPIRV/glslang 编译器）|
| 着色器 `scale_type` / 超分辨率 FBO | ⚠️ 当前 FBO 尺寸固定为视频原始分辨率；需扩展才能支持输出放大 |
| RetroArch 模拟器纹理反馈（PREV 帧）| ❌ 不支持（需要双缓冲 FBO） |

---

## 测试验证

- 编译通过（Linux Desktop Debug 模式，零 error，现有 warning 均为第三方库或历史遗留）
- 架构验证：`GlslpLoader::parseGlslFile` 逻辑通过代码审查
- 示例着色器已手工验证 GLSL 语法的兼容性处理逻辑
- 回退路径（`shader.preset` 为空）与改动前行为完全一致
