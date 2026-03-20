# Session 114 工作汇报

## 任务分析

### 任务目标
修复 `gb-pocket-5x.glslp` console-border 类着色器渲染问题：#124 的修复未能解决游戏画面不可见的问题。

### 输入
- 问题描述：`example/shaders/handheld/console-border/gb-pocket-5x.glslp` 游戏画面仍然看不见
- 可能原因1：渲染流程中缩放有问题
- 可能原因2：glslp 解析功能不完全

### 根因分析

#### 验证 #124 修复是否已正确应用

`src/Game/game_view.cpp` 中的 #124 修复已正确应用：
- `passViewW/H` 使用完整视图的物理像素（`width * wndScale`）而非游戏显示矩形
- 视口缩放着色器的显示矩形使用完整视图区域（`rect = { x, y, width, height }`）

#### 发现的根本原因：GL_BLEND 状态污染

**核心 Bug**：`RetroShaderPipeline::process()` 在渲染中间 FBO 通道时，**未保存/禁用 GL_BLEND**。

**污染链路**：
1. NanoVG 在每帧的 `nvgEndFrame()` 内调用 `glnvg__renderFlush()`，该函数会执行 `glEnable(GL_BLEND)` 并设置混合模式为 `GL_ONE, GL_ONE_MINUS_SRC_ALPHA`（预乘 alpha 混合）
2. NanoVG 渲染结束后，**不会恢复 `GL_BLEND` 状态**（`glnvg__renderFlush` 末尾只恢复 `GL_CULL_FACE`）
3. 下一帧渲染时，在 `GameView::draw()` 被调用时，`GL_BLEND` 处于 **ENABLED** 状态
4. `RetroShaderPipeline::process()` 运行：

   - **pass0**（LCD 点阵着色器）向 FBO 渲染时：
     - 正常情况（无混合）：点间像素输出 `alpha=0`（透明），亮点像素输出 `alpha>0`
     - 有混合（GL_ONE, GL_ONE_MINUS_SRC_ALPHA）：
       ```
       result.a = src.a * 1 + dst.a * (1 - src.a)
                = 0 * 1 + 1 * (1 - 0) = 1   ← 点间像素 alpha 被强制为 1！
                = a * 1 + 1 * (1 - a) = 1    ← 亮点像素 alpha 也被强制为 1！
       ```
       **所有像素 alpha 变为 1，点阵 alpha 编码信息完全丢失**

5. **pass1–pass3** 对污染后的 pass0 输出继续处理，alpha=1 继续传播
6. **pass4** 从 `Pass2Texture` 读取时，`foreground.a = 1.0`（本应区分亮点/暗点的 alpha 值变为均匀的 1.0），合成公式：
   ```glsl
   // foreground.a = 1.0 时：
   foreground_term = fg * 1.0 * (1 - 1.0*0.8) = fg * 0.2
   background_term = bg * (0.9 - 1.0*0.8*0.95) = bg * 0.14
   // 输出：极暗的均匀颜色，无法识别游戏内容
   ```
7. **pass5** 将 pass4 的暗色输出与边框叠加，游戏区域呈现为不可见的暗色方块

**结论**：游戏画面"看不见"是因为 alpha 通道被 NanoVG 遗留的混合模式污染，而非缩放或解析问题。

## 修改文件

### `src/Video/renderer/RetroShaderPipeline.cpp`

在 `process()` 函数中增加完整的 GL 渲染状态保存/禁用/恢复逻辑，与 `DirectQuadRenderer::render()` 的模式一致：

**新增状态保存变量：**
```cpp
GLboolean prevBlendEn, prevDepthEn, prevStencilEn, prevScissorEn, prevCullEn;
GLint prevBlendSrcRGB, prevBlendDstRGB, prevBlendSrcAlpha, prevBlendDstAlpha;
```

**在渲染管线开始前保存并禁用：**
```cpp
prevBlendEn = glIsEnabled(GL_BLEND);
// ... 保存所有状态 ...
glDisable(GL_BLEND);        // 关键修复：禁用混合，保证 alpha 通道正确写入
glDisable(GL_DEPTH_TEST);   // 防止深度测试干扰 FBO 渲染
glDisable(GL_STENCIL_TEST); // 防止模板测试干扰
glDisable(GL_SCISSOR_TEST); // 防止裁剪测试限制 FBO 写入范围
glDisable(GL_CULL_FACE);    // 防止面剔除丢弃全屏四边形
```

**在管线结束后恢复：**
```cpp
if (prevBlendEn)   glEnable(GL_BLEND);   else glDisable(GL_BLEND);
// ... 恢复所有状态 ...
glBlendFuncSeparate(prevBlendSrcRGB, prevBlendDstRGB, prevBlendSrcAlpha, prevBlendDstAlpha);
```

## 验证

### 修复逻辑验证
- 禁用 GL_BLEND 后，pass0 的 alpha 通道正确编码点阵信息：
  - 点间像素：`alpha = 0`（透明，背景显示）
  - 亮点像素：`alpha = rgb_to_alpha`（范围约 0.03–0.9）
- pass4 的 `foreground.a` 正确区分点/背景区域，合成公式按设计工作
- pass5 的 `mix(frame, border, border.a)` 在边框透明区域正确显示游戏内容

### 参数解析验证
已验证以下 glslp 参数正确解析和传递：
- `video_scale = 4.0`：控制 pass0 游戏内容在视口中的缩放定位（640×576 in 视口中央）
- `SCALE = 1.25`、`OUT_X = 4000.0`、`OUT_Y = 2000.0`：控制 pass5 边框纹理采样范围
- `baseline_alpha = 0.03`、`grey_balance = 3.5`、`contrast = 0.8` 等：控制 LCD 效果参数

### 与 DirectQuadRenderer 的一致性
新增的 GL 状态管理代码与 `DirectQuadRenderer::render()` 使用完全相同的模式（`glIsEnabled` + `glGetIntegerv` + `glDisable` + 恢复），保证实现一致性。
