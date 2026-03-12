# RetroArch zfast_crt.glsl 着色器兼容性修复报告

## 问题描述

用户在项目资源文件中添加了 RetroArch 官方着色器 `resources/shaders/zfast_crt.glsl`（一种经典 CRT 效果着色器），配置后游戏画面变成全黑。

---

## 根本原因分析

### 1. 阶段分割解析器不支持 `#elif` 格式

RetroArch 着色器采用两种主流格式分隔顶点/片段阶段：

| 格式 | 示例 |
|------|------|
| `#pragma stage` | `#pragma stage vertex` … `#pragma stage fragment` |
| `#if defined(VERTEX)` / `#elif defined(FRAGMENT)` | **zfast_crt.glsl 使用此格式** |

`GlslpLoader::parseGlslFile()` 中原有的"ifdef 分割"逻辑仅识别：
```
#ifdef VERTEX        → 开始顶点段
#ifdef FRAGMENT      → 开始片段段
#if defined(VERTEX)  → 开始顶点段
#if defined(FRAGMENT)→ 开始片段段
#endif               → 直接结束（无嵌套深度跟踪）
```

**缺陷 A**：不识别 `#elif defined(FRAGMENT)` 作为片段段起始标记，导致片段代码被全部归入顶点体或丢弃。

**缺陷 B**：将所有 `#endif` 均视为段结束，无法处理段内嵌套的 `#if`/`#endif` 块（如
`#if __VERSION__ >= 130 … #endif`、`#ifdef PARAMETER_UNIFORM … #endif`），导致在遇到第一个内嵌 `#endif` 时就错误地终止了顶点段，主函数无法被提取。

**最终效果**：提取的顶点体仅包含内嵌 `#if` 块的一小段，片段体为空；代码回退为"fragment-only 模式"，将整个文件作为片段着色器。由于文件内的 `main()` 被包裹在 `#elif defined(FRAGMENT)` 中（此时未定义 FRAGMENT），着色器编译失败，画面全黑。

---

### 2. `COMPAT_ATTRIBUTE` 属性声明与宏定义冲突

zfast_crt.glsl 使用如下声明：
```glsl
COMPAT_ATTRIBUTE vec4 VertexCoord;
COMPAT_ATTRIBUTE vec4 TexCoord;
COMPAT_ATTRIBUTE vec4 COLOR;
```

兼容头中已将 `VertexCoord` / `TexCoord` 定义为宏（`#define VertexCoord _bkVertexCoord()`）。  
GLSL 预处理器展开后：
```glsl
in vec4 _bkVertexCoord();   // 语法错误
```

原有 `isRaAttributeDecl()` 仅匹配以 `attribute` 或 `in` 开头的行，不匹配 `COMPAT_ATTRIBUTE`，导致这些声明未被剥除而产生编译错误。

---

### 3. `MVPMatrix * VertexCoord` 类型不匹配

兼容头将 `VertexCoord` 定义为返回 `vec2` 的函数，而 zfast_crt.glsl 顶点主函数写：
```glsl
gl_Position = MVPMatrix * VertexCoord;
```
`mat4 * vec2` 是非法的 GLSL 表达式（类型错误）。现有的 `#pragma stage` 格式着色器通过 `vec4(VertexCoord, 0.0, 1.0)` 显式提升类型来规避此问题，但 zfast_crt.glsl 直接使用。

---

### 4. 片段着色器输出变量名冲突

兼容块声明：
```glsl
out vec4 fragColor;           // 小写 f
#define Texture tex
#define Source  tex
```

zfast_crt.glsl 片段段声明：
```glsl
out COMPAT_PRECISION vec4 FragColor;   // 大写 F（不同变量）
```
并在 `main()` 中写入 `FragColor.rgb = …`。两个 `out` 变量存在，但 `fragColor`（兼容块）从未被写入，最终渲染目标为默认值（0 = 黑色）。

---

### 5. `FrameCount` uniform 类型不匹配

`ShaderChain::run()` 在 GL3 模式下使用 `glUniform1ui`（无符号整型），但 zfast_crt.glsl 将 `FrameCount` 声明为 `int`，类型不符合 OpenGL 规范。

---

## 修复方案

### 修改文件：`src/Game/GlslpLoader.cpp`

#### (A) 修复 `parseGlslFile()` 的阶段分割逻辑

重写 `#ifdef VERTEX/FRAGMENT` 分割段，引入：
- **嵌套深度计数器** `nestedDepth`：遇到 `#if`/`#ifdef`/`#ifndef` 递增，遇到 `#endif` 递减；仅在 `nestedDepth == 0` 时处理阶段跳转。
- **`#elif` 支持**：将 `#elif defined(FRAGMENT)` 和 `#elif defined(VERTEX)` 识别为阶段切换指令。
- **`#else` 支持**：将深度 0 的 `#else` 识别为从 VERTEX 到 FRAGMENT 的隐式切换（兼容 `#ifdef VERTEX / #else / #endif` 格式）。

