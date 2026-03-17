# Session 103 任务分析：焦点残留与着色器黑屏问题修复

## 任务目标

解决以下两个问题：

1. **焦点问题**：从文件列表选择着色器返回到 GameMenu 后，有概率焦点仍残留在文件列表位置
2. **着色器黑屏问题**：特定 .glslp 着色器（如 `F00_PhosphorLineReflex(Base).glslp`、`gameboy-player-tvout+interlacing.glslp`）导致渲染画面变为黑色

## 问题根因分析

### 问题1：焦点残留

**代码路径**：`src/UI/Utils/GameMenu.cpp` 中 `m_shaderPathCell`/`m_overlayPathCell` 的文件选择回调。

**根因**：调用 `brls::Application::popActivity()` 后，borealis 在内部通过焦点栈尝试恢复焦点。在以下情况下会导致恢复失败：
- `getParentActivity()` 无法正确遍历到当前 Activity（如视图层次较深）
- 回调在文件列表项目聚焦状态下触发，导致状态不一致

**修复方案**：在 `popActivity()` 之后，显式调用 `brls::Application::giveFocus(m_shaderPathCell)` / `giveFocus(m_overlayPathCell)`，确保焦点无论如何都会正确恢复。

### 问题2：着色器黑屏

通过分析失败的着色器预设文件，发现多个 uniform 变量未正确设置。

#### 2.1 `OrigInputSize` 未设置

**影响**：`phosphor-line v2.0` 系列所有 Pass（`phosphor-line.glsl`、`PP-reflex.glsl`、`post-process.glsl` 等）

**表现**：
```glsl
uniform vec2 OrigInputSize;
vec4 bbb = vec4(OrigInputSize, 1.0/OrigInputSize);
```
当 `OrigInputSize = (0,0)` 时，`1.0/OrigInputSize = Inf` 或 `NaN`，传播至后续计算导致输出黑色。

#### 2.2 `PassPrevN` 纹理未绑定

**影响**：`diffusion-h.glsl`（Pass2）、`PP-reflex.glsl`（Pass4）

**表现**：
```glsl
uniform sampler2D PassPrev2Texture; // diffusion-h.glsl
uniform sampler2D PassPrev3Texture; // PP-reflex.glsl
```
`PassPrevN` 是相对引用（当前通道索引 - N = 历史通道），原代码只绑定了绝对索引 `PassNTexture`，未绑定相对索引形式，导致采样器默认指向 unit 0（当前输入），产生错误输出。

#### 2.3 `PassPrevNTextureSize` / `PassPrevNInputSize` 未设置

**影响**：同上

**表现**：
```glsl
uniform vec2 PassPrev3TextureSize; // = (0,0) → 1.0/0.0 = NaN
uniform vec2 PassPrev3InputSize;
vec2 uvratio = PassPrev3InputSize / PassPrev3TextureSize; // = NaN
```
NaN 传入纹理坐标，采样结果为黑色。

#### 2.4 `.glslp parameters` 参数未解析和传递

**影响**：`PP-reflex.glsl` 中的 `PIC_SCALE_Y` 等参数

**表现**：`PP-reflex.glsl` 中 `PIC_SCALE_Y` 无 `#ifdef PARAMETER_UNIFORM` 保护，直接声明为 `uniform float PIC_SCALE_Y;`，默认值为 0.0。`vParameter.y = f / PIC_SCALE_Y` 会产生除零，F00 预设中覆盖值为 0.9。

## 修改文件清单

| 文件 | 修改内容 |
|------|---------|
| `include/Video/renderer/GLSLPParser.hpp` | 新增 `GLSLPParamOverride` 结构体；`parse()` 增加 `outParams` 输出参数 |
| `src/Video/renderer/GLSLPParser.cpp` | 实现 `parameters` 节的解析（名称 + 值覆盖） |
| `include/Video/renderer/RetroShaderPipeline.hpp` | `setUniforms()` 增加 `origW/origH` 参数；新增 `m_params` 字段 |
| `src/Video/renderer/RetroShaderPipeline.cpp` | 存储参数覆盖；`setUniforms()` 设置 `OrigInputSize/OrigSize`；`process()` 绑定 `PassPrevN` 纹理并设置尺寸 uniform；应用参数覆盖值 |
| `src/UI/Utils/GameMenu.cpp` | 着色器和遮罩选择回调中 `popActivity()` 后显式 `giveFocus()` |
