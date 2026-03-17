# Session 102 工作汇报：修复着色器全视口缩放导致过滤模式设置失效

## 任务说明

修复部分着色器（如 `example/shaders/scanlines/shaders/res-independent-scanlines.glsl`）导致运行时过滤/缩放模式设置无效、画面完全全屏的问题。

## 问题根因

`res-independent-scanlines.glsl` 属于依赖 `gl_FragCoord` 的扫描线着色器，需要在整个视口分辨率下渲染。加载该 `.glsl` 文件时，`RetroShaderPipeline::init()` 自动构建单通道，并将缩放类型设为视口缩放（`ScaleType::Viewport`，`scale=1.0`）。

着色器管线处理后，`m_renderChain.outputW()/outputH()` 返回视口尺寸（如 1280×720），随后 `game_view.cpp` 将其作为"游戏内容尺寸"传给 `computeRect()`：

```cpp
// 问题代码：displayW=1280, displayH=720, 视口也是 1280×720
// → scale = min(1280/1280, 720/720) = 1.0 → 全屏
beiklive::DisplayRect rect = m_display.computeRect(x, y, width, height,
                                                    static_cast<unsigned>(displayW),
                                                    static_cast<unsigned>(displayH));
```

结果是无论何种 `ScreenMode`，画面都填满整个视口，过滤/缩放模式设置完全失效。

## 修改内容

**文件**：`src/Game/game_view.cpp`

在 `computeRect()` 调用处，改为使用**原始游戏视频尺寸**（`m_texWidth × m_texHeight`）而非着色器输出尺寸：

```cpp
// 修改后：使用原始游戏尺寸计算显示矩形
unsigned contentW = (m_texWidth  > 0) ? m_texWidth  : static_cast<unsigned>(displayW);
unsigned contentH = (m_texHeight > 0) ? m_texHeight : static_cast<unsigned>(displayH);
beiklive::DisplayRect rect = m_display.computeRect(x, y, width, height,
                                                    contentW, contentH);
```

NVG 的 `nvgImagePattern()` 会将着色器输出纹理自动拉伸填充到 `computeRect()` 计算出的矩形区域，故 NVG 纹理创建仍使用正确的着色器输出纹理尺寸，不受影响。

## 修改文件清单

| 文件 | 修改说明 |
|------|---------|
| `src/Game/game_view.cpp` | `computeRect()` 改用原始游戏视频尺寸，不再受着色器输出尺寸影响 |
| `report/session_102_analysis.md` | 任务分析文档 |
| `report/session_102.md` | 本工作汇报 |

## 验证要点

1. 加载 `res-independent-scanlines.glsl`，游戏画面按配置的 `display.mode` 正确缩放（Fit、Original、IntegerScale 均有效）
2. 无着色器时行为不变（`m_texWidth = displayW`，结果一致）
3. 上采样着色器（hq2x 等）在 Fit 模式下行为不变（宽高比相同，缩放结果等效）