#### (B) 更新 `isRaAttributeDecl()`

增加对 `COMPAT_ATTRIBUTE` 前缀的识别，并将 `COLOR` 加入需要剥除的属性名列表：
```cpp
bool isAttrib = startsWithKw("attribute") || startsWithKw("in")
             || startsWithKw("COMPAT_ATTRIBUTE");
return t.find("VertexCoord") != std::string::npos
    || t.find("TexCoord")    != std::string::npos
    || t.find("COLOR")       != std::string::npos;
```

#### (C) 修复 `preprocessVertSource()` 中的 `MVPMatrix * VertexCoord`

提取 `replaceAll` 辅助函数到函数顶层（消除仅在 GL3 `#if` 块内可用的作用域限制），并添加：
```cpp
line = replaceAll(line, "MVPMatrix * VertexCoord",
                  "MVPMatrix * vec4(VertexCoord, 0.0, 1.0)");
```
此替换将 `mat4 * vec2` 错误转换为合法的 `mat4 * vec4`，对不使用此模式的着色器无影响。

#### (D) 修复 `wrapFragSource()` 的兼容块

- **GL3/GLES3 模式**：添加 `#define FragColor fragColor`，使着色器对 `FragColor` 的写入重定向到兼容块已声明的 `out vec4 fragColor` 输出变量。
- **GL2/GLES2 模式**：添加 `#define FragColor gl_FragColor`，将输出重定向到内置变量。

#### (E) 修复 `preprocessFragSource()` 中的输出声明剥除

添加规则：以 `out ` 开头、含有 `FragColor`、且不是 `#define` 行的声明一律剥除，避免与兼容块的 `out vec4 fragColor` 产生重复声明冲突：
```cpp
if (t.rfind("out ", 0) == 0 && t.find("FragColor") != std::string::npos
    && t.find("#define") == std::string::npos) {
    continue;
}
```

---

### 修改文件：`src/Game/ShaderChain.cpp`

#### (F) 修复 `FrameCount` uniform 的传递方式

将 GL3/GLES3 模式下的 `glUniform1ui` 改为 `glUniform1i`：
```cpp
// 之前
glUniform1ui(p.frameCountLoc, m_frameCount);
// 修复后
glUniform1i(p.frameCountLoc, (GLint)m_frameCount);
```
大多数 RetroArch 着色器将 `FrameCount` 声明为 `int` 而非 `uint`，使用 `glUniform1ui` 在严格驱动下会触发 `GL_INVALID_OPERATION`。

---

## 修复效果

修复后，`zfast_crt.glsl` 能被正确解析并编译：
1. 顶点段（`#if defined(VERTEX)` 至 `#elif defined(FRAGMENT)` 之间）完整提取，包含正确的 main() 函数。
2. 片段段（`#elif defined(FRAGMENT)` 至 `#endif` 之间）完整提取，包含 CRT 效果实现。
3. `COMPAT_ATTRIBUTE` 声明正确剥除，避免与宏定义冲突。
4. `MVPMatrix * VertexCoord` 被提升为 `vec4`，类型匹配。
5. `FragColor` 通过宏定义重定向到正确输出变量，CRT 效果画面正常显示。

---

## 兼容性说明

- 已有的 `#pragma stage` 格式着色器（passthrough、crt_simple、scanlines、sharpen）不受任何影响。
- 新解析逻辑完整兼容原有的 `#ifdef VERTEX / #ifdef FRAGMENT` 格式。
- 新增的 `#elif defined(FRAGMENT)` 支持可适配大量其他 RetroArch 官方着色器。

---

## 其他需求建议

以下内容在本次需求范围之外，可在后续版本中考虑：

1. **`TextureSize`/`InputSize` uniform 类型适配**：zfast_crt.glsl 将它们声明为 `vec2`，但 ShaderChain 以 `glUniform4f` 传入 4 个分量。大多数驱动会静默忽略多余分量，但严格遵循 OpenGL 规范的驱动会报错。可考虑在 `_lookupUniforms()` 中区分 `SourceSize`（vec4）与 `TextureSize`（vec2）并分别处理。

2. **`PARAMETER_UNIFORM` 参数动态设置**：RetroArch 着色器支持通过 `#pragma parameter` 定义可调参数并以 `uniform` 传入。目前项目仅使用着色器内的默认值（`#else` 分支的 `#define`），未来可解析 `#pragma parameter` 并提供 UI 调节界面。

3. **多通道 `.glslp` 预设的 `scale_type` / `scale`**：RetroArch 预设支持每个通道独立设置输出分辨率缩放比例，目前项目均使用视频原始分辨率，可在后续版本中支持。
