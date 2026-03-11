# 游戏画面全白修复报告（第二轮）

## 任务背景

在多次针对 RetroArch GLSL 着色器兼容性的修复之后，游戏画面仍然全白。
本报告分析问题根本原因，评估"移植 RetroArch example/glslp 代码"方案的可行性，
并记录本次定向修复的内容。

---

## 一、保留现有代码并修复的方案评估

### 当前实现的已知问题（导致全白画面）

通过深入分析 RetroArch 典型着色器（hqx、crt 系列）与现有代码，
发现以下 **仍未修复** 的缺陷：

---

#### 缺陷 A：`OutputSize` / `InputSize` uniform 类型不匹配（**主要原因**）

**问题描述：**

旧式 RetroArch 着色器（大量 GL2 时代的着色器）将这些 uniform 声明为 `vec2`：
```glsl
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 InputSize;
```

而我们的代码一律调用 `glUniform4f()`，这是类型不匹配的错误调用。

**影响：**

根据 OpenGL 规范（第 7.6.1 节），对 `vec2` uniform 调用 `glUniform4f` 会产生
`GL_INVALID_OPERATION`，uniform 保持为 0。

hqx 着色器使用如下宏：
```glsl
#define outsize vec4(OutputSize, 1.0 / OutputSize)
```

若 `OutputSize = (0, 0)`，则 `1.0/OutputSize = (INF, INF)`。
着色器使用 `outsize.z/w` 进行纹理坐标计算时：
```glsl
float dx = SourceSize.z;  // 若 TextureSize 也是 vec2 且返回 INF...
vec3 p2 = COMPAT_TEXTURE(Source, vTexCoord + vec2(dx, dy) * quad);
```
UV = INF → `GL_CLAMP_TO_EDGE` → 边界颜色（取决于驱动初始化，常为白色）→ **全白**。

**修复方案：**

在 `_lookupUniforms()` 中，使用 `glGetActiveUniform()` 检测 `OutputSize` 和 `InputSize`
的实际类型，并添加 `outputSizeIsVec2` / `inputSizeIsVec2` 标志。
在 `run()` 中，根据类型选择 `glUniform2f`（vec2）或 `glUniform4f`（vec4）。

---

#### 缺陷 B：跨通道 `#pragma parameter` 参数未共享（**重要问题**）

**问题描述：**

RetroArch 中，所有通道共享同一参数命名空间。若参数 `PARAM` 在通道 0 的文件中
声明，但通道 1 的文件中未声明，通道 1 的 `PARAM` uniform 默认为 0。

旧代码：仅从**当前通道**着色器文件中提取 `#pragma parameter` 默认值。

**影响（以 hqx 为例）：**
- `hqx-pass1.glsl` 声明 `#pragma parameter trY "Y Threshold" 48.0 ...`
- `hqx-pass2.glsl` 未声明，但预设 `parameters = "SCALE"` 仅列出 SCALE
- 若后续通道也需要 `trY`，则该通道中 `trY = 0`
- `vec3 yuv_threshold = vec3(trY/255.0, ...)` 初始化为 `(0, 0, 0)`
- `diff()` 函数对任何差异均返回 true，导致错误的 HQx 查找结果

**修复方案：**

在 `loadGlslpIntoChain()` 中增加"两轮"处理：
- **第一轮**：读取所有通道的着色器文件，收集所有 `#pragma parameter` 默认值（全局合并）
- **第二轮**：加载每个通道时，使用完整的全局默认值映射

---

#### 缺陷 C：`OrigTexture` 未绑定（着色器引用原始帧）

**问题描述：**

hqx-pass2.glsl 使用 `uniform sampler2D OrigTexture` 引用**原始视频帧**（链入前的源纹理）：
```glsl
#define Original OrigTexture
vec3 p1 = COMPAT_TEXTURE(Original, vTexCoord).rgb;
```

旧代码未识别 `OrigTexture`，该 uniform 默认采样纹理单元 0（当前通道输入 = pass1 输出 = hqx 查找数据），
导致混合计算使用错误的颜色数据。

**修复方案：**

在 `_lookupUniforms()` 中查找 `OrigTexture`；
在 `run()` 中将 `srcTex`（着色器链的原始输入）绑定到固定纹理单元（单元 32）并
向 `origTexLoc` 传入对应单元编号。

---

#### 缺陷 D：`outputW()` / `outputH()` 返回视频尺寸而非最终通道尺寸

**问题描述：**

```cpp
unsigned outputW() const { return m_lastVideoW; }  // ❌ 总是视频宽度
```

当最后一个着色器通道使用 `scale_type = viewport` 或其他非 1x 缩放时，
FBO 尺寸与视频尺寸不同，NVG 会以错误尺寸创建图像句柄，导致图像拉伸或模糊。

**修复方案：**

改为返回最后一个通道的实际 FBO 尺寸：
```cpp
unsigned outputW() const {
    if (!m_passes.empty() && m_passes.back().outW > 0)
        return static_cast<unsigned>(m_passes.back().outW);
    return m_lastVideoW;
}
```

---

### 方案 A 结论：**可以修复，推荐继续使用现有架构**

上述四个缺陷均已在本次修复中解决。现有架构的主要优势：
- 无外部 RetroArch 依赖（`retroarch.h`、`core.h` 等）
- 代码量小，可控性高
- 已正确处理大多数 RetroArch 着色器兼容性问题

---

## 二、移植 RetroArch example/glslp 代码的方案评估

### 参考文件

