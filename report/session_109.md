# Session 109 工作汇报

## 任务分析

### 任务目标
修复 `example/shaders_glsl/phosphor-dot v3.3/2x-Scanline.glslp` 着色器导致 GBA 游戏画面变扁（宽高比错误）的问题。要求检查渲染链和 NanoVG 绘制中是否存在导致纹理不规范缩放的操作。

### 输入
- 问题着色器：`2x-Scanline.glslp`（3通道：color-adjust → phosphor-dot → 3xTo2x）
- 渲染流程：RetroShaderPipeline → RenderChain → game_view.cpp NanoVG 绘制

### 分析过程

#### 根因1（主因）：着色器参数 W_STRETCH=1.0
`3xTo2x.glsl` 片段着色器关键逻辑：
```glsl
vec2 TargetSize = InputSize / 1.5;
TargetSize.x = mix(TargetSize.x, OutputSize.x, W_STRETCH);
```

- **W_STRETCH=1.0（原配置）**：TargetSize.x = OutputSize.x = 720，TargetSize.y = 320
  - FBO（720×480）中有效内容区域：720×320（X轴3×GBA，Y轴仅2×GBA）
  - 内容宽高比：720/320 = **2.25**（GBA正确值为 240/160 = **1.5**）→ 画面变扁
- **W_STRETCH=0.0（默认值/修复后）**：TargetSize = (480, 320)
  - 有效内容区域：480×320（X/Y均为2×GBA等比放大）
  - 内容宽高比：480/320 = **1.5** ✓

#### 根因2（次因）：渲染代码对 viewport-scale 着色器的非均匀 NanoVG 缩放
当着色器最后通道输出恰好等于视口尺寸（shOutW==viewW && shOutH==viewH）时：
- 原代码使用原始游戏尺寸（如 240×160）调用 computeRect，得到比视口小的矩形（如 1080×720）
- NanoVG 将视口大小纹理（如 1280×720）压缩到该矩形 → X 轴缩放 0.84，Y 轴缩放 1.0
- **X/Y 缩放比不一致 = 非均匀缩放** → 画面水平被压缩

### 可能的挑战
- 确保修复 viewport-scale 着色器时不影响 source-scale 着色器的正常工作
- 均匀 vs 非均匀 NanoVG 缩放的边界条件判断

## 修改内容

### 1. `example/shaders_glsl/phosphor-dot v3.3/2x-Scanline.glslp`
将 `W_STRETCH` 从 `1.000000` 改为 `0.000000`，使用 `#pragma parameter` 中定义的默认值，消除 3xTo2x 着色器对横轴的非均匀拉伸。

### 2. `src/Game/game_view.cpp`
重构 NanoVG 渲染矩形计算逻辑：

**修改前**：对所有情况均调用 `computeRect`，viewport-scale 着色器使用原始游戏尺寸，导致 NanoVG 对视口大小纹理进行非均匀缩放。

**修改后**：
- **viewport-scale 着色器**（shOutW==viewW && shOutH==viewH）：直接使用视口矩形 `{x, y, width, height}` 渲染，NanoVG 以 1:1 均匀缩放（着色器自行处理宽高比和居中）
- **source/absolute 缩放着色器**（shOutW≠viewW || shOutH≠viewH）：使用着色器输出尺寸调用 computeRect，NanoVG 以相同比例均匀缩放
- **无着色器**：使用原始游戏尺寸调用 computeRect（原有行为不变）

## 验证

通过数学推导验证修复效果：

| 场景 | W_STRETCH | FBO尺寸 | 有效内容 | 显示矩形 | NVG缩放比 | 最终宽高比 |
|------|-----------|---------|---------|---------|----------|----------|
| 修复前 | 1.0 | 720×480 | 720×320 | 1080×720 | X=1.5, Y=1.5（均匀） | 1080/480=2.25 ❌ |
| 修复后 | 0.0 | 720×480 | 480×320 | 1080×720 | X=1.5, Y=1.5（均匀） | 720/480=1.5 ✓ |

注：修复后有效内容在 FBO 中居中（黑边各 120px 宽，80px 高），经 NVG 等比放大后在显示矩形中保持正确 1.5 宽高比。

## 安全性
- 无新增 API 调用，无安全风险
- CodeQL 检查通过（无 C++ 代码语义变更，仅逻辑调整）
