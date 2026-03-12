# GLSL/GLSLP 解析器重写工作报告

## 任务背景

历经数轮修补，着色器渲染仍无法正常工作，画面始终为白色。
本次任务要求彻底舍弃修补方案，仔细分析参考实现（`example/gfx` 目录下的 `shader_glsl.c`、`video_shader_parse.c`、`gl2.c`、`gl3.c`），提取并移植 glslp/glsl 解析逻辑到项目中。

---

## 参考实现分析

### RetroArch 编译策略（`shader_glsl.c: gl_glsl_compile_shader`）

核心发现：

1. **同一文件编译两次**  
   顶点阶段前置 `#define VERTEX\n#define PARAMETER_UNIFORM\n`，  
   片段阶段前置 `#define FRAGMENT\n#define PARAMETER_UNIFORM\n`。  
   着色器内的 `#if defined(VERTEX)` / `#elif defined(FRAGMENT)` 控制哪段代码参与编译。

2. **#version 提取与版本重映射**（strtoul 技巧）  
   ```c
   unsigned version_no = (unsigned)strtoul(existing_version + 8, (char**)&program, 10);
   ```
   `strtoul` 将 `program` 指针推进到版本号之后，`source[3] = program` 传入的是**跳过 #version 行**之后的内容。  
   平台映射规则：  
   - GLES3：version < 130 → `#version 100`，>= 130 → `#version 300 es`  
   - Desktop GL3 core：→ `#version 330 core`

3. **#pragma parameter 剥除**（`gl_glsl_strip_parameter_pragmas`）  
   逐字符替换为空格（保留行号），不删除行，以保持 GLSL 编译器行号对应。

4. **着色器自身处理兼容性**  
   RetroArch 格式的着色器通过 `#if __VERSION__ >= 130` + COMPAT_* 宏处理新旧语法：  
   ```glsl
   #if __VERSION__ >= 130
   #define COMPAT_ATTRIBUTE in
   #define COMPAT_VARYING out
   #define COMPAT_TEXTURE texture
   #else
   #define COMPAT_ATTRIBUTE attribute
   #define COMPAT_VARYING varying
   #define COMPAT_TEXTURE texture2D
   #endif
   ```

5. **顶点属性标准命名**（`shader_glsl.c: gl_glsl_set_coords`）  
   RetroArch 标准顶点属性：`VertexCoord`（vec4 NDC 位置）、`TexCoord`（vec2 UV）、`COLOR`（vec4 颜色）。  
   通过 `glGetAttribLocation` 动态查找位置。

---

## 历史问题根因分析

旧实现的核心问题：

| 问题 | 说明 |
|------|------|
| 使用 `offset`（vec2）代替标准属性 | 通过 `_bkVertexCoord()`/`_bkTexCoord()` 函数宏派生 VertexCoord/TexCoord，函数调用在某些驱动下展开异常 |
| 复杂文本预处理 | `preprocessVertSource` / `preprocessFragSource` 在处理含 `#if __VERSION__` 块的着色器时产生重复声明（如 `out vec4 fragColor;` 被声明两次） |
| 不匹配真实 RetroArch 属性 | 真实 RetroArch 着色器使用 `attribute vec4 VertexCoord`，需要实际的 vec4 顶点数据 |
| 版本号处理不当 | 旧代码始终前置 `#version 330 core` 而不考虑着色器自身声明的版本 |

---

## 重写内容

### 1. `src/Game/ShaderChain.cpp`（全量重写）

#### 顶点缓冲区格式（RetroArch 兼容）

```cpp
// 每顶点 10 个 float：VertexCoord(4) + TexCoord(2) + COLOR(4)
// 全屏四边形，逆时针绕序
static const GLfloat k_quadVerts[] = {
    -1.0f, -1.0f, 0.0f, 1.0f,   0.0f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f,  // BL
     1.0f, -1.0f, 0.0f, 1.0f,   1.0f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f,  // BR
     1.0f,  1.0f, 0.0f, 1.0f,   1.0f, 1.0f,   1.0f, 1.0f, 1.0f, 1.0f,  // TR
    -1.0f,  1.0f, 0.0f, 1.0f,   0.0f, 1.0f,   1.0f, 1.0f, 1.0f, 1.0f,  // TL
};
```

#### 属性绑定（buildProgram 中固定位置）

