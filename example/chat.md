
我已经逐段检查了你的 **GLSLPParser + RetroShaderPipeline**。整体实现质量很高，**80–90% 已符合 RetroArch GLSL preset 规范**。但仍有一些 **规范偏差 / 兼容性问题 / 潜在 bug**。下面按 **重要程度**给你逐条分析。

---

# 一、关键规范问题（必须修改）

## 1 `scale_type` 默认值是否正确

你的实现：

```cpp
return ShaderPassDesc::ScaleType::Source;
```

代码位置：



```cpp
return ShaderPassDesc::ScaleType::Source; // 默认 source
```

这是 **正确的**。

RetroArch 默认：

```
scale_type = source
scale = 1
```

这一点 ✔ 完全符合规范。

---

# 二、严重问题：最后一个 pass 必须输出到 screen

你的 pipeline：

在 `process()` 中 **所有 pass 都创建 FBO**：



```cpp
if (!allocateFBO(pass, outW, outH))
```

然后：

```
pass → FBO → texture
```

RetroArch 的真实行为：

```
pass0 -> FBO
pass1 -> FBO
pass2 -> SCREEN
```

也就是：

**最后一个 pass 不应该创建 FBO。**

否则会导致：

* 多一次渲染
* viewport shader 行为错误
* CRT shader 比例错误

---

### 正确逻辑

应该：

```cpp
bool lastPass = idx == m_passes.size() - 1;
```

然后：

```
if lastPass
    render to screen
else
    render to FBO
```

---

# 三、scale 计算逻辑错误（重要）

你的 `computePassSize`：



```cpp
outW = calcAxis(desc.scaleTypeX, desc.scaleX, videoW, vpW);
outH = calcAxis(desc.scaleTypeY, desc.scaleY, videoH, vpH);
```

问题：

RetroArch 的 `source` 不是 **videoW/videoH**。

而是：

```
上一 pass 输出
```

正确应该是：

```
src = previousPassSize
```

你现在传入：

```
videoW
videoH
```

这会导致：

例如：

```
pass0 scale2x
pass1 scale2x
```

RetroArch：

```
240x160
↓
480x320
↓
960x640
```

你的实现：

```
240x160
↓
480x320
↓
480x320
```

第二次 scale 不会生效。

---

### 正确写法

传入：

```
currentW
currentH
```

而不是：

```
videoW
videoH
```

---

# 四、float_framebuffer 未实现（规范不完整）

你解析了：

```
float_framebufferN
```

代码：



```cpp
pass.floatFramebuffer
```

但创建 FBO 时：



```cpp
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
```

应该：

```
float_framebuffer = true
→ GL_RGBA16F
```

否则很多 shader（HDR / bloom）会坏。

---

# 五、srgb_framebuffer 未实现

解析了：

```
pass.srgbFramebuffer
```

但没有任何：

```
GL_FRAMEBUFFER_SRGB
```

或者：

```
GL_SRGB8_ALPHA8
```

实现。

---

# 六、wrap_mode 未实现

GLSLP 支持：

```
wrap_modeN
```

例如：

```
repeat
mirrored_repeat
```

你的实现：

FBO纹理固定：

```
GL_CLAMP_TO_EDGE
```

代码：



```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
```

应该支持：

```
clamp_to_edge
repeat
mirrored_repeat
```

---

# 七、mipmap_input 未实现

RetroArch：

```
mipmap_inputN = true
```

会：

```
glGenerateMipmap()
```

你的 parser 没解析。

---

# 八、external texture wrap 未实现

GLSLP 支持：

```
noise_wrap_mode = repeat
noise_mipmap = true
```

你只解析：

```
noise_linear
```

缺少：

```
wrap
mipmap
```

---

# 九、alias 使用逻辑正确

这部分实现非常好 👍

代码：



```cpp
extraTexUnits.emplace_back(prev.alias + "Texture", unit);
```

符合 RetroArch：

```
alias0 = firstpass
```

shader：

```
uniform sampler2D firstpassTexture;
```

✔ 完全正确。

---

# 十、PassPrevNTexture 规则正确

你的实现：



```cpp
extraTexUnits.emplace_back(
    "PassPrev" + std::to_string(prevN) + "Texture", unit);
```

RetroArch：

```
PassPrev1Texture
PassPrev2Texture
```

✔ 正确。

---

# 十一、OrigTexture 实现正确

你的实现：



```cpp
extraTexUnits.emplace_back("OrigTexture", unit);
```

RetroArch 也提供：

```
OrigTexture
```

✔ 正确。

---

# 十二、parameters 解析正确

GLSLP：

```
parameters = A;B
A = 0.5
B = 1.0
```

你的解析：



```cpp
std::vector<GLSLPParamOverride>
```

✔ 正确。

---

# 十三、#pragma parameter 解析正确

你实现：

```
#pragma parameter name "desc" default min max step
```

代码：



```cpp
parseParamMeta
```

逻辑：

* name
* desc
* default
* min
* max
* step

✔ 非常标准。

---

# 十四、TextureSize / InputSize 设置正确

你设置：

```
TextureSize
InputSize
OutputSize
```

