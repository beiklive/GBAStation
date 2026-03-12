# RetroArch GLSL 着色器兼容性全面移植报告

## 任务背景

前几次的修改已修复了 `zfast_crt.glsl` 的特定兼容问题，但项目整体对 RetroArch
着色器生态的兼容性仍不完整。本次任务将 `example/glslp/` 目录中的两个关键 RetroArch
源文件（`shader_glsl.c`、`video_shader_parse.c`）中的核心逻辑全面移植到项目中，
彻底解决 RetroArch GLSL 着色器体系的兼容难题。

---

## 参考的 example 文件

| 文件 | 说明 |
|------|------|
| `example/glslp/drivers_shader/shader_glsl.c` | RetroArch GLSL 着色器后端（`gl_glsl_strip_parameter_pragmas`, `gl_glsl_compile_program` 等） |
| `example/glslp/video_shader_parse.c` | RetroArch 预设解析器（`video_shader_parse_pass`, `video_shader_parse_textures`, `#reference` 等） |
| `example/glslp/video_shader_parse.h` | 预设相关数据结构 |

---

## 根本原因分析

### 1. `#pragma parameter` 行导致 GLSL 编译失败（**关键缺陷**）

RetroArch 着色器通过 `#pragma parameter` 行声明可调参数，例如：
```glsl
#pragma parameter SCANLINE_INTENSITY "Scanline Intensity" 0.4 0.0 1.0 0.05
```
这些行包含 `"` 字符，属于合法的 C 预处理器语法，但在 GLSL 编译器中是非法字符。  
`shader_glsl.c::gl_glsl_strip_parameter_pragmas()` 明确指出并处理此问题：
```c
/* #pragma parameter lines tend to have " characters in them,
 * which is not legal GLSL. */
```
原实现未剥除这些行，导致任何包含 `#pragma parameter` 的着色器均编译失败。

### 2. 缺少 `COMPAT_*` 宏定义

大量 RetroArch 着色器使用 COMPAT_* 宏来实现 GL2/GL3 的可移植性：
```glsl
COMPAT_ATTRIBUTE vec4 VertexCoord;
COMPAT_VARYING vec2 vTexCoord;
vec4 c = COMPAT_TEXTURE(tex, vTexCoord);
```
`shader_glsl.c::gl_glsl_compile_program()` 中以 `#define VERTEX` / `#define FRAGMENT` /
`#define PARAMETER_UNIFORM` 等作为额外预处理段传入，但未定义 `COMPAT_*` 宏。
原兼容头仅翻译了 `attribute`/`varying`/`texture2D` 字面关键字，对使用 COMPAT_* 宏的
着色器完全无效。

### 3. 缺少 `PARAMETER_UNIFORM` 宏

RetroArch 着色器通过 `#ifdef PARAMETER_UNIFORM` 实现参数的"uniform 模式"与"常量模式"切换：
```glsl
#ifdef PARAMETER_UNIFORM
uniform float SCANLINE_INTENSITY;
#else
#define SCANLINE_INTENSITY 0.4
#endif
```
原实现未定义 `PARAMETER_UNIFORM`，导致所有参数强制使用硬编码常量（无法通过预设文件覆盖）。

### 4. `.glslp` 预设功能缺失

`video_shader_parse.c` 中实现的多项预设功能在原实现中完全缺失：

| 功能 | 影响 |
|------|------|
| `#reference` 指令 | 无法加载引用其他预设的简单预设文件 |
| 每通道 `scale_type` / `scale` | 所有通道均以视频原始分辨率渲染，无法缩放 |
| 每通道 `wrap_mode` | FBO 纹理始终使用 CLAMP_TO_EDGE |
| LUT 纹理（`textures=`） | 无法使用查找纹理（大量 CRT、像素化滤镜均依赖此功能） |
| `frame_count_mod` | 着色器帧计数器无法取模 |
| 通道别名（`alias`） | 无法定义通道引用别名 |

---

## 修改内容

### 1. `src/Game/GlslpLoader.cpp`（移植 `shader_glsl.c` + `video_shader_parse.c`）

#### A. `#pragma parameter` 剥除（移植自 `shader_glsl.c::gl_glsl_strip_parameter_pragmas`）
```cpp
/// 从 GLSL 源码中剥除 #pragma parameter 行。
/// 这些行含有 " 字符（合法 C 预处理器，但非法 GLSL）。
static std::string stripPragmaParameters(const std::string& src);
```
- 被 `preprocessVertSource()` 和 `preprocessFragSource()` 在处理前调用。
- 保留换行符以维持行号不变（利于调试）。