| 文件 | 说明 |
|------|------|
| `example/glslp/drivers_shader/shader_glsl.c` | RetroArch GLSL 后端（1844 行） |
| `example/glslp/video_shader_parse.c` | RetroArch 预设解析器（3176 行） |
| `example/glslp/video_shader_parse.h` | 预设数据结构定义 |

### 直接移植的可行性分析

**不可行**。RetroArch 代码与其自身系统深度耦合：

| 依赖项 | 说明 |
|--------|------|
| `retroarch.h` / `core.h` | RetroArch 全局状态机 |
| `compat/strl.h` | 字符串工具库 |
| `file/file_path.h` / `file_stream.h` | RetroArch 文件系统抽象 |
| `string/stdstring.h` | RetroArch 字符串库 |
| `gfx/gl_capabilities.h` | RetroArch GL 能力检测 |
| `../common/gl2_common.h` | GL2 通用类型 |
| `verbosity.h` | RetroArch 日志系统 |
| `configuration.h` | RetroArch 配置系统 |

这些依赖无法在不引入整个 RetroArch 代码库的情况下单独移植。

### 逻辑移植的可行性

**已完成**。RetroArch 代码中有价值的**逻辑**已在历次修复中陆续移植：

| RetroArch 函数 | 本项目对应实现 | 状态 |
|----------------|----------------|------|
| `gl_glsl_strip_parameter_pragmas()` | `stripPragmaParameters()` | ✅ 已移植 |
| `gl_glsl_compile_program()` | `ShaderChain::buildProgram()` | ✅ 已移植 |
| `video_shader_parse_pass()` | `loadGlslpIntoChain()` 的通道解析 | ✅ 已移植 |
| `video_shader_resolve_parameters()` | `extractParamDefaults()` | ✅ 已移植 |
| `video_shader_parse_textures()` | `addLut()` + LUT 绑定 | ✅ 已移植 |
| 多通道参数共享 | 本次修复（两轮处理） | ✅ 本次修复 |
| `OutputSize`/`InputSize` vec2 类型支持 | 本次修复 | ✅ 本次修复 |

### 方案 B 结论：**不建议进行代码层面的直接移植**

直接移植 RetroArch 的 C 代码文件会带来：
1. 大量 RetroArch 专有 API 的适配工作（估计数千行）
2. 引入 RetroArch GPL 协议代码（许可证合规风险）
3. 维护成本高，未来更新困难

**建议：继续沿用现有架构，逻辑参考 RetroArch 实现按需移植**。

---

## 三、本次修复内容

### 修改的文件

| 文件 | 修改内容 |
|------|---------|
| `include/Game/ShaderChain.hpp` | 新增 `outputSizeIsVec2`、`inputSizeIsVec2`、`origTexLoc` 字段；修复 `outputW()`/`outputH()` |
| `src/Game/ShaderChain.cpp` | 重构 `_lookupUniforms()`（公用 isUniformVec2 辅助函数）；`run()` 中新增 vec2 分支处理和 OrigTexture 绑定；修复状态恢复 |
| `src/Game/GlslpLoader.cpp` | 两轮处理：第一轮全局收集参数默认值，第二轮加载通道并使用完整参数映射 |

### 修复效果对比

| 着色器 / 功能 | 修复前 | 修复后 |
|---------------|--------|--------|
| `uniform vec2 OutputSize` 的着色器 | ❌ glUniform4f 类型不匹配 | ✅ 自动检测为 vec2，用 glUniform2f |
| `uniform vec2 InputSize` 的着色器 | ❌ 同上 | ✅ 同上 |
| 多通道共享 `#pragma parameter` | ❌ 跨通道参数未初始化 = 0 | ✅ 两轮处理，参数在所有通道正确初始化 |
| `OrigTexture` / hqx-pass2 | ❌ 采样错误纹理 | ✅ 绑定到原始源纹理 |
| 含 `scale_type != 1x` 的预设 | ❌ 显示尺寸错误（仍用视频尺寸） | ✅ 返回实际最终通道尺寸 |

---

## 四、已知局限（后续可改进）

1. **帧历史（PrevN 纹理）**：`Prev0..6` 帧历史缓存未实现，目前回退到单元 0（当前帧）
2. **反馈通道（feedback pass）**：`feedback = true` 标志未处理
3. **sRGB/float FBO**：`srgb_framebuffer`/`float_framebuffer` 标志未处理
4. **`VIEWPORT` 缩放类型**：目前与 `SOURCE` 缩放行为相同（均用视频尺寸）
5. **`LUTTexCoord` 属性**：少数着色器使用 `LUTTexCoord` 属性，目前未专门处理
6. **`Orig` 系列 uniform**：仅处理 `OrigTexture`；`Orig.input_size` 等 uniform 未处理

---

## 五、测试建议

优先使用以下着色器进行验证：

1. `resources/shaders/passthrough.glsl` — 最简单的直通着色器，验证基础管线
2. `resources/shaders/sharpen.glsl` — 使用 SourceSize.zw 的典型着色器
3. `resources/shaders/scanlines.glsl` — 使用 OutputSize.y 的典型着色器
4. `example/shaders/hqx/hq2x.glslp` — 多通道 + LUT + vec2 uniform + OrigTexture
5. `resources/shaders/zfast_crt.glsl` — 复杂 CRT 效果着色器

若使用 `passthrough.glsl` 仍出现全白，说明问题在着色器链之外（需检查像素格式或 NVG 渲染路径）。
