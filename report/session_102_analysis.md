# Session 102 分析报告：着色器全视口缩放导致过滤模式设置失效

## 任务目标

修复部分着色器（如 `res-independent-scanlines.glsl`）导致运行时过滤模式设置无效、画面完全全屏的问题。

## 问题分析

### 问题描述

加载 `.glsl` 扫描线着色器后，无论在设置中选择何种屏幕缩放/过滤模式，游戏画面始终填满整个屏幕，`display.mode`（Fit / Original / IntegerScale）等设置完全无效。

### 根本原因

**代码路径**：

1. `RetroShaderPipeline::init()` 在加载 `.glsl` 单文件时，自动构建单通道描述符，并将缩放类型设为视口缩放：

   ```cpp
   // src/Video/renderer/RetroShaderPipeline.cpp
   if (ext == ".glsl") {
       ShaderPassDesc single;
       single.scaleTypeX = ShaderPassDesc::ScaleType::Viewport;
       single.scaleTypeY = ShaderPassDesc::ScaleType::Viewport;
       single.scaleX = 1.0f;
       single.scaleY = 1.0f;
       // ...
   }
   ```

2. `computePassSize()` 对 `ScaleType::Viewport` 将输出尺寸计算为完整视口大小（如 1280×720）。

3. `game_view.cpp` 的 `draw()` 函数在渲染链处理后更新显示尺寸：

   ```cpp
   if (m_renderChain.outputW() > 0) displayW = static_cast<int>(m_renderChain.outputW()); // → 1280
   if (m_renderChain.outputH() > 0) displayH = static_cast<int>(m_renderChain.outputH()); // → 720
   ```

4. 然后直接用这个"着色器输出尺寸"调用 `computeRect()`：

   ```cpp
   // BUG：displayW=1280, displayH=720, viewW=1280, viewH=720
   // → scale = min(1280/1280, 720/720) = 1.0 → 画面填满整个视口
   beiklive::DisplayRect rect = m_display.computeRect(x, y, width, height,
                                                       static_cast<unsigned>(displayW),
                                                       static_cast<unsigned>(displayH));
   ```

   当游戏内容被视为与视口等大时，无论 `ScreenMode` 是 Fit、Original 还是 IntegerScale，`computeRect()` 的结果都是全屏矩形。

### 为何 NVG 拉伸不影响正确性

`nvgImagePattern()` 会将纹理拉伸以填充所指定的矩形区域：
- `ex, ey` 参数代表一个图像重复单元的屏幕空间尺寸
- 代码中 `ex = rect.w, ey = rect.h`，即纹理始终被拉伸填充整个 `rect`

因此，着色器输出纹理（可能是 1280×720）传入 `nvgImageFromGLTexture()` 的尺寸参数只影响 NVG 内部的纹理元数据，不影响最终的屏幕呈现大小。真正决定显示大小的是传给 `nvgImagePattern()` 的 `rect.w, rect.h`，而它来自 `computeRect()`。

## 修复方案

在 `game_view.cpp` 的 `computeRect()` 调用处，**始终使用原始游戏视频尺寸**（`m_texWidth × m_texHeight`）而非着色器输出尺寸。着色器输出尺寸仅用于 NVG 纹理创建，不用于显示矩形计算。

```cpp
// 修复后
unsigned contentW = (m_texWidth  > 0) ? m_texWidth  : static_cast<unsigned>(displayW);
unsigned contentH = (m_texHeight > 0) ? m_texHeight : static_cast<unsigned>(displayH);
beiklive::DisplayRect rect = m_display.computeRect(x, y, width, height, contentW, contentH);
```

### 修复效果分析

| 场景 | 修复前 | 修复后 |
|------|--------|--------|
| 无着色器 | displayW=游戏宽, 正常 | contentW=m_texWidth=游戏宽, 不变 |
| 上采样着色器（hq2x，256→512） | displayW=512, Fit 模式下与游戏宽等效 | contentW=256, Fit/IntegerScale 行为一致 |
| 扫描线着色器（视口缩放，视口=1280） | displayW=1280 → 全屏 BUG | contentW=256 → 正确缩放 |

### 潜在影响

- **无着色器**：`m_texWidth/m_texHeight` 等于 `displayW/displayH`，行为完全不变
- **Fit / Fill / IntegerScale 模式**：对于宽高比不变的上采样着色器，缩放结果相同
- **Original 模式 + 上采样着色器**：显示 1:1 游戏像素（而非 1:1 着色器输出像素），更符合该模式语义
- **视口缩放着色器**：修复全屏 BUG，画面按游戏内容宽高比正确缩放并居中

## 影响文件

| 文件 | 修改类型 |
|------|---------|
| `src/Game/game_view.cpp` | 修复 computeRect() 使用原始游戏尺寸 |