#### B. `#pragma stage` 剥除
```cpp
/// 剥除 #pragma stage 标记行（已被分割逻辑消费，不应进入着色器源码）
static std::string stripStagePragmas(const std::string& src);
```

#### C. `COMPAT_*` 宏注入（移植自 `shader_glsl.c` 编译前缀）

顶点着色器兼容头新增：
```glsl
#define PARAMETER_UNIFORM
#define VERTEX
#define COMPAT_ATTRIBUTE in           // (GL3/GLES3) 或 attribute (GL2/GLES2)
#define COMPAT_VARYING out            // 顶点着色器输出
#define COMPAT_PRECISION              // 空 (GL3) 或 mediump (GLES2)
#define COMPAT_TEXTURE texture        // (GL3) 或 texture2D (GL2)
#define COMPAT_TEXTURE_2D texture     // 同上，兼容旧名称
```

片段着色器兼容头新增：
```glsl
#define PARAMETER_UNIFORM
#define FRAGMENT
#define COMPAT_VARYING in             // 片段着色器输入
#define COMPAT_PRECISION
#define COMPAT_TEXTURE texture
#define COMPAT_TEXTURE_2D texture
```

#### D. `COMPAT_*` 语法转换

`preprocessVertSource()` 和 `preprocessFragSource()` 中新增对 COMPAT_* 系列的
行级替换，兜底处理未展开宏的情况：
- `COMPAT_ATTRIBUTE ` → `in ` / `attribute `
- `COMPAT_VARYING ` → `out `/`in ` / `varying `
- `COMPAT_TEXTURE_2D(` → `texture(` / `texture2D(`
- `COMPAT_TEXTURE(` → 同上

#### E. `.glslp` 预设增强（移植自 `video_shader_parse.c`）

**`#reference` 指令支持：**
```
#reference "path/to/other_preset.glslp"
```
解析到此行后，递归加载引用的预设文件（RetroArch "简单预设"格式）。

**每通道 scale 因子（移植自 `video_shader_parse_pass()`）：**
- `scale_type` / `scale_type_x` / `scale_type_y`：`source` / `viewport` / `absolute`
- `scale` / `scale_x` / `scale_y`：乘数
- 传递给新增的 `ShaderPassScale` 结构体，影响 FBO 尺寸计算

**每通道 wrap 模式（移植自 `video_shader_parse_pass()`）：**
- `wrap_mode0` = `clamp_to_edge` / `repeat` / `mirrored_repeat` / `clamp_to_border`
- 传递给 `ShaderPass::wrapMode`，用于 FBO 纹理参数设置

**LUT 纹理（移植自 `video_shader_parse_textures()`）：**
```ini
textures = lut1;lut2
lut1_path = textures/palette.png
lut1_linear = true
lut1_mipmap = false
lut1_wrap_mode = repeat
```
- 支持 PNG / JPEG / BMP / TGA（通过 stb_image）
- 自动查找各通道中对应的 uniform 位置并在渲染时绑定

**帧计数取模 / 通道别名：**
- `frame_count_mod0 = 60` → `FrameCount` 每 60 帧重置
- `alias0 = PassName` → 通道别名

---

### 2. `include/Game/ShaderChain.hpp` / `src/Game/ShaderChain.cpp`

#### 新增数据结构

```cpp
/// 每个通道的 FBO 缩放设置（移植自 video_shader_parse.h::gfx_fbo_scale）
struct ShaderPassScale {
    enum Type { SOURCE = 0, ABSOLUTE, VIEWPORT };
    Type typeX, typeY;
    float scaleX, scaleY;   // SOURCE/VIEWPORT 乘数
    int   absX, absY;       // ABSOLUTE 固定像素尺寸
};

/// LUT（查找）纹理描述符
struct ShaderLut {
    std::string id;          // uniform 名称
    GLuint      tex = 0;
};
```

`ShaderPass` 新增字段：
- `inputSizeLoc` → `uniform vec4 InputSize`
- `finalVpSizeLoc` → `uniform vec4 FinalViewportSize`
- `lutLocs[]` → LUT uniform 位置数组
- `glFilter`, `wrapMode` → 每通道独立设置
- `scale`, `fcMod`, `alias` → 缩放/帧计数/别名