完全符合 RetroArch。

---

# 十五、缺少 PrevTexture

RetroArch shader 常用：

```
PrevTexture
```

你的实现没有。

应该绑定：

```
上一帧纹理
```

---

# 十六、Texture sampler 别名兼容很好

你绑定：

```
Texture
Source
tex
texture
```

这对兼容老 shader 很重要 👍

---

# 总体兼容度评估

| 模块             | 兼容度 |
| -------------- | --- |
| GLSLP parser   | 90% |
| pipeline       | 80% |
| shader uniform | 90% |
| texture system | 70% |

总体：

```
≈ 85% RetroArch 兼容
```

已经非常接近完整实现。

---

# 最关键需要修的 3 个问题

优先级：

### ① scale source 使用 previous pass

现在：

```
videoW/videoH
```

必须改为：

```
previous pass size
```

---

### ② 最后一层 pass 不应该创建 FBO

应该：

```
render to screen
```

---

### ③ float_framebuffer

必须支持：

```
GL_RGBA16F
```

否则很多 shader 不工作。

---




你的渲染循环本身 **基本符合 RetroArch 逻辑**，问题其实不在这里，而是在：

```
m_renderChain.run(...)
```

里面 **是否为最后一个 pass 创建了 FBO**。

RetroArch 的规范是：

```
如果最后一层 pass 没有显式 scale
→ 不创建 FBO
→ 直接渲染到屏幕
```

即：

```
pass0 -> FBO
pass1 -> FBO
pass2 -> Screen
```

而不是：

```
pass0 -> FBO
pass1 -> FBO
pass2 -> FBO -> Screen
```

---

# 一、你现在代码的逻辑问题

你的代码：

```cpp
GLuint chainOut = m_renderChain.run(
    m_texture,
    m_texWidth,
    m_texHeight,
    viewportW,
    viewportH
);
```

意味着：

```
renderChain 必须返回一个 texture
```

但 **如果最后 pass 直接渲染到屏幕**：

```
最后pass没有输出 texture
```

因此：

```
chainOut = 0
```

才是正确行为。

你的代码其实已经 **兼容这个逻辑**：

```cpp
if (chainOut != 0) {
    displayTex = chainOut;
}
```

如果：

```
chainOut == 0
```

就会：

```
displayTex = m_texture
```

但这里其实会出现 **逻辑错误**。

因为：

```
最后pass已经画到屏幕
```

而你还会再：

```
NanoVG 再画一次
```

---

# 二、正确设计（推荐）

你的 `renderChain.run()` 应该返回：

```
0  = 已经渲染到屏幕
tex = 需要你继续显示
```

因此修改：

```cpp
GLuint chainOut = m_renderChain.run(...);

if (chainOut == 0) {
    // shader 已经完成最终输出
    return;
}

displayTex = chainOut;
```

这样：

```
shader pipeline → screen
```

NanoVG 就不会重复绘制。

---

# 三、renderChain 内部应该怎么写

关键逻辑：

```cpp
for (int i = 0; i < passes; i++)
{
    bool last = (i == passes - 1);

    if (last && !pass[i].hasScale) {
        bindFramebuffer(0);   // 直接屏幕
    } else {
        bindFramebuffer(pass[i].fbo);
    }

    drawPass(i);
}
```

最后：

```cpp
if (last_pass_rendered_to_screen)
    return 0;

return lastFboTexture;
```

---

# 四、FBO创建规则（RetroArch）

创建 FBO 的条件：

```
非最后pass
OR
最后pass定义了scale
```

代码：

```cpp
bool needFBO = !(last && !pass.hasScale);
```

---

# 五、纹理尺寸计算

RetroArch pass 输出尺寸：

```
scale_type = source
    size = input * scale

scale_type = viewport
    size = viewport * scale

scale_type = absolute
    size = scale
```

如果 **最后pass无scale**

```
output = viewport
```

但：

```
不创建FBO
```

---

# 六、你的代码建议修改

在你这段代码里：

```cpp
GLuint chainOut = m_renderChain.run(...);
```

改成：

```cpp
GLuint chainOut = m_renderChain.run(...);

if (chainOut == 0) {
    // shader 已经直接输出到屏幕
    return;
}

displayTex = chainOut;
displayW   = m_renderChain.outputW();
displayH   = m_renderChain.outputH();
```

---

# 七、NanoVG 的一个隐藏问题

NanoVG：

```
nvgImageFromGLTexture
```

默认 **会自己设置 texture filter**。

如果 shader pipeline 已经做了：

```
GL_NEAREST
GL_LINEAR
```

可能被 NanoVG 覆盖。

建议：

```
NVG_IMAGE_NEAREST
NVG_IMAGE_LINEAR
```

显式控制。

---

# 八、你这个架构其实已经很接近 RetroArch

你的 pipeline：

```
Core frame
   ↓
uploadFrame
   ↓
RenderChain.run()
   ↓
NanoVG
```

和 RetroArch：

```
Core frame
   ↓
shader passes
   ↓
screen quad
```

唯一差异是：

```
RetroArch 不会再用 UI 再绘制
```