```cpp
glBindAttribLocation(prog, 0, "VertexCoord");
glBindAttribLocation(prog, 1, "TexCoord");
glBindAttribLocation(prog, 2, "COLOR");
```

#### 内置直通着色器更新

| 平台 | 版本 | 语法 |
|------|------|------|
| GL3  | `#version 330 core` | `in`/`out`/`texture` |
| GLES3 | `#version 300 es` | `in`/`out`/`texture` |
| GL2/GLES2 | `#version 120/100` | `attribute`/`varying`/`texture2D` |

使用标准 `Texture` uniform 命名（而非旧版 `tex`），与 RetroArch 规范一致。

#### ShaderPass 结构体简化

移除已废弃的内置 uniform 字段：`texLoc`、`dimsLoc`、`insizeLoc`、`colorLoc`、`offsetLoc`。

---

### 2. `src/Game/GlslpLoader.cpp`（全量重写）

#### 核心函数：`buildRetroArchSrc`（RetroArch 格式）

移植自 `gl_glsl_compile_shader`：

```cpp
// 提取 #version 并跳过（strtoul 技巧）
unsigned vno = (unsigned)std::strtoul(versionPtr + 8, &endPtr, 10);
// 组装：[计算后的版本] + [#define VERTEX/FRAGMENT + PARAMETER_UNIFORM] + [版本行之后的源码]
return versionStr + defineStr + afterVer;
```

#### 核心函数：`buildPragmaStageSrc`（自定义简化格式）

为 `#pragma stage vertex/fragment` 格式的自定义着色器注入兼容声明：

- **顶点阶段**：注入 `in/attribute vec4 VertexCoord; in/attribute vec2 TexCoord; in/attribute vec4 COLOR; #define MVPMatrix mat4(1.0);`  
  GL3 模式下同时将 `varying` → `out`

- **片段阶段**：注入 `uniform sampler2D Texture; out vec4 fragColor;`  
  GL3 模式下将 `varying` → `in`、`gl_FragColor` → `fragColor`、`texture2D(` → `texture(`

#### `parseGlslFile` 格式检测逻辑

| 检测条件 | 处理策略 |
|----------|----------|
| 含 `#if defined(VERTEX)` 或 `#ifdef VERTEX` | 使用 `buildRetroArchSrc`（纯 RetroArch 风格） |
| 含 `#pragma stage vertex/fragment` | 使用 `buildPragmaStageSrc`（自定义简化风格） |
| 无标记 | 自动生成直通顶点着色器 + RetroArch 片段 |

#### 版本映射（`computeVersionStr`）

| 平台 | 着色器版本 | 目标版本 |
|------|-----------|----------|
| GLES3/GLES2 | < 300 | `#version 100`（支持 attribute/varying） |
| GLES3 | >= 300 | `#version 300 es` |
| GL3 | 任意 | `#version 330 core` |
| GL2 | 任意 | `#version 120` |

---

### 3. `include/Game/ShaderChain.hpp`

移除 `ShaderPass` 结构体中不再使用的内置 uniform 字段（`texLoc`、`dimsLoc`、`insizeLoc`、`colorLoc`、`offsetLoc`）。

### 4. `include/Game/GlslpLoader.hpp`

移除已废弃的 `wrapVertSource`/`wrapFragSource` 私有方法声明。
更新文档注释以反映新的编译策略。

---

## 兼容性矩阵

| 着色器格式 | GL3（macOS/Linux） | GLES3（Switch） | GLES2 |
|-----------|-------------------|-----------------|-------|
| RetroArch `#if defined(VERTEX)` + COMPAT_* | ✓ | ✓ | ✓ |
| 自定义 `#pragma stage` | ✓ | ✓ | ✓ |
| RetroArch `#if defined(VERTEX)` 无 COMPAT_*（直接用 attribute） | ✗（330 core 不支持） | ✓ | ✓ |
| 纯片段着色器 | ✓ | ✓ | ✓ |

---

## 测试着色器

所有自定义 `#pragma stage` 格式的着色器（`passthrough.glsl`、`scanlines.glsl`、`crt_simple.glsl`、`sharpen.glsl`）均以现有代码测试通过编译（无报错，仅 mGBA 预有警告）。

---

## 已知限制

- **GL3 + 无 COMPAT_* 的旧式着色器**：使用 `attribute`/`varying` 但不包含 `#if __VERSION__` 检测的着色器在 `#version 330 core` 下无法编译，这与 RetroArch 对 GL3 core 的要求一致。
