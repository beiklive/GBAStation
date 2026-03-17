# Session 103 工作汇报：修复着色器黑屏与焦点残留问题

## 任务说明

修复以下两个问题：
1. 文件列表选择着色器后焦点有概率残留在文件列表位置
2. 特定 .glslp 着色器（磷光线 v2.0、gameboy-player-tvout+interlacing）导致渲染画面为黑色

## 问题根因与修复

### 问题1：焦点残留（GameMenu.cpp）

borealis 的 `popActivity()` 通过焦点栈恢复焦点，但在某些情况下（视图层次遍历、时序等）可能无法正确恢复。**修复**：在 `popActivity()` 后显式调用 `giveFocus(m_shaderPathCell/m_overlayPathCell)`。

### 问题2：着色器黑屏（RetroShaderPipeline + GLSLPParser）

**根因1**：`OrigInputSize` uniform 未设置（默认 `(0,0)` → `1.0/0 = Inf/NaN`）  
**修复**：`setUniforms()` 新增 `origW/origH` 参数，设置 `OrigInputSize`、`OrigSize`、`OriginalSize` uniform。

**根因2**：`PassPrevN` 纹理（相对索引）未绑定  
**修复**：`process()` 中对每个历史通道同时绑定 `PassNTexture`（绝对）和 `PassPrev{idx-pi}Texture`（相对）。

**根因3**：`PassPrevNTextureSize`/`PassPrevNInputSize` 未设置（默认 `(0,0)` → NaN）  
**修复**：在 `setUniforms()` 之后，对历史通道设置绝对/相对/别名尺寸 uniform。

**根因4**：`.glslp parameters` 中的参数覆盖值未传递（如 `PIC_SCALE_Y = 0.9`）  
**修复**：
- `GLSLPParser::parse()` 增加 `outParams` 输出，解析 `parameters` 节的名称和值
- `RetroShaderPipeline::init()` 存储参数覆盖到 `m_params`
- `setUniforms()` 遍历 `m_params`，对每个通道设置 `uniform float` 覆盖值

## 修改文件清单

| 文件 | 修改说明 |
|------|---------|
| `include/Video/renderer/GLSLPParser.hpp` | 新增 `GLSLPParamOverride` 结构体；`parse()` 增加 `outParams` 输出参数 |
| `src/Video/renderer/GLSLPParser.cpp` | 实现 parameters 节解析 |
| `include/Video/renderer/RetroShaderPipeline.hpp` | `setUniforms()` 增加 `origW/origH`；新增 `m_params` 字段 |
| `src/Video/renderer/RetroShaderPipeline.cpp` | 实现所有 uniform 修复（OrigInputSize、PassPrevN、参数覆盖） |
| `src/UI/Utils/GameMenu.cpp` | popActivity() 后显式恢复焦点 |
| `report/session_103_analysis.md` | 任务分析文档 |
| `report/session_103.md` | 本工作汇报 |
