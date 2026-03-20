# Session 113 工作汇报

## 任务分析

### 任务目标
修复 `gb-pocket-5x.glslp` console-border 类着色器渲染问题：使用该着色器时，只能看到图片资源（边框背景图），看不到游戏画面。

### 问题根因分析

**根本原因：着色器视口（viewport）尺寸计算错误**

`gb-pocket-5x.glslp` 是一种 console-border（控制台边框）着色器，其 pass 0 的顶点着色器通过以下公式将游戏内容定位到视口中央：

```glsl
vec2 scaled_video_out = (InputSize.xy * vec2(video_scale));
gl_Position = MVPMatrix * VertexCoord / vec4(vec2(outsize.xy / scaled_video_out), 1.0, 1.0);
```

对于 GB 游戏（160×144），`video_scale=4.0`，`scaled_video_out = 640×576`。  
该公式要求 `OutputSize（视口）>= scaled_video_out`，才能将游戏正确居中在视口中。

**问题所在（修复前）：**  
代码使用游戏显示矩形尺寸（`preRect.w × preRect.h`）作为着色器视口：
- 若用户使用 Original 模式（1:1）：passViewW=160, passViewH=144
- `scale_factor = 160/640 = 0.25`，顶点坐标缩放至 ±4（NDC），造成游戏内容几乎全部被裁剪
- Pass 4 的 `Pass2Texture` 中几乎没有可见游戏内容
- Pass 5 的 `mix(frame, border, border.a)` 中 frame 为近黑色，只显示边框图片

即使是 Fit 模式也存在问题：视口不够大，边框无法在正确的屏幕区域展示。

### 修复方案

当着色器激活时，使用完整视图区域的物理像素（`width × windowScale` × `height × windowScale`）作为着色器视口，而不是游戏显示矩形。同时，当着色器输出尺寸等于视口尺寸（viewport 缩放着色器）时，将输出显示到完整视图区域。

**对不同类型着色器的影响：**
- **border/console-border 着色器（viewport 缩放）**：使用完整视图，游戏正确定位在边框中央 ✓
- **source-scale 着色器（scalefx 等）**：输出尺寸与视口不同，仍使用 computeRect 适配到视图 ✓（行为不变）
- **CRT/tvout 等 viewport 着色器**：使用完整视图，游戏填充完整视图区域（行为有微小变化，但符合 RetroArch 的设计预期）

## 修改文件

### `src/Game/game_view.cpp`

**修改1：passViewW/passViewH 计算逻辑**

```cpp
// 修改前：
unsigned passViewW = std::max(1u, static_cast<unsigned>(std::lround(preRect.w)));
unsigned passViewH = std::max(1u, static_cast<unsigned>(std::lround(preRect.h)));

// 修改后：
float wndScale = brls::Application::windowScale;
unsigned passViewW, passViewH;
if (m_renderChain.hasShader()) {
    // 着色器模式：使用完整视图物理像素作为视口
    passViewW = std::max(1u, static_cast<unsigned>(std::lround(width  * wndScale)));
    passViewH = std::max(1u, static_cast<unsigned>(std::lround(height * wndScale)));
} else {
    // 无着色器：使用游戏显示矩形尺寸
    passViewW = std::max(1u, static_cast<unsigned>(std::lround(preRect.w)));
    passViewH = std::max(1u, static_cast<unsigned>(std::lround(preRect.h)));
}
```

**修改2：显示矩形（rect）计算逻辑**

```cpp
// 修改前（viewport 着色器默认使用 preRect）：
beiklive::DisplayRect rect = preRect;
if (m_renderChain.hasShader()) {
    if (shOutW != passViewW || shOutH != passViewH) {
        rect = m_display.computeRect(...);
    }
    // else: 使用 preRect（游戏显示区域）
}

// 修改后（viewport 着色器使用完整视图区域）：
beiklive::DisplayRect rect = preRect;
if (m_renderChain.hasShader()) {
    if (shOutW != passViewW || shOutH != passViewH) {
        // source/absolute 缩放：输出与视口不同，适配到视图
        rect = m_display.computeRect(...);
    } else {
        // viewport 缩放：输出填满视口，显示到完整视图区域
        rect = { x, y, width, height };
    }
}
```

## 验证

- 代码逻辑分析：border 着色器的 pass 0 能正确将游戏定位在 1280×720 视口中央（UV [0.25, 0.75] × [0.1, 0.9]）
- Pass 4 复合游戏内容与背景图，游戏内容在正确位置可见
- Pass 5 通过 `mix(frame, border, border.a)` 在边框透明区域显示游戏内容
- Source-scale 着色器（scalefx）行为不变：输出尺寸≠视口时使用 computeRect 适配

## 技术细节

- `video_scale = 4.0`：游戏以 4× 缩放渲染到 1280×720 视口中（640×576 in 1280×720）
- `SCALE = 1.25`：pass 5 采样视口中心 80%×80% 区域（vTexCoord [0.1, 0.9]）
- `OUT_X = 4000, OUT_Y = 2000`：边框纹理设计分辨率，控制边框在不同分辨率下的裁剪范围
- 修复后 `tex_border` 范围约为 [0.34, 0.66] × [0.32, 0.68]，正确采样边框纹理的设备框架区域