#### 更新方法

**`addPass()`** 新增可选参数：
```cpp
bool addPass(const std::string& vert, const std::string& frag,
             GLenum filter = GL_NEAREST,
             GLenum wrapMode = GL_CLAMP_TO_EDGE,
             const ShaderPassScale& scale = ShaderPassScale{},
             int fcMod = 0,
             const std::string& alias = {});
```
向后兼容（所有新参数均有默认值）。

**`addLut()`** 新增方法，使用 stb_image 加载图片并创建 GL 纹理。

**`_initPassFbo()`** 现在根据 `ShaderPassScale` 计算实际 FBO 尺寸：
- `SOURCE`：`outW = (int)(srcW * scale.scaleX)`
- `ABSOLUTE`：`outW = scale.absX`
- `VIEWPORT`：`outW = (int)(viewW * scale.scaleX)`

**`_lookupUniforms()`** 新增查询：
- `InputSize`、`FinalViewportSize` uniform 位置

**`run()`** 更新：
- 按通道绑定 LUT 纹理（GL_TEXTURE1 起）
- 传递 `InputSize`、`FinalViewportSize` uniform
- `FrameCount` 支持取模（`fcMod > 0`）
- 渲染完毕后正确解绑 LUT 纹理单元

---

## 兼容性矩阵

| 着色器类型 | 修改前 | 修改后 |
|-----------|--------|--------|
| 含 `#pragma parameter` 的着色器 | ❌ 编译失败 | ✅ 正常运行 |
| 使用 `COMPAT_ATTRIBUTE/VARYING` 的着色器 | ❌ 未定义符号 | ✅ 宏已定义 |
| 使用 `COMPAT_TEXTURE` 的着色器 | ❌ 未定义符号 | ✅ 宏已定义 |
| 使用 `PARAMETER_UNIFORM` 开关的着色器 | ❌ 只用常量模式 | ✅ 启用 uniform 模式 |
| 使用 LUT 纹理的 .glslp 预设 | ❌ 纹理缺失 | ✅ 自动加载并绑定 |
| 使用 `scale_type` 的多通道预设 | ❌ 所有通道同尺寸 | ✅ 按配置缩放 |
| 使用 `wrap_mode` 的预设 | ❌ 强制 CLAMP | ✅ 按配置设置 |
| 使用 `#reference` 的简单预设 | ❌ 解析失败 | ✅ 递归跟随引用 |
| 现有 `#pragma stage` 格式着色器 | ✅ | ✅ 不受影响 |
| 现有 `#if defined(VERTEX)` 格式 | ✅ | ✅ 不受影响 |

---

## 修改的文件列表

| 文件 | 变更类型 | 说明 |
|------|---------|------|
| `src/Game/GlslpLoader.cpp` | 重构 | 添加 #pragma parameter 剥除、COMPAT_* 宏、#reference、scale、LUT |
| `include/Game/GlslpLoader.hpp` | 更新注释 | 同步文档说明 |
| `include/Game/ShaderChain.hpp` | 扩展接口 | 新增 ShaderPassScale/ShaderLut 结构及 addLut/addPass 签名 |
| `src/Game/ShaderChain.cpp` | 功能扩展 | 实现 addLut、per-pass scale/wrap/fcMod、InputSize/FinalVpSize |

---

## 向后兼容性

- `ShaderChain::addPass(vert, frag)` 调用（无额外参数）完全兼容
- `ShaderChain::setFilter()` 接口不变
- `GlslpLoader::loadGlslIntoChain()` / `loadGlslpIntoChain()` 接口不变
- 现有配置文件无需修改

---

## 已知局限（可在后续版本改进）

1. **帧历史（Prev 帧）**：`shader_glsl.c` 支持 `Prev0..Prev6` 纹理（前 N 帧），本版本不实现
2. **通道间引用（pass 别名采样）**：如 `PassName`（别名）被后续通道当 sampler 使用，本版本未实现
3. **sRGB/float FBO**：`srgb_framebuffer` / `float_framebuffer` 标志未处理
4. **视口尺寸**：`VIEWPORT` 缩放类型目前与 `SOURCE` 相同处理
5. **`#pragma parameter` UI**：参数虽可作为 uniform 传入，但暂无 UI 调节界面
