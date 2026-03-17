# Session 99 工作汇报

## 任务描述

实现 RetroArch 着色器支持，参考 `example/chat.md` 中的架构。

## 修改内容

### 新增文件（`include/Video/renderer/`）

#### `GLSLPParser.hpp` / `GLSLPParser.cpp`
- 解析 RetroArch `.glslp` 着色器预设文件
- 支持 `shaders`、`shader0..N`、`filter_linear0`、`scale_type_x/y0`、`scale_x/y0` 等键
- 着色器路径相对于 `.glslp` 文件目录解析为绝对路径

#### `ShaderCompiler.hpp` / `ShaderCompiler.cpp`
- 编译 RetroArch 合并 `.glsl` 文件（`#if defined(VERTEX)` / `#elif defined(FRAGMENT)` 格式）
- 自动注入平台对应 `#version`（GLES2/GLES3/GL2/GL3）
- 链接前绑定固定属性位置（`VertexCoord=0`, `COLOR=1`, `TexCoord=2`）

#### `FullscreenQuad.hpp` / `FullscreenQuad.cpp`
- VAO + VBO + EBO 全屏四边形（-1..+1 NDC）
- 顶点格式：`VertexCoord(vec4)` + `COLOR(vec4)` + `TexCoord(vec4)`
- 支持 GLES2 无 VAO 回退路径

#### `GameTexture.hpp` / `GameTexture.cpp`
- 管理游戏帧 GL 纹理（RGBA8888，自动分配/调整大小）
- 支持运行时过滤模式切换

#### `FrameUploader.hpp` / `FrameUploader.cpp`
- RGBA8888 直接上传
- XRGB8888（libretro 格式）上传：桌面用 `GL_BGRA`，GLES 用软件转换

#### `RetroShaderPipeline.hpp` / `RetroShaderPipeline.cpp`
- 多通道着色器管线核心实现
- `init(glslpPath)` 加载并编译所有通道
- `process()` 逐通道执行 FBO 渲染
- 支持标准 RetroArch uniform：`MVPMatrix`、`FrameCount`、`FrameDirection`、`TextureSize`、`InputSize`、`OutputSize`、`Texture`/`Source`
- GL 状态保存/恢复（FBO、视口、程序）

### 修改文件

#### `include/Video/RenderChain.hpp`
- 从直通占位符升级为完整实现
- 集成 `RetroShaderPipeline`
- 新增 `init(shaderPath)` 参数、`setShader()` 接口
- 更新 `run()` 签名：新增可选 `viewW`/`viewH` 参数

#### `src/Video/RenderChain.cpp`
- 实现 `init()`、`deinit()`、`run()`、`setShader()`
- 直通模式（无着色器）仍完整工作

#### `include/Game/game_view.hpp`
- 新增 `m_nvgImageSrc` 成员（跟踪 `m_nvgImage` 当前封装的 GL 纹理 ID）

#### `src/Game/game_view.cpp`
- `initialize()`：从配置读取 `display.shader`，传入 `m_renderChain.init()`
- `initialize()`：新增 `display.shader` 配置默认值（空字符串）
- `uploadFrame()`：尺寸变化时仅失效 NVG 图像句柄，不立即重建
- `draw()`：
  - `run()` 调用增加 `width`/`height` 视口参数
  - 使用 `m_renderChain.outputW/H()` 获取实际输出尺寸
  - 新增 `m_nvgImageSrc` 比较逻辑，着色器输出变化时重建 NVG 句柄

## 使用方法

在配置文件中设置：
```ini
display.shader = /path/to/shader.glslp
```

留空（默认）则以直通模式运行，不影响原有功能。

## 测试

构建测试（Linux）：
- `cmake --build build_linux -j$(nproc)` ✓ 编译通过，仅原有警告

着色器使用示例（`example/shaders/bilinear.glslp`）：
```ini
display.shader = /path/to/example/shaders/bilinear.glslp
```
