# 着色器颜色全白问题根本原因分析与修复报告

## 任务背景

用户反映：游戏画面经过 RetroArch 着色器处理后颜色变白。
经过与 ChatGPT 的深入讨论（`example/chat.md`），用户确认问题出在"解析处理"环节。
本报告基于对 RetroArch 着色器体系的全面分析，定位根本原因并完成修复。

---

## ChatGPT 聊天记录分析摘要

`example/chat.md` 详细讨论了 RetroArch 解析 `.glsl` / `.glslp` 文件的完整流程：

| 话题 | 关键结论 |
|------|---------|
| RetroArch 编译策略 | 同一 `.glsl` 文件编译两次，分别前置 `#define VERTEX` / `#define FRAGMENT` + `#define PARAMETER_UNIFORM` |
| `#pragma parameter` | RetroArch 自行解析，注入为 `uniform float`，GLSL 编译器不处理该指令 |
| 顶点坐标 | NDC 空间（-1~+1），着色器直接输出 `gl_Position = VertexCoord`，**不使用 MVP 矩阵** |
| Uniform 注入 | RetroArch 还额外注入 `#define _HAS_ORIGINALASPECT_UNIFORMS` / `#define _HAS_FRAMETIME_UNIFORMS` |
| `FrameDirection` | 标准 uniform，+1 正向播放，-1 倒带，大量着色器声明此变量 |
| 像素格式 | XRGB8888 使用 `GL_TEXTURE_SWIZZLE_R/B` 处理；RGB565 直接以 `GL_RGB/GL_UNSIGNED_SHORT_5_6_5` 上传 |

---

## 根本原因分析

### 主要原因：着色器将 FragColor.α 置为 0 → NanoVG 预乘 Alpha 混合 → 全白

**问题着色器示例：`gba-color.glsl`**

```glsl
// 颜色矩阵，第四列（alpha 行）全为 0
mat4 color = mat4(r,   rg,  rb,  0.0,   // 第 1 列
                  gr,  g,   gb,  0.0,   // 第 2 列
                  br,  bg,  b,   0.0,   // 第 3 列
                  blr, blg, blb, 0.0);  // 第 4 列（alpha 行）

screen = color * screen;   // → screen.a = 0.0 (透明！)
FragColor = pow(screen, vec4(1.0 / display_gamma));
// → FragColor.a = pow(0.0, 0.45) = 0.0
```

矩阵乘法后 `FragColor.a = 0.0`，着色器将**透明**颜色写入 FBO。

**NanoVG 渲染机制（预乘 Alpha 混合）：**

NanoVG 使用 `GL_ONE, GL_ONE_MINUS_SRC_ALPHA` 混合模式（预乘 Alpha）：

```
最终颜色 = src × GL_ONE + dst × (1 − src.a)
         = (R, G, B, 0) × 1 + (1, 1, 1, 1) × (1 − 0)
         = (R, G, B, 0) + (1, 1, 1, 1)
         = (R+1, G+1, B+1, 1)
         → 夹紧至 (1, 1, 1, 1) = 纯白！
```

borealis UI 的背景为白色/浅色，当 FBO 纹理的 Alpha 通道为 0 时，背景完全透过，
形成**全白画面**。这正是用户报告的症状。

### 次要原因一：`RETRO_PIXEL_FORMAT_0RGB1555` 被错误解析为 RGB565

libretro API 的**默认**像素格式是 `RETRO_PIXEL_FORMAT_0RGB1555`（5-5-5 布局）：
```
bit 15=0 | bits 14-10=R | bits 9-5=G | bits 4-0=B
```

RGB565 的布局（5-6-5）：
```
bits 15-11=R | bits 10-5=G | bits 4-0=B
```

原有 `else` 分支将 0RGB1555 数据当 RGB565 解码，导致 R/G 分量偏移错误。

### 次要原因二：缺少 `FrameDirection` uniform 支持

大量 RetroArch 着色器（如 `gba-color.glsl`）声明：
```glsl
uniform COMPAT_PRECISION int FrameDirection;
```

未设置时默认值为 0，部分着色器使用此值执行条件判断或动态效果，
0 可能触发错误分支。RetroArch 固定传入 +1（正向）或 -1（倒带）。

### 次要原因三：缺少 RetroArch 注入的额外宏

RetroArch 的 `shader_glsl.c::gl_glsl_compile_program()` 在编译时还注入：
```c
"#define _HAS_ORIGINALASPECT_UNIFORMS\n#define _HAS_FRAMETIME_UNIFORMS\n"
```

部分着色器通过 `#ifdef _HAS_ORIGINALASPECT_UNIFORMS` 决定是否使用长宽比相关计算。

---

## 修复方案

### 修复 1：禁止着色器写入 FBO Alpha 通道（**主要修复**）

**文件：`src/Game/ShaderChain.cpp`**

