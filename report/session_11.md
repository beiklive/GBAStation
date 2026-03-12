# 着色器全白画面修复报告

## 问题现象

加载多通道 `.glslp` 着色器预设（如 lcd-shader-gba-color.glslp）后，着色器可正常加载且 FBO 创建成功，但游戏画面全白。

日志示例：
```
[ShaderChain] Initialised, built-in pass 0 ready
[ShaderChain] User passes cleared
[ShaderChain] User pass 1 added (alias='')
[GlslpLoader] Loaded pass 0: F:\shaders\handheld\shaders\lcd-shader\lcd-pass-0.glsl
...（共 5 个通道加载成功）
[ShaderChain] FBO created: 240×160, tex=17, fbo=1
...（共 6 个 FBO 创建成功）
```

---

## 根本原因分析

### 主要原因：`PARAMETER_UNIFORM` 宏导致参数 uniform 默认为 0

兼容性头（`wrapVertSource` / `wrapFragSource`）注入了：

```glsl
#define PARAMETER_UNIFORM
```

RetroArch 着色器（如 lcd-shader）使用以下模式声明可调参数：

```glsl
#pragma parameter BRIGHTEST_WHITE "Brightest White" 1.0 0.0 1.0 0.05
#ifdef PARAMETER_UNIFORM
    uniform COMPAT_PRECISION float BRIGHTEST_WHITE;
#else
    #define BRIGHTEST_WHITE 1.0   // 硬编码默认值
#endif
```

由于 `PARAMETER_UNIFORM` 被定义，着色器进入 `#ifdef` 分支，将 `BRIGHTEST_WHITE` 声明为 `uniform float`。
但代码中 **从未调用 `glUniform1f` 为这些 uniform 赋值**，导致其默认为 **0.0**。

当着色器内部对该值执行除法时：
```glsl
float white_level = 1.0 / BRIGHTEST_WHITE;  // = 1/0 = INF（无穷大）
fragColor = vec4(min(color.rgb * white_level, 1.0), 1.0);
// = vec4(min(color * INF, 1.0), 1.0)
// = vec4(1.0, 1.0, 1.0, 1.0)  // 全白！
```

### 次要原因：`SourceSize` 未使用实际输入纹理尺寸

在多通道渲染中，`run()` 始终将 `SourceSize` 设为原始视频尺寸 `videoW × videoH`，即使某个中间通道的 FBO 已使用不同尺寸（如 `scale_type = viewport`）。应为每个通道使用其实际输入纹理的尺寸。

### 次要原因：旧式 `TextureSize vec2` uniform 的 `glUniform4f` 类型不匹配

部分旧式着色器（如 hqx）将 `TextureSize` 声明为 `vec2` 而非 `vec4`。
对 `vec2` uniform 调用 `glUniform4f` 会产生 `GL_INVALID_OPERATION`，导致 `TextureSize` 保持为 `(0,0)`，进而引发 `1/0 = INF` 的计算错误。

---

## 修复方案

### 1. 解析 `#pragma parameter` 默认值并设置 uniform（主要修复）

**新增函数** `extractParamDefaults(src)` 在着色器编译前解析所有 `#pragma parameter` 行，提取参数名及默认值。

**修改 `ShaderChain::addPass()`**，新增 `paramDefaults` 参数：
- 链接 GLSL 程序后，遍历参数映射，将每个参数 uniform 初始化为其默认值
- 保持向下兼容（默认参数为空映射）

**修改 `GlslpLoader::loadGlslpIntoChain()` 和 `loadGlslIntoChain()`**：
- 读取着色器原始源码
- 提取参数默认值
- 合并 `.glslp` 预设文件中的全局参数覆盖（`parameters = "PARAM1;PARAM2"` 及对应的 `PARAM1 = value`）
- 将合并后的默认值传递给 `chain.addPass()`

### 2. 每通道使用正确的 `SourceSize`（次要修复）

`run()` 循环中新增 `inputW` / `inputH` 变量，每个通道使用实际输入纹理的尺寸，每轮迭代更新为前一通道的 FBO 输出尺寸。

### 3. 正确处理 `vec2 TextureSize`（次要修复）

`ShaderPass` 新增字段 `sourceSizeIsVec2`：
- `_lookupUniforms()` 使用 `glGetActiveUniform` 检测 `TextureSize` uniform 的实际类型
- `run()` 根据该标志选择 `glUniform2f`（vec2）或 `glUniform4f`（vec4）

---

## 修改文件清单

| 文件 | 修改内容 |
|------|---------|
| `include/Game/ShaderChain.hpp` | 新增 `#include <unordered_map>`；`ShaderPass` 新增 `sourceSizeIsVec2` 字段；`addPass()` 新增 `paramDefaults` 参数 |
| `src/Game/ShaderChain.cpp` | `_lookupUniforms()` 增加 vec2/vec4 类型检测；`addPass()` 接收并设置参数默认值；`run()` 按通道追踪输入尺寸，并根据类型选择正确的 glUniform 调用 |
| `src/Game/GlslpLoader.cpp` | 新增 `extractParamDefaults()` 函数；`loadGlslIntoChain()` 提取并传递默认值；`loadGlslpIntoChain()` 提取每通道默认值并合并预设全局覆盖 |

---

## 验证

- 构建（Linux / Desktop 模式）：**通过**，无新增错误，仅有已有的无关告警
- 逻辑验证：lcd-shader 中 `BRIGHTEST_WHITE` 默认值 1.0 将被正确设置，避免除零
- hqx 着色器：`TextureSize` 的 vec2 类型将被正确检测并使用 `glUniform2f`
