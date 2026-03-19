# Session 110 工作汇报 - 渲染链和GameView着色器问题修复

## 任务分析

### 任务目标
修复以下三个着色器渲染问题：
1. **ScaleFX列偏移**：所有含 ScaleFX 的 `.glslp` 文件中，游戏画面每隔几列像素向上偏移
2. **CRT+nnedi3跳过通道**：`CRT+nnedi3.glslp` 着色器渲染时跳过通道1和2（无程序/编译失败）
3. **2x-Scanline压扁居中**：`2x-Scanline.glslp` 游戏画面被压扁居中

### 根本原因分析

#### 问题2（nnedi3跳过通道）
- `ShaderCompiler::compileRetroShader()` 强制将所有着色器的 `#version` 替换为 `#version 130`
- `nnedi3` 着色器声明 `#version 330`，依赖 GLSL 3.30 的特性
- 用 `#version 130` 替换后，某些 GLSL 3.30 特性可能导致编译失败

#### 问题3（2x-Scanline压扁）
- **子问题A**：`GLSLPParser` 未遵循 RetroArch 规范：最后一个通道若无显式缩放设置，应默认为 `viewport×1.0`，而非 `source×1.0`
- **子问题B**：`game_view.cpp` 对视口缩放着色器（FBO输出 == 视口大小）做了额外的 NanoVG 宽高比缩放，导致着色器自带的居中/裁剪逻辑被二次扭曲

#### 问题1（ScaleFX列偏移）
- **过滤模式不符合 RetroArch 规范**：RetroArch 规范中 `filter_linearN` 决定通道 N 的**输入**纹理过滤方式，而非通道 N 的 FBO 输出纹理创建时的过滤方式
- 原代码在 `allocateFBO()` 创建 FBO 时使用当前通道的 `filterLinear` 设置过滤模式，应在 `process()` 中为每个通道设置其输入纹理的过滤模式

## 修复内容

### 修改文件

#### 1. `src/Video/renderer/ShaderCompiler.cpp`
**修复**：保留着色器原始 `#version` 声明，而非强制覆盖为 `#version 130`
- 新增逻辑：从着色器文件中提取 `#version XXX` 行
- 若着色器有版本声明（如 `#version 330`），使用该版本
- 若无声明，回退到平台默认版本（`#version 130`）
- 解决：nnedi3 等需要 GLSL 3.30 的着色器编译失败问题

#### 2. `include/Video/renderer/GLSLPParser.hpp`
**修复**：在 `ShaderPassDesc` 中新增 `hasExplicitScale` 标志
- 追踪通道是否在 `.glslp` 文件中显式指定了缩放设置
- 用于后续判断最后通道是否需要应用默认 viewport×1.0

#### 3. `src/Video/renderer/GLSLPParser.cpp`
**修复**：
- 在所有 `scale_type`/`scale`/`scale_x`/`scale_y` 等缩放键解析时，设置 `hasExplicitScale = true`
- 解析完所有通道后，若最后一个通道未显式指定缩放，按 RetroArch 规范将其默认设为 `viewport×1.0`

#### 4. `src/Video/renderer/RetroShaderPipeline.cpp`
**修复**：在 `process()` 中为每个通道正确设置输入纹理的过滤模式
- 原代码仅在 `allocateFBO()` 创建纹理时设置过滤模式（使用当前通道的 `filterLinear`，而非下一通道的）
- 新增：在绑定输入纹理前，根据**当前通道**的 `filterLinear` 设置 `currentTex` 的 `GL_MIN/MAG_FILTER`
- 这确保 phosphor-dot（`filter_linear=true`）等着色器能正确以线性过滤读取前一通道输出

#### 5. `src/Game/game_view.cpp`
**修复**：视口缩放着色器输出直接填充整个视口，不做额外 NanoVG 宽高比缩放
- 原代码：当 FBO 输出 == 视口大小时，用游戏原始尺寸做 `computeRect()` 宽高比缩放，再用 NanoVG 把视口大小的 FBO 压缩进小矩形 → 画面扭曲
- 新逻辑：当 FBO 输出 == 视口大小时，着色器已自行处理宽高比，直接 `rect = {x, y, width, height}` 填满整个视口
- 非视口缩放着色器（如 ScaleFX 3× source scale）仍使用 `computeRect()` 保持宽高比

## 已验证

- CMake 配置成功（Desktop/Linux 平台）
- 编译通过，无新增错误
- 修改文件：ShaderCompiler.cpp、GLSLPParser.hpp、GLSLPParser.cpp、RetroShaderPipeline.cpp、game_view.cpp

## 遗留问题

- **ScaleFX列偏移的根本原因**：经过深入分析，`float_framebuffer` 量化精度（GL_RGBA8）、浮点数精度、`textureOffset` 行为等均符合规范。修复了过滤模式后，实际效果需在真实设备上验证。若问题仍存在，可能需要增加调试输出来定位具体问题。