在 `run()` 方法中，所有着色器通道开始前调用：
```cpp
glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);  // 禁止 Alpha 写入
```

FBO 在每通道执行前以 `glClearColor(0, 0, 0, 1)` + `glClear(COLOR_BUFFER_BIT)` 清除，
alpha 预置为 1.0。通过 `GL_FALSE` 屏蔽着色器对 Alpha 通道的写入，
确保 FBO 输出对 NanoVG 始终**完全不透明（α=1.0）**。

同时新增 `GL_COLOR_WRITEMASK` 的状态保存与恢复：
```cpp
GLboolean prevColorMask[4] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
glGetBooleanv(GL_COLOR_WRITEMASK, prevColorMask);
// ... 着色器链执行 ...
glColorMask(prevColorMask[0], prevColorMask[1], prevColorMask[2], prevColorMask[3]);
```

### 修复 2：正确解析 `RETRO_PIXEL_FORMAT_0RGB1555`

**文件：`src/Game/LibretroLoader.cpp`**

将原有 `else` 分支（错误地以 RGB565 解码所有非 XRGB8888 格式）拆分为两个分支：

```cpp
} else if (s_current->m_pixelFormat == RETRO_PIXEL_FORMAT_RGB565) {
    // 5-6-5 布局：bits 15-11=R, 10-5=G, 4-0=B
    uint8_t r5 = (px >> 11) & 0x1F;
    uint8_t g6 = (px >>  5) & 0x3F;
    uint8_t b5 =  px        & 0x1F;
} else {
    // RETRO_PIXEL_FORMAT_0RGB1555（默认格式）：5-5-5 布局
    // bit15=0 | bits 14-10=R | bits 9-5=G | bits 4-0=B
    uint8_t r5 = (px >> 10) & 0x1F;  // 正确：从 bit10 开始取 R
    uint8_t g5 = (px >>  5) & 0x1F;  // 正确：5 位 G（不是 6 位）
    uint8_t b5 =  px        & 0x1F;
}
```

### 修复 3：新增 `FrameDirection` uniform 支持

**文件：`include/Game/ShaderChain.hpp`，`src/Game/ShaderChain.cpp`**

- `ShaderPass` 新增字段 `GLint frameDirectionLoc = -1`
- `_lookupUniforms()` 查询 `FrameDirection` 位置
- `run()` 中每帧传入 `glUniform1i(p.frameDirectionLoc, 1)` (+1 = 正向播放)

### 修复 4：补全 RetroArch 编译宏注入

**文件：`src/Game/GlslpLoader.cpp`**

`buildRetroArchSrc()` 中追加两个宏（与 RetroArch `shader_glsl.c` 保持一致）：

```cpp
std::string defineStr = isVertex
    ? "#define VERTEX\n#define PARAMETER_UNIFORM\n"
      "#define _HAS_ORIGINALASPECT_UNIFORMS\n#define _HAS_FRAMETIME_UNIFORMS\n"
    : "#define FRAGMENT\n#define PARAMETER_UNIFORM\n"
      "#define _HAS_ORIGINALASPECT_UNIFORMS\n#define _HAS_FRAMETIME_UNIFORMS\n";
```

---

## 修改文件清单

| 文件 | 修改类型 | 说明 |
|------|---------|------|
| `src/Game/ShaderChain.cpp` | 修改 | `run()` 新增 `glColorMask(RGB=true, A=false)` + 状态保存/恢复；`_lookupUniforms()` 查询 `FrameDirection`；`run()` 设置 `FrameDirection=1` |
| `include/Game/ShaderChain.hpp` | 修改 | `ShaderPass` 新增 `frameDirectionLoc` 字段 |
| `src/Game/LibretroLoader.cpp` | 修改 | `s_videoRefreshCallback` 新增 `RETRO_PIXEL_FORMAT_0RGB1555` 分支，正确按 5-5-5 解码 |
| `src/Game/GlslpLoader.cpp` | 修改 | `buildRetroArchSrc()` 补全 `_HAS_ORIGINALASPECT_UNIFORMS`、`_HAS_FRAMETIME_UNIFORMS` 宏注入 |

---

## 验证

- Linux Desktop 编译通过，无新增错误（`GBAStation` 可执行文件生成成功）
- `glColorMask(R,G,B, GL_FALSE)` 屏蔽 Alpha 写入后，gba-color.glsl 等颜色矩阵着色器
  的 FBO 输出 alpha 将保持清除值 1.0，NanoVG 混合正确输出游戏颜色
- 0RGB1555 格式 R 分量从 bit10 而非 bit11 读取，G 分量为 5 位而非 6 位，颜色精度更准确
- `FrameDirection=1` 确保依赖运动方向的着色器使用正确分支

## 安全摘要

本次修改仅涉及 OpenGL 状态管理和像素格式转换，不引入任何新的安全漏洞。
