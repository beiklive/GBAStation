# Session 109 工作汇报

## 任务分析

### 任务目标
修复三个 phosphor-dot v3.3 着色器预设在渲染时的问题：
1. `2x-Dot.glslp` — 左边和下边边缘像素拉伸
2. `Dot+ScaleFX.glslp` — 画面像素上下错位
3. `CRT-diffusion.glslp` — 画面模糊

### 输入
- phosphor-dot v3.3 着色器预设文件（.glslp）
- RetroShaderPipeline（渲染管线）
- ShaderCompiler（着色器编译器）
- GLSLPParser（预设解析器）

### 根本原因分析

#### 问题1 & 问题2（边缘拉伸 + 上下错位）
- **根本原因**：所有 .glslp 预设均指定 `wrap_mode = "clamp_to_border"`，但 GLSLPParser 没有解析该字段，FBO 纹理和游戏帧纹理均使用 `GL_CLAMP_TO_EDGE`。
- **效果**：当着色器的 UV 坐标超出 `[0,1]` 时（例如 phosphor-dot 的内容居中计算），`GL_CLAMP_TO_EDGE` 会将边缘像素向外拉伸，而正确行为应是返回黑色（透明边框）。
- **表现**：
  - 2x-Dot：左边和下边显示了源纹理边缘像素的重复（拉伸）
  - Dot+ScaleFX：视口外侧边框区域出现了源图像的内容（像素错位感）

#### 问题3（画面模糊）
- **根本原因**：`diffusion-sampler.glsl` 和 `diffusion-mixer.glsl` 两个着色器在顶点着色器中使用 `PassPrev2TexCoord` 顶点属性，但 `ShaderCompiler::compileProgram()` 只绑定了 `VertexCoord`（位置0）、`COLOR`（位置1）、`TexCoord`（位置2）三个属性，未绑定 `PassPrevNTexCoord`。
- **效果**：`PassPrev2TexCoord` 获得默认值 `(0,0,0,0)`，所有顶点的该属性均为零。
- **表现**：`diffusion-sampler.glsl` 对 `PassPrev2Texture` 的采样全部在 UV(0,0) 处（落在图像边缘黑色区域），导致扩散模糊效果错误；`diffusion-mixer.glsl` 将 `PassPrev2Texture`（HtDot 通道输出）在错误坐标处采样，混合结果出现异常，整体画面呈现模糊感。

## 解决方案

### 修改1：GLSLPParser.hpp
- 在 `ShaderPassDesc` 结构体中新增 `WrapMode` 枚举（`ClampToEdge`, `ClampToBorder`, `Repeat`, `MirroredRepeat`）
- 新增成员 `WrapMode wrapMode = WrapMode::ClampToEdge`，默认保持兼容性
- 在文档中新增 `wrap_mode0` 键的说明

### 修改2：GLSLPParser.cpp
- 新增 `parseWrapMode()` 辅助函数：将字符串（"clamp_to_border" 等）映射到枚举值
- 在 per-pass 解析循环中，读取 `wrap_mode<idx>` 键并赋值给 `pass.wrapMode`

### 修改3：ShaderCompiler.cpp
- 在 `compileProgram()` 中，循环绑定 `PassPrev1TexCoord` ～ `PassPrev8TexCoord` 顶点属性到位置2（与 `TexCoord` 相同）
- 原因：本实现所有 FBO 纹理的 UV 均为 `[0,1]`，与 `TexCoord` 完全一致；绑定后 `PassPrevNTexCoord` 能正确传递 UV 坐标

### 修改4：RetroShaderPipeline.cpp
- 在每个渲染通道绑定输入纹理之前，根据 `pass.desc.wrapMode` 设置 `GL_TEXTURE_WRAP_S` 和 `GL_TEXTURE_WRAP_T`
- 当使用 `GL_CLAMP_TO_BORDER` 时，设置边框颜色为黑色透明 `(0,0,0,0)`（非 GLES2 平台）
- GLES2 平台不支持 `GL_CLAMP_TO_BORDER`，降级使用 `GL_CLAMP_TO_EDGE`
- 管线处理结束后，将原始游戏帧纹理（inputTex）的环绕模式恢复为 `GL_CLAMP_TO_EDGE`，避免影响 NanoVG 后续渲染

## 测试结果
- 编译通过（无新增编译错误）
- 所有修改均针对渲染链中的着色器处理逻辑，不影响游戏逻辑和输入系统

## 文件变更清单
| 文件 | 变更内容 |
|------|---------|
| `include/Video/renderer/GLSLPParser.hpp` | 新增 `WrapMode` 枚举和 `wrapMode` 字段 |
| `src/Video/renderer/GLSLPParser.cpp` | 新增 `parseWrapMode()` + 解析 `wrap_mode` 键 |
| `src/Video/renderer/ShaderCompiler.cpp` | 绑定 `PassPrevNTexCoord` 属性到位置2 |
| `src/Video/renderer/RetroShaderPipeline.cpp` | 在每个通道前设置输入纹理的 wrap mode |
