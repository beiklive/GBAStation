# Session 99 任务分析

## 任务目标

实现 RetroArch 着色器支持，参考 `example/chat.md` 中的架构设计，将游戏帧数据通过多通道 GPU 着色器管线处理后显示。

## 架构分析

### 现有代码结构

- `include/Video/RenderChain.hpp` + `src/Video/RenderChain.cpp`：占位符（直通实现）
- `GameView::uploadFrame()` + `GameView::draw()`：帧上传和 NanoVG 显示
- `m_renderChain.run()` 调用：直接返回原始纹理

### 目标渲染链

```
frame.pixels (CPU内存)
        │
        ▼
上传到 OpenGL Texture (m_texture)
        │
        ▼
Game FBO
        │
        ▼
RetroArch Shader Pass0 (FBO)
        │
        ▼
RetroArch Shader Pass1 (FBO)
        │
        ▼
RetroArch Shader PassN (FBO)
        │
        ▼
Final Texture
        │
        ▼
NanoVG UI / 显示
```

## 实现设计

### 新增文件

**头文件（`include/Video/renderer/`）：**
1. `GLSLPParser.hpp` - 解析 RetroArch .glslp 预设文件
2. `ShaderCompiler.hpp` - 编译合并 GLSL 着色器
3. `FullscreenQuad.hpp` - 全屏四边形渲染
4. `GameTexture.hpp` - 游戏纹理管理
5. `FrameUploader.hpp` - CPU 帧上传工具
6. `RetroShaderPipeline.hpp` - 多通道着色器管线

**实现文件（`src/Video/renderer/`）：**
对应的 .cpp 实现文件

### 关键设计决策

#### GLES 兼容性
- 不同平台注入不同 `#version` 前缀
- GLES2: `#version 100`
- GLES3: `#version 300 es` + `precision mediump float;`
- GL3/GL4: `#version 130`
- RetroArch 着色器通过 `#ifdef GL_ES` + `#if __VERSION__ >= 130` 自适应

#### 顶点数据布局
- `VertexCoord` (layout=0): vec4 位置 (x,y,z,w)
- `COLOR` (layout=1): vec4 颜色 (r,g,b,a)
- `TexCoord` (layout=2): vec4 纹理坐标 (u,v,s,t)
- 链接前 `glBindAttribLocation()` 绑定固定位置

#### NVG 图像句柄管理
- 添加 `m_nvgImageSrc` 跟踪当前封装的 GL 纹理 ID
- 着色器输出纹理切换时自动重建 NVG 句柄
- `uploadFrame()` 尺寸变化时仅失效 NVG 句柄，不立即重建

#### GL 状态保存/恢复
- 着色器管线执行前保存 `GL_FRAMEBUFFER_BINDING`、`GL_VIEWPORT`、`GL_CURRENT_PROGRAM`
- 管线执行后完整恢复，确保 NanoVG 渲染不受影响

#### 配置项
- `display.shader` = .glslp 文件路径（空=直通模式）
- 默认值为空字符串，即默认不使用着色器

## 挑战与解决方案

| 挑战 | 解决方案 |
|------|---------|
| GLES vs Desktop GL 版本差异 | 编译时宏选择 `#version` 字符串 |
| NVG 图像句柄引用旧纹理 | `m_nvgImageSrc` 追踪，变化时重建 |
| FBO 状态污染 NanoVG | process() 开始/结束保存/恢复 GL 状态 |
| 运行时着色器切换 | `RenderChain::setShader()` 接口 |
