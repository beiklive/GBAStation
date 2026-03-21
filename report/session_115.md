# Session 115 工作汇报

## 任务分析

### 任务目标
对比 `example/gfx/video_shader_parse.c` 的渲染链，找出项目渲染链的缩放问题并修复。

### 输入
- 问题描述：参考 RetroArch 的 video_shader_parse.c 及 gl2/gl3 渲染链实现，找出项目着色器渲染链的缩放问题
- 涉及文件：`example/gfx/drivers/gl2.c`、`example/gfx/drivers/gl3.c`、`example/gfx/video_shader_parse.c`、`src/Game/game_view.cpp`

### 根因分析

#### 对比 RetroArch 渲染链的缩放计算

**RetroArch 的 viewport 缩放实现（gl2/gl3）：**

```c
// gl2.c:1901-1902
width  = gl->video_width;   // 完整游戏视图宽度（物理像素）
height = gl->video_height;  // 完整游戏视图高度（物理像素）

// gl2_renderchain_recompute_pass_sizes 中：
case RARCH_SCALE_VIEWPORT:
    fbo_rect->img_width = fbo_scale->scale_x * vp_width;   // vp_width = gl->video_width
    fbo_rect->img_height = fbo_scale->scale_y * vp_height;  // vp_height = gl->video_height
```

所有带 `scale_type = viewport` 的通道，其 FBO 尺寸 = **完整游戏视图物理像素 × 缩放系数**。

**项目修复前的实现：**

```cpp
// src/Game/game_view.cpp（修复前）
preRect = m_display.computeRect(x, y, width, height, m_texWidth, m_texHeight);
// passViewW/H = 游戏显示矩形（宽高比适配后的子区域，虚拟坐标）
unsigned passViewW = std::max(1u, static_cast<unsigned>(std::lround(preRect.w)));
unsigned passViewH = std::max(1u, static_cast<unsigned>(std::lround(preRect.h)));
```

- **对于 4:3 的 GBA 游戏（240×160）在 1280×720 屏幕上以 Fit 模式显示：**
  - `preRect = { x=160, y=90, w=960, h=540 }`（Fit 后的矩形，虚拟坐标）
  - `passViewW = 960, passViewH = 540`（项目 viewport 缩放基准）
  - RetroArch 的 `vp_width = 1280, vp_height = 720`（完整视图）

- **效果差异：**
  - `viewport × 1.0` 着色器（如 console-border）：
    - 项目：FBO = 960×540（仅游戏显示区，带黑边的边框无法填满屏幕）
    - RetroArch：FBO = 1280×720（完整屏幕，边框正确填满屏幕）
  - `video_scale = 4.0` 的 GB 边框着色器：
    - 顶点着色器期望 `OutputSize ≥ 640×576`（160×144 × 4）
    - 若 OutputSize 只有 960×540（甚至更小的 Original 模式 160×144），定位计算失败

### 修复方案

**问题1：passViewW/H 应使用完整视图物理像素**

```cpp
// 着色器激活时：使用完整视图物理像素（与 RetroArch gl->video_width×video_height 对应）
float wndScale = brls::Application::windowScale;
unsigned passViewW, passViewH;
if (m_renderChain.hasShader()) {
    passViewW = std::max(1u, static_cast<unsigned>(std::lround(width  * wndScale)));
    passViewH = std::max(1u, static_cast<unsigned>(std::lround(height * wndScale)));
} else {
    passViewW = std::max(1u, static_cast<unsigned>(std::lround(preRect.w)));
    passViewH = std::max(1u, static_cast<unsigned>(std::lround(preRect.h)));
}
```

**问题2：viewport 缩放着色器输出应显示到完整视图区域**

```cpp
if (m_renderChain.hasShader()) {
    unsigned shOutW = m_renderChain.outputW();
    unsigned shOutH = m_renderChain.outputH();
    if (shOutW > 0 && shOutH > 0 && (shOutW != passViewW || shOutH != passViewH)) {
        // source/absolute 缩放：输出与视口不同，重新计算显示矩形
        rect = m_display.computeRect(x, y, width, height, shOutW, shOutH);
    } else {
        // viewport 缩放：输出已填满完整视图，直接使用整个游戏视图区域
        rect = { x, y, width, height };
    }
}
```

## 修改文件

### `src/Game/game_view.cpp`

**修改1：passViewW/passViewH 计算逻辑**

着色器激活时，使用完整视图物理像素（`width × windowScale`）而非游戏显示矩形（`preRect.w`）作为 viewport 缩放基准。

**修改2：viewport 缩放着色器的显示矩形**

当着色器输出尺寸等于 passViewW×passViewH 时（viewport 缩放），将 FBO 显示到整个游戏视图区域 `{x, y, width, height}`，而非 `preRect`。

## 验证

### 编译验证
- 已成功编译，无新增错误
- 所有警告均来自第三方库（mgba、borealis），与本次修改无关

### 逻辑验证

**场景一：console-border 着色器（gb-pocket-5x.glslp 等）**
- 修复前：passViewW=960（Fit 模式 GBA 4:3 显示矩形）→ OutputSize=960×540 → 顶点着色器定位失败
- 修复后：passViewW=1280（完整视图）→ OutputSize=1280×720 → 游戏正确居中，边框填满屏幕

**场景二：source-scale 着色器（scalefx 3×等）**
- 不受影响：这类着色器输出尺寸 ≠ passViewW×passViewH → 仍使用 computeRect() 适配
- 行为不变

**场景三：普通 viewport 着色器（CRT、tvout 等）**
- 修复前：FBO 只覆盖 preRect 区域（960×540），再被 drawToScreen 缩放到 preRect 显示
- 修复后：FBO 覆盖完整视图（1280×720），直接显示到 {x, y, width, height}
- 与 RetroArch 行为一致

**场景四：无着色器（直通模式）**
- 不受影响：`passViewW/H` 在直通模式下不被 `run()` 使用
